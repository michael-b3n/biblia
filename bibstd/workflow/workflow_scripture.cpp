#include "bibstd/workflow/workflow_scripture.hpp"
#include "bibstd/bible/versification.hpp"
#include "bibstd/core/core_scripture_store.hpp"
#include "bibstd/framework/settings_base.hpp"
#include "bibstd/util/contains.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/log.hpp"
#include "bibstd/util/visit_helper.hpp"
#include "bibstd/workflow/workflow_settings.hpp"

#include <cstddef>
#include <memory>

namespace bibstd::workflow
{

///
///
workflow_scripture_settings::workflow_scripture_settings(std::shared_ptr<workflow_settings> workflow_settings)
  : framework::settings_base{std::move(workflow_settings)}
  , scripture_name{workflow_settings_->create_setting(
      "scripture.name",
      setting_value_t<decltype(scripture_name)>{},
      std::make_shared<framework::setting_validator_list<setting_value_t<decltype(scripture_name)>>>()
    )}
  , scripture_folder{workflow_settings_->create_setting(
      "scripture.folder", workflow_settings_->data_folder() / default_folder_name
    )}
  , fallback_versification{workflow_settings_->create_setting(
      "scripture.fallback_versification_name",
      std::string{bible::versification::default_kjv::name},
      std::make_shared<framework::setting_validator_list<setting_value_t<decltype(fallback_versification)>>>(
        workflow_scripture::default_versifications | std::views::keys |
        std::views::transform([](const auto& n) { return std::string{n}; }) | std::ranges::to<std::vector>()
      )
    )}
{
  // The default value set above must be one of the default versifications, else it would be invalid.
  static_assert(meta::contains_v<bible::versification::all_defaults_variant, bible::versification::default_kjv>);
}

///
///
workflow_scripture::versification_wrapper::versification_wrapper(std::shared_ptr<bible::scripture> scripture)
  : data_{std::move(scripture)}
{
}

///
///
workflow_scripture::versification_wrapper::versification_wrapper(bible::versification versification)
  : data_{std::move(versification)}
{
}

///
///
auto workflow_scripture::versification_wrapper::get() const -> const bible::versification&
{
  return util::visit_lambdas(
    data_,
    [](const bible::versification& versification) -> const bible::versification& { return versification; },
    [](const std::shared_ptr<bible::scripture>& scripture) -> const bible::versification& { return scripture->versification(); }
  );
}

///
///
workflow_scripture::workflow_scripture(std::shared_ptr<workflow_settings> workflow_settings)
  : workflow_base{std::move(workflow_settings)}
  , thread_pool_guard_{framework::thread_pool::init()}
  , core_scripture_store_(std::make_unique<core::core_scripture_store>(settings().scripture_folder->value()))
{
  update_scripture_name_setting();
}

///
///
workflow_scripture::~workflow_scripture() noexcept = default;

///
///
auto workflow_scripture::scripture_count() const -> std::size_t
{
  const auto lock = std::scoped_lock{mtx_};
  return core_scripture_store_->scriptures().size();
}

///
///
auto workflow_scripture::scripture(const scripture_params& params) const -> scripture_result
{
  try
  {
    const auto lock = std::scoped_lock{mtx_};
    decltype(auto) scriptures = core_scripture_store_->scriptures();
    if(scriptures.empty())
    {
      // no scriptures loaded, no warning since this is a valid state
      return scripture_result{return_failure};
    }
    const auto scripture_name = params->scripture_name ? params->scripture_name : settings().scripture_name->value();
    auto result = scripture_result{return_failure};
    if(scripture_name)
    {
      if(const auto it = scriptures.find(*scripture_name); it != std::ranges::cend(scriptures))
      {
        result = scripture_result::value_type{.name = it->first, .scripture = it->second};
      }
      else
      {
        LOG_WARN("scripture name not found: \"{}\"", *scripture_name);
      }
    }
    else
    {
      LOG_WARN("scripture name not set: using first scripture in store: \"{}\"", scriptures.begin()->first);
      result = scripture_result::value_type{.name = scriptures.begin()->first, .scripture = scriptures.begin()->second};
    }
    return result;
  }
  catch(...)
  {
    LOG_ERROR("exception occurred: {}", util::exception_report());
    return return_failure;
  }
}

///
///
auto workflow_scripture::versification_or_fallback(const scripture_params& params) const -> versification_wrapper_type
{
  if(const auto result = scripture(params))
  {
    return versification_wrapper_type{result.value().scripture};
  }
  return versification_wrapper_type{default_versifications.at(settings().fallback_versification->value())};
}

///
///
auto workflow_scripture::passage(const passage_params& params) const -> passage_result
{
  try
  {
    const auto lock = std::scoped_lock{mtx_};
    decltype(auto) scriptures = core_scripture_store_->scriptures();
    if(scriptures.empty())
    {
      // no scriptures loaded, no warning since this is a valid state
      return passage_result{return_failure};
    }
    const auto scripture_name = params->scripture_name ? params->scripture_name : settings().scripture_name->value();
    auto result = passage_result{return_failure};
    if(scripture_name)
    {
      if(const auto it = scriptures.find(*scripture_name); it != std::ranges::cend(scriptures))
      {
        if(const auto passage_result = it->second->passage(params->reference))
        {
          result = passage_result::value_type{.passage = *passage_result};
        }
        else
        {
          LOG_WARN("passage not found: reference=\"{}\"", params->reference);
        }
      }
      else
      {
        LOG_WARN("scripture name not found: \"{}\"", *scripture_name);
      }
    }
    else
    {
      LOG_WARN("scripture name not set");
    }
    return result;
  }
  catch(...)
  {
    LOG_ERROR("exception occurred: {}", util::exception_report());
    return passage_result{return_failure};
  }
}

///
///
auto workflow_scripture::import_scriptures(const import_params& params) -> void
{
  try
  {
    framework::thread_pool::queue_task(
      [this, params]()
      {
        auto imported = std::size_t{0};
        try
        {
          {
            const auto lock = std::scoped_lock{mtx_};
            imported = core_scripture_store_->import(params->folder);
          }
          if(imported != 0)
          {
            update_scripture_name_setting();
          }
        }
        catch(...)
        {
          LOG_ERROR("exception occurred: {}", util::exception_report());
        }
        notify(&signals_type::import_ended, params.process_id(), imported);
      },
      strand_id_
    );
  }
  catch(...)
  {
    LOG_ERROR("exception occurred: {}", util::exception_report());
    notify(&signals_type::import_ended, params.process_id(), std::size_t{0});
  }
}

///
///
auto workflow_scripture::update_scripture_name_setting() -> void
{
  const auto lock = std::scoped_lock{mtx_};
  decltype(auto) scriptures = core_scripture_store_->scriptures();
  const auto scripture_names = scriptures | std::views::keys | std::ranges::to<std::vector>();
  decltype(auto) scripture_name_validator =
    std::get<framework::setting_validator_list<std::optional<std::string>>::sptr_type>(settings().scripture_name->validator);
  std::ignore = scripture_name_validator->available(scripture_names);

  // A setting naming a scripture that is not loaded leaves every lookup without one. That is what
  // a name left over from scriptures that are gone does, so it is pointed at a loaded scripture.
  const auto name = settings().scripture_name->value();
  const auto name_contained = name.has_value() && util::contains(scripture_names, *name);

  if(!(name_contained || scripture_names.empty()))
  {
    static constexpr auto has_kjv_versification = [](const auto& s)
    { return s.second->versification() == bible::versification_kjv; };

    if(const auto it = std::ranges::find_if(scriptures, has_kjv_versification); it != std::ranges::cend(scriptures))
    {
      settings().scripture_name->value(it->first);
    }
    else
    {
      settings().scripture_name->value(scripture_names.front());
    }
  }
}

} // namespace bibstd::workflow
