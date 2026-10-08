#include "bibstd/workflow/workflow_scripture.hpp"
#include "bibstd/bible/versification.hpp"
#include "bibstd/core/core_scripture_store.hpp"
#include "bibstd/util/contains.hpp"
#include "bibstd/util/enum.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/log.hpp"
#include "bibstd/util/visit_helper.hpp"
#include "bibstd/workflow/workflow_settings.hpp"
#include "bibstd/workflow/workflow_settings_base.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <map>
#include <memory>
#include <ranges>
#include <tuple>
#include <utility>
#include <vector>

namespace bibstd::workflow
{
namespace
{

///
/// \return the list validator of \p setting
///
[[nodiscard]] auto list_validator(const auto& setting)
{
  return std::get<framework::setting_validator_list<std::optional<std::string>>::sptr_type>(setting->validator);
}

} // namespace

///
///
workflow_scripture_settings::workflow_scripture_settings(std::shared_ptr<workflow_settings> workflow_settings)
  : workflow_settings_base{std::move(workflow_settings)}
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
workflow_scripture::workflow_scripture(
  std::shared_ptr<workflow_settings> workflow_settings, std::shared_ptr<workflow_script> workflow_script
)
  : workflow_base{std::move(workflow_settings)}
  , thread_pool_guard_{framework::thread_pool::init()}
  , workflow_script_{std::move(workflow_script)}
  , core_scripture_store_(std::make_unique<core::core_scripture_store>(settings().scripture_folder->value()))
{
  update_scripture_name_setting();
  // Connected last: scripts_loaded comes once all workflows are constructed, and with every "Load scripts"
  workflow_script_->connect_queued(&workflow_script_sigs::scripts_loaded, [this]() { update_scripts(); }, executor_);
}

///
///
workflow_scripture::~workflow_scripture() noexcept
{
  // No slot reaches the workflow once disconnected, a request on the way ends with its script
  executor_.disconnect();
}

///
///
auto workflow_scripture::scripture_count() const -> std::size_t
{
  const auto lock = std::scoped_lock{mtx_};
  return scripture_names().size();
}

///
///
auto workflow_scripture::information(const scripture_params& params) const -> std::optional<bible::scripture_info>
{
  try
  {
    if(const auto scripture = stored(params->scripture_name))
    {
      return scripture->information();
    }
    const auto name = selected_name(params->scripture_name);
    const auto output = name ? run_script<information_manifest>({*name}) : std::nullopt;
    if(!output)
    {
      return std::nullopt;
    }
    return bible::scripture_info{
      .name = *name,
      .abbreviation = output->get<"abbreviation">().value_or(""),
      .language = output->get<"language">().value_or(""),
      .copyright = output->get<"copyright">(),
    };
  }
  catch(...)
  {
    LOG_ERROR("exception occurred: {}", util::exception_report());
    return std::nullopt;
  }
}

///
///
auto workflow_scripture::book_information(const scripture_params& params, const bible::book_id book) const
  -> std::optional<bible::book_name>
{
  try
  {
    if(const auto scripture = stored(params->scripture_name))
    {
      return scripture->book_information(book);
    }
    const auto name = selected_name(params->scripture_name);
    const auto output = name ? run_script<book_manifest>({*name, std::string{util::enum_name(book)}}) : std::nullopt;
    if(!output || std::apply([](const auto&... names) { return !(names || ...); }, output->values()))
    {
      return std::nullopt;
    }
    return bible::book_name{
      .abbreviation = output->get<"abbreviation">().value_or(""),
      .short_name = output->get<"short_name">().value_or(""),
      .long_name = output->get<"long_name">().value_or(""),
    };
  }
  catch(...)
  {
    LOG_ERROR("exception occurred: {}", util::exception_report());
    return std::nullopt;
  }
}

///
///
auto workflow_scripture::versification_or_fallback(const scripture_params& params) const -> versification_wrapper_type
{
  if(auto scripture = stored(params->scripture_name))
  {
    return versification_wrapper_type{std::move(scripture)};
  }
  return versification_wrapper_type{default_versifications.at(settings().fallback_versification->value())};
}

///
///
auto workflow_scripture::passage(const passage_params& params) const -> passage_result
{
  static constexpr auto verse_not_found = "verse not found";
  static constexpr auto script_failed = "script failed";
  try
  {
    const auto& ref = params->reference;
    if(const auto scripture = stored(params->scripture_name))
    {
      if(auto passage = scripture->passage(ref))
      {
        return passage_result_t{.passage = std::move(*passage)};
      }
      LOG_WARN("passage not found: reference=\"{}\"", ref);
      return std::unexpected{std::string{verse_not_found}};
    }
    const auto name = selected_name(params->scripture_name);
    const auto output = name ? run_script<passage_manifest>({
                                 *name,
                                 std::string{util::enum_name(ref.book())},
                                 static_cast<std::int64_t>(ref.chapter().value),
                                 static_cast<std::int64_t>(ref.verse().value),
                               })
                             : std::nullopt;
    const auto content = output ? output->get<"text">() : std::nullopt;
    if(!content)
    {
      // What the script reports, e.g. a web page it could not fetch
      const auto error = output ? output->get<"error">() : std::string{script_failed};
      return std::unexpected{error.value_or(std::string{verse_not_found})};
    }
    using markup = bible::passage_markup;
    return passage_result_t{
      .passage = {.ref = ref, .content = markup::section(markup::paragraph_undefined, markup::escaped_xml(*content))}
    };
  }
  catch(...)
  {
    auto report = util::exception_report();
    LOG_ERROR("exception occurred: {}", report);
    return std::unexpected{std::move(report)};
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
auto workflow_scripture::scripture_names() const -> std::vector<std::string>
{
  decltype(auto) scriptures = core_scripture_store_->scriptures();
  auto names = scriptures | std::views::keys | std::ranges::to<std::vector>();
  if(script_scriptures_)
  {
    // The folder's, \see stored
    names.append_range(
      *script_scriptures_ | std::views::keys | std::views::filter([&](const auto& n) { return !scriptures.contains(n); })
    );
  }
  return names;
}

///
///
auto workflow_scripture::selected_name(const std::optional<std::string>& name) const -> std::optional<std::string>
{
  return name ? name : settings().scripture_name->value();
}

///
///
auto workflow_scripture::stored(const std::optional<std::string>& name) const -> std::shared_ptr<bible::scripture>
{
  const auto selected = selected_name(name);
  const auto lock = std::scoped_lock{mtx_};
  decltype(auto) scriptures = core_scripture_store_->scriptures();
  const auto it = selected ? scriptures.find(*selected) : std::ranges::end(scriptures);
  return it != std::ranges::end(scriptures) ? it->second : nullptr;
}

///
///
template<script_manifest M>
auto workflow_scripture::run_script(typename M::input input) const -> std::optional<typename M::output>
{
  auto& name = input.template get<"name">();
  // Copied, so the lock is not held while the script runs
  const auto scripture = [&]() -> std::optional<script_scripture_t>
  {
    const auto lock = std::scoped_lock{mtx_};
    if(!script_scriptures_)
    {
      return std::nullopt;
    }
    const auto it = script_scriptures_->find(name);
    return it != script_scriptures_->cend() ? std::optional{it->second} : std::nullopt;
  }();
  if(!scripture)
  {
    return std::nullopt;
  }
  // The script knows the scripture by its own name
  name = scripture->name;
  return workflow_script_->run<M>(scripture->script, input);
}

///
///
auto workflow_scripture::update_scripts() -> void
{
  // The name in the script, then the name of the script, e.g. "LUT (Bibleserver)"
  static constexpr auto script_scripture_name = "{} ({})";
  try
  {
    // The scriptures of a script by their names in the app
    const auto of_script = [this](const workflow_script::scripts_type::value_type& script)
    {
      const auto output = workflow_script_->run<names_manifest>(script.first, {});
      const auto names = output ? output->get<"names">() : std::vector<std::string>{};
      return names |
             std::views::transform(
               [&](const auto& name)
               {
                 return std::pair{
                   std::format(script_scripture_name, name, script.first), script_scripture_t{script.first, name}
                 };
               }
             ) |
             std::ranges::to<std::vector>();
    };
    const auto scripts = workflow_script_->scripts<names_manifest, information_manifest, book_manifest, passage_manifest>();
    auto scriptures = scripts | std::views::transform(of_script) | std::views::join |
                      std::ranges::to<std::map<std::string, script_scripture_t>>();
    {
      const auto lock = std::scoped_lock{mtx_};
      script_scriptures_ = std::move(scriptures);
    }
    update_scripture_name_setting();
  }
  catch(...)
  {
    LOG_ERROR("exception occurred: {}", util::exception_report());
  }
  notify(&signals_type::scriptures_changed);
}

///
///
auto workflow_scripture::update_scripture_name_setting() -> void
{
  static constexpr auto has_kjv_versification = [](const auto& s)
  { return s.second->versification() == bible::versification_kjv; };

  // Read under the lock, the setting is changed without it: its listeners may call back into the workflow
  auto names = std::vector<std::string>{};
  auto preferred = std::optional<std::string>{};
  auto scripts_loaded = false;
  {
    const auto lock = std::scoped_lock{mtx_};
    decltype(auto) scriptures = core_scripture_store_->scriptures();
    names = scripture_names();
    scripts_loaded = script_scriptures_.has_value();
    if(const auto it = std::ranges::find_if(scriptures, has_kjv_versification); it != std::ranges::cend(scriptures))
    {
      preferred = it->first;
    }
  }
  const auto name = settings().scripture_name->value();
  if(!scripts_loaded && name)
  {
    // Until the scripts are loaded, the name may be one of theirs.
    // If not here, validator would drop the existing value.
    names.push_back(*name);
  }
  std::ignore = list_validator(settings().scripture_name)->available(names);

  // A setting naming a scripture that is not loaded leaves every lookup without one. That is what
  // a name left over from scriptures that are gone does, so it is pointed at a loaded scripture.
  const auto name_contained = name.has_value() && util::contains(names, *name);
  if(!(name_contained || names.empty()))
  {
    settings().scripture_name->value(preferred ? *preferred : names.front());
  }
}

} // namespace bibstd::workflow
