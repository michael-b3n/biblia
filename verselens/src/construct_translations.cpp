#include "src/construct_translations.hpp"
#include "res/version.hpp"

#include <bibstd/framework/setting_validator.hpp>
#include <bibstd/util/exception.hpp>
#include <bibstd/util/incbin.hpp>
#include <bibstd/util/log.hpp>
#include <bibstd/workflow/workflow_settings.hpp>

#include <QMetaObject>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <algorithm>
#include <format>
#include <memory>
#include <span>
#include <utility>
#include <vector>

INC_RESOURCE(display_names, "res/display_names.csv");
const auto display_names_view = bibstd::util::incbin::to_span<std::byte>(res_display_names_data, res_display_names_size);

namespace verselens
{
namespace
{

///
/// The validator of the language setting lists what the document offers.
/// \return languages of the translations
///
[[nodiscard]] auto available_languages(const bibqml::Translations& translations) -> std::vector<std::string>
{
  const auto languages = translations.availableLanguages();
  auto result = std::vector<std::string>{};
  result.reserve(static_cast<std::size_t>(languages.size()));
  std::ranges::transform(languages, std::back_inserter(result), [](const auto& l) { return l.toStdString(); });
  return result;
}

///
/// Translations naming every key after itself, for a start that could not read the document.
/// \return translations holding no names
///
[[nodiscard]] auto no_translations() -> std::unique_ptr<bibqml::Translations>
{
  return std::make_unique<bibqml::Translations>(std::span<const std::byte>{});
}

} // anonymous namespace

///
///
translations_instance::translations_instance(
  std::unique_ptr<bibqml::Translations> translations, const language_setting_type language_setting
)
  : translations_{std::move(translations)}
  , language_setting_{language_setting}
{
  if(language_setting_ == nullptr)
  {
    // Without a language setting the translations stay in their default language.
    return;
  }
  translations_->setLanguage(QString::fromStdString(language_setting_->value()));
  language_setting_->connect_queued(
    &bibstd::framework::setting_signals::value_changed,
    [this]()
    {
      QMetaObject::invokeMethod(
        translations_.get(),
        [this]() { translations_->setLanguage(QString::fromStdString(language_setting_->value())); },
        Qt::QueuedConnection
      );
    },
    executor_
  );
}

///
///
translations_instance::translations_instance(
  std::unique_ptr<bibqml::Translations> translations, const std::optional<std::string>& language
)
  : translations_{std::move(translations)}
  , language_setting_{nullptr}
{
  if(language.has_value())
  {
    translations_->setLanguage(QString::fromStdString(*language));
  }
}

///
///
translations_instance::~translations_instance() noexcept = default;

///
///
auto translations_instance::disconnect() -> void
{
  executor_.disconnect();
}

///
///
auto read_language_setting() -> std::optional<std::string>
{
  try
  {
    auto tree = boost::property_tree::ptree{};
    boost::property_tree::read_xml(
      bibstd::workflow::workflow_settings::settings_file_path(version::data_folder_name).generic_string(), tree
    );
    const auto language = tree.get_optional<std::string>(std::format(
      "{}.{}", bibstd::workflow::workflow_settings::settings_root_name, translations_instance::language_setting_path
    ));
    return language ? std::optional{*language} : std::nullopt;
  }
  catch(...)
  {
    return std::nullopt;
  }
}

///
///
auto construct_translations(backend_instance& backend) -> translations_instance
{
  try
  {
    auto translations = std::make_unique<bibqml::Translations>(display_names_view);
    const auto languages = available_languages(*translations);
    auto* const language_setting = backend.workflow_settings->create_setting(
      std::string{translations_instance::language_setting_path},
      languages.front(),
      std::make_shared<bibstd::framework::setting_validator_list<std::string>>(languages)
    );
    return translations_instance{std::move(translations), language_setting};
  }
  catch(...)
  {
    LOG_ERROR("construct translations failed: {}", bibstd::util::exception_report());
    return translations_instance{no_translations(), std::nullopt};
  }
}

///
///
auto construct_translations(const std::optional<std::string>& language) -> translations_instance
{
  try
  {
    return translations_instance{std::make_unique<bibqml::Translations>(display_names_view), language};
  }
  catch(...)
  {
    LOG_ERROR("construct translations failed: {}", bibstd::util::exception_report());
    return translations_instance{no_translations(), std::nullopt};
  }
}

} // namespace verselens
