#include "bibstd/workflow/workflow_hotkey.hpp"
#include "bibstd/framework/setting.hpp"
#include "bibstd/framework/thread_pool.hpp"
#include "bibstd/system/hotkey.hpp"
#include "bibstd/util/contains.hpp"
#include "bibstd/util/enum.hpp"

#include <array>
#include <format>
#include <ranges>

namespace bibstd::workflow
{
namespace
{

///
/// Keys a hotkey can be bound to. The mouse buttons the key enum also names are left out, the
/// system registers a hotkey for a key of the keyboard only.
/// \return assignable keys
///
[[nodiscard]] auto assignable_keys() -> std::vector<system::hotkey_common::key>
{
  using enum system::hotkey_common::key;
  static constexpr auto mouse_buttons =
    std::array{vk_left_button, vk_right_button, vk_middle_button, vk_x1_button, vk_x2_button};
  return util::enum_values<system::hotkey_common::key>() |
         std::views::filter([](const auto key) { return !util::contains(mouse_buttons, key); }) |
         std::ranges::to<std::vector>();
}

///
/// Path of the setting naming the modifier of the hotkey of \p callback_path.
/// \return setting path
///
[[nodiscard]] auto modifier_setting_path(const std::string& callback_path) -> std::string
{
  return std::format("{}.hotkey.modifier", callback_path);
}

///
/// Path of the setting naming the key of the hotkey of \p callback_path.
/// \return setting path
///
[[nodiscard]] auto key_setting_path(const std::string& callback_path) -> std::string
{
  return std::format("{}.hotkey.key", callback_path);
}

} // namespace

///
///
workflow_hotkey::workflow_hotkey(std::shared_ptr<workflow_settings> workflow_settings)
  : workflow_settings_{std::move(workflow_settings)}
  , thread_pool_guard_{framework::thread_pool::init()}
  , hotkey_guard_{system::hotkey::init()}
{
}

///
///
workflow_hotkey::~workflow_hotkey() noexcept = default;

///
///
auto workflow_hotkey::available_callbacks() const -> std::vector<std::string>
{
  const auto lock = std::scoped_lock{mtx_};
  return callbacks_ | std::views::keys | std::ranges::to<std::vector<std::string>>();
}

///
///
auto workflow_hotkey::register_callback(const path_type& path, const key_modifier default_modifier, const key default_key)
  -> shared_sig_type
{
  auto shared_sig = shared_sig_type{};
  {
    const auto lock = std::scoped_lock{mtx_};
    if(const auto it = callbacks_.find(path); it != std::cend(callbacks_))
    {
      return it->second.shared_sig;
    }

    auto data = callback_data{
      .shared_sig = std::make_shared<signal::signal_type<void()>>(),
      .modifier_setting = workflow_settings_->create_setting(modifier_setting_path(path), default_modifier),
      .key_setting = workflow_settings_->create_setting(
        key_setting_path(path),
        default_key,
        std::make_shared<framework::setting_validator_list<hotkey_type::key>>(assignable_keys())
      ),
      .registered = std::nullopt,
      .connections = {}
    };
    // A hotkey the user chose takes effect as soon as it is written, both halves of it on their own
    data.connections.emplace_back(
      data.modifier_setting->connect(&framework::setting_signals::value_changed, [this, path]() { apply_hotkey(path); })
    );
    data.connections.emplace_back(
      data.key_setting->connect(&framework::setting_signals::value_changed, [this, path]() { apply_hotkey(path); })
    );
    shared_sig = data.shared_sig;
    callbacks_.emplace(path, std::move(data));
  }
  // Registration takes the lock of its own, the callback is known by now
  apply_hotkey(path);
  return shared_sig;
}

///
///
auto workflow_hotkey::apply_hotkey(const path_type& path) -> void
{
  const auto lock = std::scoped_lock{mtx_};
  const auto it = callbacks_.find(path);
  if(it == std::cend(callbacks_))
  {
    return;
  }
  auto& data = it->second;
  const auto combination = std::pair{data.key_setting->value(), data.modifier_setting->value()};
  if(data.registered != combination)
  {
    if(data.registered)
    {
      system::hotkey::unregister_callback(data.registered->first, data.registered->second);
    }
    // The system holds a combination only once, a registration left over from another path is taken over
    system::hotkey::unregister_callback(combination.first, combination.second);
    system::hotkey::register_callback(
      combination.first,
      combination.second,
      [sig = std::weak_ptr{data.shared_sig}]()
      {
        framework::thread_pool::queue_task(
          [sig]()
          {
            if(auto s = sig.lock())
            {
              (*s)();
            }
          }
        );
      }
    );
    data.registered = combination;
  }
}

} // namespace bibstd::workflow
