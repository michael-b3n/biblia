#pragma once

#include "bibstd/framework/setting.hpp"
#include "bibstd/framework/setting_common.hpp"
#include "bibstd/lua/function_cast.hpp"
#include "bibstd/lua/names.hpp"
#include "bibstd/lua/registration.hpp"
#include "bibstd/lua/sol.hpp"
#include "bibstd/util/path.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string_view>
#include <utility>
#include <variant>

namespace bibstd::lua
{

// Forward declarations
class state_owner;

namespace detail
{

///
/// Data of a Lua state, shared by its owners and locks.
///
struct state_data final
{
  std::recursive_mutex mtx;
  std::atomic<bool> shutdown_flag{false};
  sol::state lua;
  sol::table interface; // root.interface, what scripts see
  sol::table sandbox;   // its read-only view, the globals scripts run on
};

} // namespace detail

///
/// Locked Lua state, handed out by state_owner::lock. The lock is held as long as this object lives.
/// Lua references taken from it (tables, functions, objects) must not outlive it.
///
class state final
{
  // Friends
  friend class state_owner;

  // Variables
  std::shared_ptr<detail::state_data> data_;
  std::unique_lock<std::recursive_mutex> lock_;

private: // Structors
  explicit state(std::shared_ptr<detail::state_data> data);

public: // Structors
  state(const state&) = delete;
  state(state&&) noexcept = default;
  auto operator=(const state&) -> state& = delete;
  auto operator=(state&&) noexcept -> state& = delete;
  ~state() noexcept = default;

public: // Accessors
  [[nodiscard]] auto operator*() const -> sol::state&;
  [[nodiscard]] auto operator->() const -> sol::state*;

public: // Operations
  ///
  /// Register a function at \p p, the last section is its name, for as long as the registration is kept.
  /// What the function reaches must live as long, \see registration. \p function takes and returns C++
  /// values, what a script passes and gets is converted, \see function_cast.
  /// \return the registration, one of nothing if \p p can not be registered
  ///
  template<function_castable F>
  [[nodiscard]] auto register_function(const util::path& p, F&& function) -> registration;

  ///
  /// Register \p setting at \p p, as table with the functions get(), set(value) and postfix(), for as long as the
  /// registration is kept. \p setting must live as long.
  /// \return the registration, one of nothing if the path can not be registered
  ///
  template<framework::underlying_setting_type T>
  [[nodiscard]] auto register_setting(const util::path& p, framework::setting<T>& setting) -> registration;

  ///
  /// Run a user script in an environment of its own, so its globals do not reach other scripts.
  /// Text only, Lua does not verify bytecode and a damaged one crashes past any error handling.
  /// \return what the script returns, nil if nothing, or std::nullopt if it failed
  ///
  [[nodiscard]] auto run_script(std::string_view name, std::string_view code) -> std::optional<sol::object>;

  ///
  /// Call \p function with \p args.
  /// \return its first result, nil if none, or std::nullopt if it failed
  ///
  template<typename... Args>
  [[nodiscard]] auto call(const sol::protected_function& function, Args&&... args) -> std::optional<sol::object>;

private: // Implementation
  [[nodiscard]] auto free_parent(const util::path& p) -> std::optional<sol::table>;
  [[nodiscard]] auto run(const std::function<sol::protected_function_result()>& code) -> std::optional<sol::object>;
};

///
///
template<function_castable F>
auto state::register_function(const util::path& p, F&& function) -> registration
{
  auto parent = free_parent(p);
  if(!parent)
  {
    return {};
  }
  auto registered = std::make_shared<const std::monostate>();
  parent->set_function(p.sections().back(), function_cast(std::forward<F>(function), registered));
  return registration{data_, p, std::move(registered)};
}

///
///
template<framework::underlying_setting_type T>
auto state::register_setting(const util::path& p, framework::setting<T>& setting) -> registration
{
  using erased_type = framework::setting_type_erased_type_from<T>;

  auto parent = free_parent(p);
  if(!parent)
  {
    return {};
  }
  auto registered = std::make_shared<const std::monostate>();
  auto node = parent->create_named(p.sections().back());
  node.set_function(
    names::function_get,
    function_cast(
      [&setting]() -> erased_type { return framework::create_setting_value_converter<T, erased_type>()(setting.value()); },
      registered
    )
  );
  node.set_function(
    names::function_set,
    function_cast(
      [&setting](const erased_type& value) -> bool
      { return setting.value(framework::create_setting_value_converter<erased_type, T>()(value)); },
      registered
    )
  );
  node.set_function(names::function_postfix, function_cast([&setting]() { return setting.postfix; }, registered));
  return registration{data_, p, std::move(registered)};
}

///
///
template<typename... Args>
auto state::call(const sol::protected_function& function, Args&&... args) -> std::optional<sol::object>
{
  return run([&] { return function(std::forward<Args>(args)...); });
}

} // namespace bibstd::lua
