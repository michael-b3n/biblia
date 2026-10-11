#include "bibstd/system/tray.hpp"
#include "bibstd/execution/execution.hpp"
#include "bibstd/execution/launch.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/log.hpp"
#include "bibstd/util/non_owning_ptr.hpp"
#include "bibstd/util/visit_helper.hpp"

#include <algorithm>
#include <condition_variable>
#include <utility>

namespace bibstd::system
{
namespace
{

///
/// Convert UTF-8 text to the ANSI code page, the tray menu is built with the ANSI API.
/// Characters the code page cannot represent are replaced.
///
[[nodiscard]] auto to_ansi(const std::string& text) -> std::string
{
  const auto wide_count = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
  if(wide_count <= 0)
  {
    return text;
  }
  auto wide = std::wstring(static_cast<std::size_t>(wide_count), wchar_t{});
  MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wide.data(), wide_count);
  const auto ansi_count = WideCharToMultiByte(CP_ACP, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
  if(ansi_count <= 0)
  {
    return text;
  }
  auto ansi = std::string(static_cast<std::size_t>(ansi_count), char{});
  WideCharToMultiByte(CP_ACP, 0, wide.c_str(), -1, ansi.data(), ansi_count, nullptr, nullptr);
  ansi.pop_back(); // terminating null
  return ansi;
}

} // namespace

///
///
auto tray::init(const icon_buffer icon, std::vector<entry_type>&& entries) -> util::shared_scope_guard
{
  static util::shared_scope_guard::creator guard_creator;
  static std::condition_variable cv_init;

  auto lock = std::unique_lock{mtx_};
  auto guard = guard_creator.create(
    []()
    {
      auto scope = decltype(scope_){};
      auto loop = decltype(loop_){};
      {
        // Not held any longer, a callback that is waited for below may call set_text
        const auto lock = std::scoped_lock{mtx_};
        scope = std::move(scope_);
        loop = std::move(loop_);
      }
      try
      {
        auto close = []
        {
          if(tray_)
          {
            tray_->exit();
            tray_.reset();
          }
        };
        if(loop)
        {
          loop->call(close);
        }
      }
      catch(...)
      {
        LOG_ERROR("exception occurred: {}", util::exception_report());
      }
      scope.reset();
      loop.reset();
      {
        const auto lock = std::scoped_lock{mtx_};
        active_ = false;
      }
      cv_init.notify_all();
    }
  );
  if(guard.is_initial_instance())
  {
    cv_init.wait(lock, []() { return !active_; });
    active_ = true;
    try
    {
      scope_ = std::make_unique<execution::scope>();
      loop_ = std::make_unique<message_loop>();
    }
    catch(...)
    {
      // The guard closes what exists and takes the lock for it
      lock.unlock();
      throw;
    }
    auto create = [&, scope = util::non_owning_ptr<execution::scope>{scope_.get()}]
    {
      // The callbacks are called by the tray on the loop thread and run on the thread pool
      const auto run = [scope](std::function<void()> f)
      { execution::launch(ex::schedule(ex::get_parallel_scheduler()) | ex::then(std::move(f)), *scope); };
      const auto void_callback_wrapper = [run](const auto& callback) { return [callback, run]() { run(callback); }; };
      const auto toggle_callback_wrapper = [run](const auto& callback)
      { return [callback, run](bool flag) { run(callback(flag)); }; };
      tray_ = std::make_unique<Tray::Tray>("system_tray_identifier", Tray::Icon(icon.buffer));
      std::ranges::for_each(
        entries,
        [&](const auto& entry)
        {
          util::visit_lambdas(
            entry,
            [&](const button& v) { tray_->addEntry(Tray::Button(to_ansi(v.text), void_callback_wrapper(v.callback))); },
            [&](const label& v) { tray_->addEntry(Tray::Label(to_ansi(v.text))); },
            [&]([[maybe_unused]] const separator& /*v*/) { tray_->addEntry(Tray::Separator()); },
            [&](const toggle& v)
            { tray_->addEntry(Tray::Toggle(to_ansi(v.text), v.state, toggle_callback_wrapper(v.callback))); }
          );
        }
      );
    };
    const auto loop = util::non_owning_ptr<message_loop>{loop_.get()};
    // Released first: an exception of the tray reaches the caller, the guard then closes what exists
    lock.unlock();
    loop->call(create);
  }
  return guard;
}

///
///
auto tray::set_text(const std::size_t index, std::string text) -> void
{
  const auto lock = std::scoped_lock{mtx_};
  if(!loop_)
  {
    return;
  }
  // The menu belongs to the loop thread, so it is rebuilt there.
  auto rebuild = [index, text = to_ansi(text)]() mutable
  {
    if(!tray_)
    {
      return;
    }
    const auto entries = tray_->getEntries();
    if(index < entries.size())
    {
      entries[index]->setText(std::move(text));
    }
  };
  loop_->post(std::move(rebuild));
}

} // namespace bibstd::system
