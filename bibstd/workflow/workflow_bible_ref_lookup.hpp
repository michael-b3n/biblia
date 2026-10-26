#pragma once

#include "bibstd/bible/reference_range.hpp"
#include "bibstd/framework/process_params.hpp"
#include "bibstd/framework/queued_connections.hpp"
#include "bibstd/lua/script_table.hpp"
#include "bibstd/util/path.hpp"
#include "bibstd/workflow/workflow_base.hpp"
#include "bibstd/workflow/workflow_script.hpp"
#include "bibstd/workflow/workflow_settings_base.hpp"

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace bibstd::workflow
{

///
/// Settings corresponding to workflow bible reference lookup.
///
class workflow_bible_ref_lookup_settings final : public workflow_settings_base
{
public: // Structors
  workflow_bible_ref_lookup_settings(std::shared_ptr<workflow_settings> workflow_settings);
  ~workflow_bible_ref_lookup_settings() noexcept override = default;

public: // Variables
  const setting_type<std::string> script;
  const setting_type<std::vector<std::string>> translations;
};

///
/// Workflow for bible reference lookup. The script of the setting "lookup.script" builds the url of each chapter
/// of a reference, the workflow opens it in the browser. Scripts offering the two manifests below can be chosen,
/// the chosen one tells the translations of the setting "lookup.translations". Until the user chooses, it is the
/// script offering the most translations.
///
class workflow_bible_ref_lookup final : public workflow_base<workflow_bible_ref_lookup_settings>
{
  // Typedefs
  struct params_t final
  {
    std::vector<bible::reference_range> references;
  };

  // Variables
  const std::shared_ptr<workflow_script> workflow_script_;
  mutable std::mutex mtx_;
  framework::queued_connections connections_;

public: // Typedefs
  using params = framework::process_params<params_t>;

  // Manifests of a script offering the lookup, \see doc/lua_api.md
  ///
  /// \lua A script that offers this and `lookup.url` opens references in the browser. `Lookup > Scripts` chooses the
  /// script by its name. Until the user chooses, the app uses the script with the most translations. Respect the terms
  /// of use of the web pages a script reads.
  /// \param language the language of the user, `english` or `german`
  /// \return names the translations to choose from in `Lookup > Translations`, may be empty. The app shows them as
  /// they are, so the bundled scripts offer full names, e.g. `Lutherbibel`, and map them to url abbreviations.
  /// \return defaults the translations used until the user chooses some
  ///
  struct translations_manifest final
  {
    static inline const util::path id{"lookup.translations"};
    using input = lua::script_table<lua::field<"language", std::string>>;
    using output = lua::script_table<
      lua::field<"names", std::vector<std::string>>,
      lua::field<"defaults", std::optional<std::vector<std::string>>>>;
  };

  ///
  /// \lua For a reference over several chapters the app asks once per chapter and opens each url in its own tab.
  /// \param translations the chosen translations, in the order chosen
  /// \param book the identifier of the app, e.g. `john`
  /// \param chapter the chapter
  /// \param verse_begin the first verse
  /// \param verse_end the last verse
  /// \return url the web page that shows the verses, or `nil`. The app opens only a valid `http` or `https` url, so
  /// encode characters that are not ASCII.
  ///
  struct url_manifest final
  {
    static inline const util::path id{"lookup.url"};
    using input = lua::script_table<
      lua::field<"translations", std::vector<std::string>>,
      lua::field<"book", std::string>,
      lua::field<"chapter", std::int64_t>,
      lua::field<"verse_begin", std::int64_t>,
      lua::field<"verse_end", std::int64_t>>;
    using output = lua::script_table<lua::field<"url", std::optional<std::string>>>;
  };

public: // Structors
  workflow_bible_ref_lookup(
    std::shared_ptr<workflow_settings> workflow_settings, std::shared_ptr<workflow_script> workflow_script
  );
  ~workflow_bible_ref_lookup() noexcept override;

public: // Modifiers
  ///
  /// Look up bible references: the url of each chapter is opened in the default web browser. Blocks until
  /// then, a failure is logged. A reference over several books is looked up at its first verse only.
  ///
  auto lookup(const params& params) -> void;

private: // Implementation
  [[nodiscard]] auto urls(const bible::reference_range& range) const -> std::vector<std::string>;
  auto update_scripts() -> void;
  auto update_translations() -> void;
};

} // namespace bibstd::workflow
