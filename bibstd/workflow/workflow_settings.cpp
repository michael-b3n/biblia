#include "bibstd/workflow/workflow_settings.hpp"
#include "bibstd/system/filesystem.hpp"

#include <algorithm>
#include <ranges>

namespace bibstd::workflow
{

///
///
auto workflow_settings::settings_file_path(const std::optional<std::string_view> folder_name) -> std::filesystem::path
{
  return system::filesystem::local_data_folder(folder_name) / settings_file_name;
}

///
///
workflow_settings::workflow_settings(const std::optional<std::string_view> folder_name)
  : data_folder_{system::filesystem::local_data_folder(folder_name)}
  , tree_{framework::property_tree::create(data_folder_ / settings_file_name)}
{
}

///
///
auto workflow_settings::data_folder() const -> const std::filesystem::path&
{
  return data_folder_;
}

///
///
auto workflow_settings::type_erased_settings() const -> std::vector<setting_data>
{
  const auto lock = std::scoped_lock{mtx_};
  auto retval = std::vector<setting_data>(settings_.size());
  const auto to_setting_data = [](const auto& data)
  {
    return setting_data{
      data.path,
      std::visit([](const auto& e) -> setting_type_erased_non_owning_ptr_variant_type { return e.get(); }, data.setting)
    };
  };
  for(const auto [i, d] : settings_ | std::views::transform(to_setting_data) | std::views::enumerate)
  {
    retval.at(i) = d;
  }
  return retval;
}

///
///
auto workflow_settings::type_erased_setting(const std::string& path) const
  -> std::optional<setting_type_erased_non_owning_ptr_variant_type>
{
  const auto lock = std::scoped_lock{mtx_};
  const auto it = std::ranges::find_if(settings_, [&path](const auto& data) { return data.path == path; });
  if(it != std::ranges::cend(settings_))
  {
    return std::visit([](const auto& e) -> setting_type_erased_non_owning_ptr_variant_type { return e.get(); }, it->setting);
  }
  return std::nullopt;
}

} // namespace bibstd::workflow
