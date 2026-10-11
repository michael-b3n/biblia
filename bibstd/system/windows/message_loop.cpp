#include "bibstd/system/windows/message_loop.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/log.hpp"

#include "bibstd/system/windows/win.hpp"

#include <exception>
#include <memory>
#include <tuple>
#include <utility>

namespace bibstd::system
{
namespace
{

constexpr auto post_message = UINT{WM_APP};
constexpr auto call_message = UINT{WM_APP + 1};
constexpr auto quit_message = UINT{WM_APP + 2};
constexpr auto* window_class_name = L"bibstd_message_loop";

///
/// Task of message_loop::call and the exception it ended with.
///
struct call_data final
{
  const std::function<void()>& task;
  std::exception_ptr error;
};

///
/// Register the class of the loop windows, once for the process.
/// \return true if the class is registered
///
[[nodiscard]] auto register_window_class(const WNDPROC procedure) -> bool
{
  static const auto registered = [procedure]
  {
    auto window_class = WNDCLASSEXW{};
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = procedure;
    window_class.hInstance = GetModuleHandleW(nullptr);
    window_class.lpszClassName = window_class_name;
    return RegisterClassExW(&window_class) != 0;
  }();
  return registered;
}

} // namespace

///
/// Window procedure of the loop, a member to reach its handler.
///
struct message_loop::access final
{
  static auto CALLBACK procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT;
};

///
///
auto CALLBACK message_loop::access::procedure(const HWND window, const UINT message, const WPARAM wparam, const LPARAM lparam)
  -> LRESULT
{
  switch(message)
  {
  case WM_NCCREATE:
  {
    const auto* const create = reinterpret_cast<const CREATESTRUCTW*>(lparam);
    SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    return DefWindowProcW(window, message, wparam, lparam);
  }
  case post_message:
  {
    const auto task = std::unique_ptr<task_type>{reinterpret_cast<task_type*>(lparam)};
    try
    {
      (*task)();
    }
    catch(...)
    {
      LOG_ERROR("message loop task exception: {}", util::exception_report());
    }
    return 0;
  }
  case call_message:
  {
    auto* const call = reinterpret_cast<call_data*>(lparam);
    try
    {
      call->task();
    }
    catch(...)
    {
      call->error = std::current_exception();
    }
    return 0;
  }
  case quit_message: DestroyWindow(window); return 0;
  case WM_DESTROY: PostQuitMessage(0); return 0;
  default: break;
  }
  if(
    auto* const self = reinterpret_cast<message_loop*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    self != nullptr && self->handler_
  )
  {
    try
    {
      self->handler_(message, wparam);
    }
    catch(...)
    {
      LOG_ERROR("message loop handler exception: {}", util::exception_report());
    }
  }
  return DefWindowProcW(window, message, wparam, lparam);
}

///
///
message_loop::message_loop(handler_type&& handler)
  : handler_{std::move(handler)}
  , thread_{[this] { run(); }}
{
  ready_.wait();
  if(window_ == nullptr)
  {
    throw util::exception{"failed to create message loop window"};
  }
  LOG_INFO("message loop start thread: id={}", thread_.get_id());
}

///
///
message_loop::~message_loop() noexcept
{
  LOG_INFO("message loop stop thread: id={}", thread_.get_id());
  // Queued behind the posted tasks, so they run first
  PostMessageW(static_cast<HWND>(window_.load()), quit_message, 0, 0);
}

///
///
auto message_loop::window() const noexcept -> void*
{
  return window_;
}

///
///
auto message_loop::post(task_type&& task) -> void
{
  auto posted = std::make_unique<task_type>(std::move(task));
  if(PostMessageW(static_cast<HWND>(window_.load()), post_message, 0, reinterpret_cast<LPARAM>(posted.get())) == 0)
  {
    LOG_ERROR("message loop failed to post a task: error={}", GetLastError());
    return;
  }
  // Owned by the message from here on
  std::ignore = posted.release();
}

///
///
auto message_loop::call(const std::function<void()>& task) -> void
{
  auto call = call_data{.task = task, .error = nullptr};
  SendMessageW(static_cast<HWND>(window_.load()), call_message, 0, reinterpret_cast<LPARAM>(&call));
  if(call.error)
  {
    std::rethrow_exception(call.error);
  }
}

///
///
auto message_loop::run() noexcept -> void
{
  // A window instead of thread messages, those are lost while a menu runs its own loop
  const auto window =
    register_window_class(&access::procedure)
      ? CreateWindowExW(0, window_class_name, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, GetModuleHandleW(nullptr), this)
      : nullptr;
  window_ = window;
  ready_.count_down();
  if(window == nullptr)
  {
    return;
  }
  auto message = MSG{};
  while(IsWindow(window) != 0)
  {
    const auto result = GetMessageW(&message, nullptr, 0, 0);
    if(result == -1)
    {
      LOG_ERROR("message loop failed to get message: error={}", GetLastError());
      break;
    }
    // WM_QUIT of somebody else on this thread, e.g. a tray that is closed, is not the end
    if(result != 0)
    {
      TranslateMessage(&message);
      DispatchMessageW(&message);
    }
  }
}

} // namespace bibstd::system
