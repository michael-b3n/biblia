#pragma once

#include "src/construct_backend.hpp"

#include <bibqml/translation/Translations.hpp>

#include <bibstd/framework/setting.hpp>
#include <bibstd/signal/synchronized_executor.hpp>
#include <bibstd/util/non_owning_ptr.hpp>

#include <optional>
#include <string>
#include <string_view>

namespace verselens
{

///
/// Read the language the display names are written in from the settings file. The file is only
/// read, which lets an instance owning no settings display the language of the one that does.
/// \return language, std::nullopt if the file holds none
///
[[nodiscard]] auto read_language_setting() -> std::optional<std::string>;

///
/// Instance holding the translations of the application.
/// This owns the QML translations singleton and keeps the language its display
/// names are written in synchronized with the language setting. The translations themselves know nothing
/// about settings, and the backend knows nothing about translations.
///
class translations_instance final
{
  // Variables
  const std::unique_ptr<bibqml::Translations> translations_;
  const bibstd::util::non_owning_ptr<bibstd::framework::setting<std::string>> language_setting_;
  bibstd::signal::synchronized_executor executor_;

public: // Typedefs
  using language_setting_type = decltype(language_setting_);

public: // Constants
  static constexpr auto language_setting_path = std::string_view{"ui.language"};

public: // Structors
  ///
  /// Construct the translations instance.
  /// If no setting is provided, the display names stay in their default language.
  ///
  translations_instance(std::unique_ptr<bibqml::Translations> translations, language_setting_type language_setting);

  ///
  /// Construct the translations instance without a setting to follow.
  /// The display names are written in the given language and never change afterwards.
  ///
  translations_instance(std::unique_ptr<bibqml::Translations> translations, const std::optional<std::string>& language);

  ~translations_instance() noexcept;

public: // Modifiers
  ///
  /// Disconnect all signal connections.
  /// This will stop the frontend backend communication.
  ///
  auto disconnect() -> void;
};

///
/// Initialize the translations of the application.
/// The display names are compiled into the application, the language they are displayed in is
/// stored in a setting that is created in the settings workflow of the backend.
/// \return translations instance, holding no display names if they could not be loaded
///
auto construct_translations(backend_instance& backend) -> translations_instance;

///
/// Initialize the translations of an application that owns no settings.
/// The display names are written in \p language, in the default language if not set.
/// \return translations instance, holding no display names if they could not be loaded
///
auto construct_translations(const std::optional<std::string>& language) -> translations_instance;

} // namespace verselens
