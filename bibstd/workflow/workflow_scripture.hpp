#pragma once

#include "arca/framework/process_params.hpp"
#include "arca/framework/queued_connections.hpp"
#include "arca/lua/script_table.hpp"
#include "arca/signal/adapter.hpp"
#include "arca/signal/common.hpp"
#include "arca/util/const_map.hpp"
#include "arca/util/path.hpp"
#include "arca/workflow/workflow_base.hpp"
#include "arca/workflow/workflow_script.hpp"
#include "arca/workflow/workflow_settings.hpp"
#include "arca/workflow/workflow_settings_base.hpp"
#include "bibstd/bible/scripture_outline.hpp"
#include "bibstd/workflow/workflow_scripture_store.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace bibstd::workflow
{

///
/// Signals emitted by workflow scripture.
///
struct workflow_scripture_sigs final
{
  signal::signal_type<void()> scriptures_changed;
};

///
/// Settings corresponding to workflow scripture.
///
class workflow_scripture_settings final : public workflow_settings_base
{
public: // Structors
  workflow_scripture_settings(std::shared_ptr<workflow_settings> workflow_settings);
  ~workflow_scripture_settings() noexcept override = default;

public: // Variables
  const setting_type<std::optional<std::string>> default_scripture_name;
  const setting_type<std::string> fallback_versification;
};

///
/// Workflow for scripture. Scriptures come from the files of the scripture store or from scripts.
/// A scripture of a script without versification takes the fallback versification.
/// Signal IDs to connect to:
/// - scriptures_changed: Emitted when the scriptures of the store and the scripts changed.
///
class workflow_scripture final
  : public workflow_base<workflow_scripture_settings>
  , public signal::adapter<workflow_scripture_sigs>
{
  // Typedefs
  ///
  /// The versification of a scripture without exposing the scripture.
  ///
  class versification_wrapper final
  {
  public: // Constructor
    explicit versification_wrapper(std::shared_ptr<bible::scripture_outline> outline);
    explicit versification_wrapper(bible::versification versification);

  public: // Accessors
    ///
    /// Get the underlying versification.
    /// \return the underlying versification
    ///
    [[nodiscard]] auto get() const -> const bible::versification&;

  private: // Variables
    std::variant<bible::versification, std::shared_ptr<bible::scripture_outline>> data_;
  };

  struct scripture_params_t final
  {
    std::optional<std::string> scripture_name;
  };

  struct passage_params_t final
  {
    bible::reference reference;
    std::optional<std::string> scripture_name;
  };

  struct passage_result_t final
  {
    bible::passage passage;
  };

  struct import_params_t final
  {
    std::vector<std::filesystem::path> sources;
  };

  struct script_scripture_t final
  {
    workflow_script::script_name_type script;
    std::string name;
  };

  using script_scriptures_t = std::map<std::string, script_scripture_t>;
  using source_t = std::variant<workflow_scripture_store::file_scripture, script_scriptures_t::value_type>;

  // Variables
  const std::shared_ptr<workflow_script> workflow_script_;
  const std::unique_ptr<workflow_scripture_store> workflow_scripture_store_;
  mutable std::mutex mtx_;
  std::optional<script_scriptures_t> script_scriptures_;
  framework::queued_connections connections_;
  lua::registration registrations_;

public: // Constants
  static constexpr auto default_versifications = []()
  {
    using all_defaults_variant = bible::versification::all_defaults_variant;
    return [&]<std::size_t... I>(std::index_sequence<I...>)
    {
      return util::make_const_bimap<std::string_view, bible::versification>({
        {std::string_view{meta::pack_info<all_defaults_variant>::type_at<I>::name},
         bible::versification{meta::pack_info<all_defaults_variant>::type_at<I>{}}}
        ...
      });
    }(std::make_index_sequence<meta::pack_info<all_defaults_variant>::size>{});
  }();

public: // Typedefs
  using versification_wrapper_type = versification_wrapper;
  using scripture_params = framework::process_params<scripture_params_t>;
  using passage_params = framework::process_params<passage_params_t>;
  using passage_result = std::expected<passage_result_t, std::string>;
  using import_params = framework::process_params<import_params_t>;

  // Manifests of a script offering scriptures, \see doc/lua_api.md
  // clang-format off
  ///
  /// \lua A script that offers this and the three manifests below offers scriptures, and several scripts may do so.
  /// The app lists them with its other scriptures, named after the script, e.g. `LUT (Bibleserver)`, and uses the
  /// fallback versification for them. Respect the copyright of the translations.
  /// \return names the scriptures the script offers, e.g. `{ "ABC" }`
  ///
  struct names_manifest final
  {
    static inline const util::path id{"scripture.names"};
    using input = lua::script_table<>;
    using output = lua::script_table<lua::field<"names", std::vector<std::string>>>;
  };

  ///
  /// \lua Information about a scripture.
  /// \param name one of the names the script offers
  /// \return abbreviation its abbreviation
  /// \return language its language
  /// \return copyright its copyright, shown below the verses
  ///
  struct information_manifest final
  {
    static inline const util::path id{"scripture.information"};
    using input = lua::script_table<lua::field<"name", std::string>>;
    using output = lua::script_table<
      lua::field<"abbreviation", std::optional<std::string>>,
      lua::field<"language", std::optional<std::string>>,
      lua::field<"copyright", std::optional<std::string>>>;
  };

  ///
  /// \lua The names of a book in a scripture.
  /// \param name one of the names the script offers
  /// \param book the identifier of the app, e.g. `john`
  /// \return abbreviation its abbreviation
  /// \return short_name its short name, shown in the app and copied. The app has no names of its own, so a script names every book it can not fetch a name for.
  /// \return long_name its long name
  ///
  struct book_manifest final
  {
    static inline const util::path id{"scripture.book"};
    using input = lua::script_table<lua::field<"name", std::string>, lua::field<"book", std::string>>;
    using output = lua::script_table<
      lua::field<"abbreviation", std::optional<std::string>>,
      lua::field<"short_name", std::optional<std::string>>,
      lua::field<"long_name", std::optional<std::string>>>;
  };

  ///
  /// \lua A verse. The app asks for every verse it shows and keeps nothing, so the script may ask `workflow.cache`
  /// first. A reader of scripture files offers it too, \see `scripture.read`.
  /// \param name one of the names the script offers, for a reader the name of the file
  /// \param book the identifier of the app, e.g. `john`
  /// \param chapter the chapter
  /// \param verse the verse
  /// \return markup the verse as a sequence of paragraph sections `<p data-id="...">`: `begin` for a verse that opens
  /// its paragraph, `continue` for one that continues it and `undefined` outside of paragraphs. Inside, `<b>` and
  /// `<i>` mark text up, and `&`, `<` and `>` are escaped. A verse without text of its own is empty, e.g. 3 and 4 of a
  /// verse "2-4".
  /// \return text its plain text in place of \p markup. The app shows markup of a web page as text.
  /// \return cross_references the cross references of the verse
  /// \return error shown in place of a verse without \p markup or \p text. Without one the app shows `verse not found`.
  ///
  struct passage_manifest final
  {
    static inline const util::path id{"scripture.passage"};
    using input = lua::script_table<
      lua::field<"name", std::string>,
      lua::field<"book", std::string>,
      lua::field<"chapter", std::int64_t>,
      lua::field<"verse", std::int64_t>>;
    using output = lua::script_table<
      lua::field<"markup", std::optional<std::string>>,
      lua::field<"text", std::optional<std::string>>,
      lua::field<"cross_references", std::optional<std::vector<std::string>>>,
      lua::field<"error", std::optional<std::string>>>;
  };
  // clang-format on

public: // Structors
  workflow_scripture(std::shared_ptr<workflow_settings> workflow_settings, std::shared_ptr<workflow_script> workflow_script);
  ~workflow_scripture() noexcept override;

public: // Accessors
  ///
  /// \return the number of scriptures, those of the scripts included
  ///
  [[nodiscard]] auto scripture_count() const -> std::size_t;

  ///
  /// \return true once the scriptures of the files and the scripts are loaded after the scripts
  ///
  [[nodiscard]] auto loaded() const -> bool;

  ///
  /// Information about the scripture of the params, or else of the settings. Like book_information and passage
  /// it blocks while the script answers, so neither is meant for the UI thread.
  /// \return information, or std::nullopt if there is no such scripture
  ///
  [[nodiscard]] auto information(const scripture_params& params) const -> std::optional<bible::scripture_info>;

  ///
  /// \return the names of \p book in the scripture of the params, or else of the settings, std::nullopt if none
  ///
  [[nodiscard]] auto book_information(const scripture_params& params, bible::book_id book) const
    -> std::optional<bible::book_name>;

  ///
  /// \return the versification of the scripture of \p params, the fallback versification if there is none
  ///
  [[nodiscard]] auto versification_or_fallback(const scripture_params& params) const -> versification_wrapper_type;

  ///
  /// Get a passage, of the scripture of the settings if \p params names none.
  /// \return passage, or why there is none: "scripture not found", the error of the script,
  /// else "verse not found" or "script failed"
  ///
  [[nodiscard]] auto passage(const passage_params& params) const -> passage_result;

public: // Modifiers
  ///
  /// Take the scripture files named in the params, or those of the folders named there, into the scripture folder
  /// and load them, \see workflow_scripture_store::import. Blocks until the import is over, a failure is logged.
  /// \return number of imported files
  ///
  [[nodiscard]] auto import_scriptures(const import_params& params) -> std::size_t;

  ///
  /// Read the files of the scripture folder anew, \see workflow_scripture_store::reload. Blocks until all are read.
  ///
  auto reload_scriptures() -> void;

private: // Implementation
  [[nodiscard]] auto scripture_names() const -> std::vector<std::string>;
  [[nodiscard]] auto selected_name(const std::optional<std::string>& name) const -> std::optional<std::string>;
  [[nodiscard]] auto find(const std::optional<std::string>& name) const -> std::optional<source_t>;
  template<script_manifest M>
  [[nodiscard]] auto run_script(const script_scripture_t& scripture, typename M::input input) const
    -> std::optional<typename M::output>;
  auto update_scripts() -> void;
  auto update_scripture_name_setting() -> void;
};

} // namespace bibstd::workflow
