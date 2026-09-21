#pragma once

#include "bibstd/framework/property_tree.hpp"
#include "bibstd/framework/setting_type_erased.hpp"
#include "bibstd/meta/for_each.hpp"
#include "bibstd/meta/type_traits.hpp"
#include "bibstd/signal/adapter.hpp"
#include "bibstd/util/contains.hpp"
#include "bibstd/util/enum.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/non_owning_ptr.hpp"

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

namespace bibstd::workflow
{
namespace detail
{

///
/// Conditionally creates the default validator of a new setting.
/// For enum values, optional or list enums, a list validator is created
/// \return default validator types
///
template<framework::underlying_setting_type T>
auto conditional_default_validator() -> framework::setting_validator<T>
{
  // Unpacks a type from its wrapper. std::vector<T> and std::optional<T>
  // will return T, which is checked for enum type.
  if constexpr(std::is_enum_v<meta::remove_wrapper_t<T>>)
  {
    using enum_type = meta::remove_wrapper_t<T>;
    return std::make_shared<framework::setting_validator_list<T>>(util::enum_values<enum_type>());
  }
  else
  {
    return std::make_shared<framework::setting_validator_unbound>();
  }
}

} // namespace detail

///
/// Struct containing signals for workflow settings class.
///
struct workflow_settings_signals final
{
  /// Setting created signal: will be emitted when a new setting was created.
  /// The path of the newly created setting is provided as argument.
  signal::signal_type<void(std::string)> setting_created;
};

///
/// Workflow setting. This class owns setting objects.
/// Since this workflow owns the settings, it is required
/// to live as long as the settings are used.
///
class workflow_settings final : public signal::adapter<workflow_settings_signals>
{
  // Typedefs
  template<framework::underlying_setting_type_erased_type T>
  using setting_type_erased_non_owning_ptr_type = util::non_owning_ptr<framework::setting_type_erased<T>>;
  template<framework::underlying_setting_type_erased_type T>
  using setting_type_erased_uptr_type = std::unique_ptr<framework::setting_type_erased<T>>;
  using setting_type_erased_uptr_variant_type =
    meta::for_each_t<framework::setting_type_erased_variant, setting_type_erased_uptr_type>;

  struct setting_uptr_data final
  {
    std::string path;
    setting_type_erased_uptr_variant_type setting;
  };

  // Variables
  const std::filesystem::path data_folder_;
  const framework::property_tree::sptr_type tree_;
  mutable std::mutex mtx_;
  std::vector<setting_uptr_data> settings_;

public: // Typedefs
  using setting_type_erased_non_owning_ptr_variant_type =
    meta::for_each_t<framework::setting_type_erased_variant, setting_type_erased_non_owning_ptr_type>;

  ///
  /// Settings data struct.
  ///
  struct setting_data final
  {
    std::string path;
    setting_type_erased_non_owning_ptr_variant_type setting;
  };

  template<framework::underlying_setting_type T>
  using setting_non_owning_ptr_type = util::non_owning_ptr<framework::setting<T>>;

public: // Constants
  static constexpr std::string_view settings_file_name = "settings.xml";
  static constexpr std::string_view settings_root_name = "settings";
  static constexpr char path_separator = '.';

public: // Static interface
  ///
  /// Get the settings file path inside the local data folder \p folder_name, see system::filesystem::local_data_folder.
  /// \return settings file path
  ///
  [[nodiscard]] static auto settings_file_path(std::optional<std::string_view> folder_name = std::nullopt)
    -> std::filesystem::path;

  ///
  /// Split a setting path into the segments it is made of.
  /// \return segments in the order they are written in, empty segments are left out
  ///
  [[nodiscard]] static auto split_path(std::string_view path) -> std::vector<std::string>;

public: // Accessors
  ///
  /// Get the local data folder the settings are stored in, see system::filesystem::local_data_folder.
  /// \return local data folder
  ///
  [[nodiscard]] auto data_folder() const -> const std::filesystem::path&;

  ///
  /// Access all created settings.
  /// \return list of all created settings
  ///
  [[nodiscard]] auto type_erased_settings() const -> std::vector<setting_data>;

  ///
  /// Access a type erased setting for the specified path.
  /// \return type erased setting or nullopt if no setting with the specified path exists
  ///
  [[nodiscard]] auto type_erased_setting(const std::string& path) const
    -> std::optional<setting_type_erased_non_owning_ptr_variant_type>;

public: // Structors
  ///
  /// Construct the settings, stored in the local data folder \p folder_name.
  ///
  explicit workflow_settings(std::optional<std::string_view> folder_name = std::nullopt);

public: // Modifiers
  ///
  /// Create a new setting with a value type that is only known at runtime, or access the
  /// already existing setting of the specified path. The created setting is unbound, it will
  /// be owned by this workflow.
  /// \note This is intended for settings that are declared outside of the backend, e.g. by the frontend.
  /// \throws util::exception if a setting of the specified path exists with a different value type
  /// \return the newly created or the already existing setting
  ///
  template<framework::underlying_setting_type_erased_type T>
  [[nodiscard]] auto type_erased_setting(
    const std::string& path,
    T default_value,
    std::string postfix = {},
    framework::setting_validator<T> validator = detail::conditional_default_validator<T>()
  ) -> setting_type_erased_non_owning_ptr_variant_type;

  ///
  /// Create or access a setting with no postfix, naming only the validator. \see type_erased_setting
  /// \return the newly created or the already existing setting
  ///
  template<framework::underlying_setting_type_erased_type T>
  [[nodiscard]] auto type_erased_setting(const std::string& path, T default_value, framework::setting_validator<T> validator)
    -> setting_type_erased_non_owning_ptr_variant_type;

  ///
  /// Create a new setting. The setting will be owned by this workflow and has static lifetime.
  /// \p postfix is displayed behind the value, e.g. the unit it is measured in.
  /// \return non owning pointer to the newly created setting
  ///
  template<framework::underlying_setting_type T>
  [[nodiscard]] auto create_setting(
    const std::string& path,
    T default_value,
    std::string postfix = {},
    framework::setting_validator<T> validator = detail::conditional_default_validator<T>()
  ) -> setting_non_owning_ptr_type<T>;

  ///
  /// Create a new setting with no postfix, naming only the validator. \see create_setting
  /// \return non owning pointer to the newly created setting
  ///
  template<framework::underlying_setting_type T>
  [[nodiscard]] auto create_setting(const std::string& path, T default_value, framework::setting_validator<T> validator)
    -> setting_non_owning_ptr_type<T>;
};

///
///
template<framework::underlying_setting_type_erased_type T>
auto workflow_settings::type_erased_setting(
  const std::string& path, T default_value, std::string postfix, framework::setting_validator<T> validator
) -> setting_type_erased_non_owning_ptr_variant_type
{
  auto lock = std::unique_lock{mtx_};
  auto it = std::ranges::find_if(settings_, [&path](const auto& data) { return data.path == path; });
  const auto setting_exists = it != std::ranges::cend(settings_);
  if(!setting_exists)
  {
    const auto setting = std::make_shared<framework::setting<T>>(
      path,
      std::move(tree_->create_property(
        framework::property_tree::path_type{std::string(settings_root_name)} / framework::property_tree::path_type{path},
        std::move(default_value)
      )),
      std::move(postfix),
      std::move(validator)
    );
    using underlying_setting_type_erased_type = framework::setting_type_erased<framework::setting_type_erased_type_from<T>>;
    settings_.emplace_back(
      setting_uptr_data{.path = path, .setting = std::make_unique<underlying_setting_type_erased_type>(setting)}
    );
    it = std::prev(settings_.end());
  }
  const auto setting_ptr =
    std::visit([](const auto& e) -> setting_type_erased_non_owning_ptr_variant_type { return e.get(); }, it->setting);
  lock.unlock();

  // If the setting already exists, check if the value type is the same as the requested type.
  if(setting_exists)
  {
    const auto is_same_value_type = std::visit(
      [](const auto setting)
      {
        using value_type = std::remove_pointer_t<std::remove_cvref_t<decltype(setting)>>::value_type;
        return std::is_same_v<value_type, std::remove_cvref_t<T>>;
      },
      setting_ptr
    );
    if(!is_same_value_type)
    {
      throw util::exception{std::format("setting exists with different value type: path=\"{}\"", path)};
    }
  }
  else
  {
    notify(&workflow_settings_signals::setting_created, path);
  }
  return setting_ptr;
}

///
///
template<framework::underlying_setting_type_erased_type T>
auto workflow_settings::type_erased_setting(const std::string& path, T default_value, framework::setting_validator<T> validator)
  -> setting_type_erased_non_owning_ptr_variant_type
{
  return type_erased_setting<T>(path, std::move(default_value), {}, std::move(validator));
}

///
///
template<framework::underlying_setting_type T>
auto workflow_settings::create_setting(
  const std::string& path, T default_value, std::string postfix, framework::setting_validator<T> validator
) -> setting_non_owning_ptr_type<T>
{
  const auto setting = std::make_shared<framework::setting<T>>(
    path,
    std::move(tree_->create_property(
      framework::property_tree::path_type{std::string(settings_root_name)} / framework::property_tree::path_type{path},
      std::move(default_value)
    )),
    std::move(postfix),
    std::move(validator)
  );
  const auto setting_ptr = setting.get();
  using underlying_setting_type_erased_type = framework::setting_type_erased<framework::setting_type_erased_type_from<T>>;
  {
    const auto lock = std::scoped_lock{mtx_};
    const auto contains_path = util::contains(settings_, [&path](const auto& data) { return data.path == path; });
    if(contains_path)
    {
      throw util::exception(std::format("setting already created: path=\"{}\"", path));
    }
    settings_.emplace_back(
      setting_uptr_data{.path = path, .setting = std::make_unique<underlying_setting_type_erased_type>(setting)}
    );
  }
  notify(&workflow_settings_signals::setting_created, path);
  return setting_ptr;
}

///
///
template<framework::underlying_setting_type T>
auto workflow_settings::create_setting(const std::string& path, T default_value, framework::setting_validator<T> validator)
  -> setting_non_owning_ptr_type<T>
{
  return create_setting<T>(path, std::move(default_value), {}, std::move(validator));
}

} // namespace bibstd::workflow
