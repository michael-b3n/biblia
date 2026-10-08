#include "bibstd/workflow/workflow_bible_ref_lookup.hpp"
#include "bibstd/bible/versification.hpp"
#include "bibstd/system/locale.hpp"
#include "bibstd/system/open_browser.hpp"
#include "bibstd/util/contains.hpp"
#include "bibstd/util/enum.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/log.hpp"
#include "bibstd/util/url.hpp"

#include <algorithm>
#include <ranges>
#include <tuple>
#include <utility>

namespace bibstd::workflow
{
namespace
{

///
/// \return the list validator of \p setting
///
template<framework::underlying_setting_type T>
[[nodiscard]] auto list_validator(const workflow_settings::setting_non_owning_ptr_type<T>& setting)
{
  return std::get<typename framework::setting_validator_list<T>::sptr_type>(setting->validator);
}

} // namespace

///
///
workflow_bible_ref_lookup_settings::workflow_bible_ref_lookup_settings(std::shared_ptr<workflow_settings> workflow_settings)
  : workflow_settings_base{std::move(workflow_settings)}
  , script{workflow_settings_->create_setting(
      "lookup.script", std::string{}, std::make_shared<framework::setting_validator_list<setting_value_t<decltype(script)>>>()
    )}
  , translations{workflow_settings_->create_setting(
      "lookup.translations",
      setting_value_t<decltype(translations)>{},
      std::make_shared<framework::setting_validator_list<setting_value_t<decltype(translations)>>>()
    )}
{
}

///
///
workflow_bible_ref_lookup::workflow_bible_ref_lookup(
  std::shared_ptr<workflow_settings> workflow_settings, std::shared_ptr<workflow_script> workflow_script
)
  : workflow_base{std::move(workflow_settings)}
  , thread_pool_guard_{framework::thread_pool::init()}
  , workflow_script_{std::move(workflow_script)}
{
  // Connected last: scripts_loaded comes once all workflows are constructed, and with every "Load scripts"
  settings().script->connect_queued(&framework::setting_signals::value_changed, [this]() { update_translations(); }, executor_);
  workflow_script_->connect_queued(&workflow_script_sigs::scripts_loaded, [this]() { update_scripts(); }, executor_);
}

///
///
workflow_bible_ref_lookup::~workflow_bible_ref_lookup() noexcept
{
  // No slot reaches the workflow once disconnected
  executor_.disconnect();
}

///
///
auto workflow_bible_ref_lookup::lookup(const params& params) -> void
{
  try
  {
    framework::thread_pool::queue_task(
      [this, params]()
      {
        try
        {
          const auto open = [](const std::string& url)
          {
            if(!system::open_browser::open(url))
            {
              LOG_WARN("failed to open the browser: url=\"{}\"", url);
            }
          };
          std::ranges::for_each(
            params->references, [&](const auto& reference_range) { std::ranges::for_each(urls(reference_range), open); }
          );
        }
        catch(...)
        {
          LOG_ERROR("exception occurred: {}", util::exception_report());
        }
        notify(&signals_type::ended, params.process_id());
      },
      strand_id_
    );
  }
  catch(...)
  {
    LOG_ERROR("exception occurred: {}", util::exception_report());
    notify(&signals_type::ended, params.process_id());
  }
}

///
///
auto workflow_bible_ref_lookup::urls(const bible::reference_range& range) const -> std::vector<std::string>
{
  const auto script = settings().script->value();
  const auto same_book = range.begin().book() == range.end().book();
  if(!same_book)
  {
    LOG_WARN("lookup supports one book only: {}", range);
  }
  // Web pages count chapters and verses like the KJV
  const auto chapters = bible::versification_kjv.split_by_chapter(same_book ? range : bible::reference_range{range.begin()});
  const auto translations = settings().translations->value();
  const auto to_url = [&](const bible::reference_range& chapter) -> std::optional<std::string>
  {
    const auto begin = chapter.begin();
    const auto output = workflow_script_->run<url_manifest>(
      script,
      {
        translations,
        std::string{util::enum_name(begin.book())},
        static_cast<std::int64_t>(begin.chapter().value),
        static_cast<std::int64_t>(begin.verse().value),
        static_cast<std::int64_t>(chapter.end().verse().value),
      }
    );
    auto url = output ? output->get<"url">() : std::nullopt;
    // A script built it, anything but a web page could start a program
    if(!url || !util::url::is_web_url(*url))
    {
      LOG_WARN("lookup without the url of a web page: script=\"{}\", url=\"{}\"", script, url.value_or(""));
      return std::nullopt;
    }
    return url;
  };
  // An optional is a range of one or none, joined the script runs once per chapter
  return chapters | std::views::transform(to_url) | std::views::join | std::ranges::to<std::vector>();
}

///
///
auto workflow_bible_ref_lookup::update_scripts() -> void
{
  try
  {
    const auto scripts = workflow_script_->scripts<translations_manifest, url_manifest>();
    const auto names = scripts | std::views::keys | std::ranges::to<std::vector>();
    // No script chosen yet, or the chosen one is gone: the one offering the most translations takes its place
    const auto gone = !util::contains(names, settings().script->value());
    std::ignore = list_validator(settings().script)->available(names);
    if(gone && !scripts.empty())
    {
      const auto language = std::string{util::enum_name(system::locale::preferred_language())};
      const auto translations = [&](const workflow_script::scripts_type::value_type& script)
      {
        const auto output = workflow_script_->run<translations_manifest>(script.first, {language});
        return output ? output->get<"names">().size() : std::size_t{0};
      };
      std::ignore = settings().script->value(std::ranges::max_element(scripts, {}, translations)->first);
    }
    update_translations();
  }
  catch(...)
  {
    LOG_ERROR("exception occurred: {}", util::exception_report());
  }
}

///
///
auto workflow_bible_ref_lookup::update_translations() -> void
{
  try
  {
    const auto language = std::string{util::enum_name(system::locale::preferred_language())};
    const auto output = workflow_script_->run<translations_manifest>(settings().script->value(), {language});
    if(!output)
    {
      return;
    }
    const auto& names = output->get<"names">();
    const auto offered = [&](const std::string& name) { return util::contains(names, name); };
    const auto of_names = [&](const std::vector<std::string>& translations)
    { return translations | std::views::filter(offered) | std::ranges::to<std::vector>(); };

    std::ignore = list_validator(settings().translations)->available(names);
    // Those of another script are gone, the defaults of the script take their place
    auto translations = of_names(settings().translations->value());
    if(translations.empty())
    {
      translations = of_names(output->get<"defaults">().value_or(std::vector<std::string>{}));
    }
    std::ignore = settings().translations->value(translations);
  }
  catch(...)
  {
    LOG_ERROR("exception occurred: {}", util::exception_report());
  }
}

} // namespace bibstd::workflow
