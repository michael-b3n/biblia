#pragma once

#include "bibstd/bible/scripture.hpp"
#include "bibstd/framework/process_params.hpp"
#include "bibstd/framework/settings_base.hpp"
#include "bibstd/framework/thread_pool.hpp"
#include "bibstd/lua/script_table.hpp"
#include "bibstd/signal/adapter.hpp"
#include "bibstd/signal/common.hpp"
#include "bibstd/signal/synchronized_executor.hpp"
#include "bibstd/util/const_map.hpp"
#include "bibstd/util/path.hpp"
#include "bibstd/workflow/workflow_base.hpp"
#include "bibstd/workflow/workflow_script.hpp"
#include "bibstd/workflow/workflow_settings.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <variant>
#include <vector>

// Forward declarations
namespace bibstd::core
{
class core_scripture_store;
} // namespace bibstd::core

namespace bibstd::workflow
{

///
/// Signals emitted by workflow scripture.
///
struct workflow_scripture_sigs final
{
  signal::signal_type<void(framework::process_id_type, std::size_t)> import_ended;
  signal::signal_type<void()> scriptures_changed;
};

///
/// Settings corresponding to workflow scripture.
///
class workflow_scripture_settings final : public framework::settings_base
{
public: // Structors
  workflow_scripture_settings(std::shared_ptr<workflow_settings> workflow_settings);
  ~workflow_scripture_settings() noexcept override = default;

public: // Constants
  static constexpr auto default_folder_name = "scriptures";

public: // Variables
  const setting_type<std::optional<std::string>> scripture_name;
  const setting_type<std::filesystem::path> scripture_folder;
  const setting_type<std::string> fallback_versification;
};

///
/// Workflow for scripture. The scriptures are the zip files in the folder of the setting "scripture.folder",
/// by default the folder "scriptures" in the local data folder. They are loaded on construction.
/// More scriptures come from the scripts implementing the four manifests below, named after the id of their
/// script, e.g. "LUT (scripture_bibleserver)". Scriptures from scripts use the fallback versification.
/// Signal IDs to connect to:
/// - import_ended: Emitted when an import ended. Slots receive the process ID and the number of imported files.
/// - scriptures_changed: Emitted when the scriptures of the scripts changed, e.g. once the scripts are loaded.
///
class workflow_scripture final
  : public workflow_base<workflow_scripture_settings>
  , public signal::adapter<workflow_scripture_sigs>
{
  // Typedefs
  ///
  /// Wrapper for bible scripture versification.
  /// This wrapper is needed to provide access to the versification
  /// of a scripture without exposing the whole scripture.
  ///
  class versification_wrapper final
  {
  public: // Constructor
    explicit versification_wrapper(std::shared_ptr<bible::scripture> scripture);
    explicit versification_wrapper(bible::versification versification);

  public: // Accessors
    ///
    /// Get the underlying versification.
    /// \return the underlying versification
    ///
    auto get() const -> const bible::versification&;

  private: // Variables
    std::variant<bible::versification, std::shared_ptr<bible::scripture>> data_;
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
    std::filesystem::path folder;
  };

  // Scripture of a script, \see update_scripts
  struct script_scripture_t final
  {
    util::identifier script;
    std::string name; // in the script
  };

  // Variables
  const framework::thread_pool::strand_id_type strand_id_{framework::thread_pool::strand_id()};
  const util::shared_scope_guard thread_pool_guard_;
  const std::shared_ptr<workflow_script> workflow_script_;
  const std::unique_ptr<core::core_scripture_store> core_scripture_store_;
  mutable std::mutex mtx_;
  std::optional<std::map<std::string, script_scripture_t>> script_scriptures_; // by name in the app, guarded by mtx_
  signal::synchronized_executor executor_{strand_id_};

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
  using passage_result = framework::process_result<passage_result_t>;
  using import_params = framework::process_params<import_params_t>;

  // Manifests of a script offering scriptures, \see doc/lua_scripts.md
  struct names_manifest final
  {
    static inline const util::path id{"scripture.names"};
    using input = lua::script_table<>;
    using output = lua::script_table<lua::field<"names", std::vector<std::string>>>;
  };

  struct information_manifest final
  {
    static inline const util::path id{"scripture.information"};
    using input = lua::script_table<lua::field<"name", std::string>>;
    using output = lua::script_table<
      lua::field<"abbreviation", std::optional<std::string>>,
      lua::field<"language", std::optional<std::string>>,
      lua::field<"copyright", std::optional<std::string>>>;
  };

  struct book_manifest final
  {
    static inline const util::path id{"scripture.book"};
    using input = lua::script_table<lua::field<"name", std::string>, lua::field<"book", std::string>>;
    using output = lua::script_table<
      lua::field<"abbreviation", std::optional<std::string>>,
      lua::field<"short_name", std::optional<std::string>>,
      lua::field<"long_name", std::optional<std::string>>>;
  };

  struct passage_manifest final
  {
    static inline const util::path id{"scripture.passage"};
    using input = lua::script_table<
      lua::field<"name", std::string>,
      lua::field<"book", std::string>,
      lua::field<"chapter", std::int64_t>,
      lua::field<"verse", std::int64_t>>;
    using output = lua::script_table<lua::field<"text", std::optional<std::string>>>;
  };

public: // Structors
  workflow_scripture(std::shared_ptr<workflow_settings> workflow_settings, std::shared_ptr<workflow_script> workflow_script);
  ~workflow_scripture() noexcept override;

public: // Accessors
  ///
  /// \return the number of scriptures, those of the scripts included
  ///
  [[nodiscard]] auto scripture_count() const -> std::size_t;

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
  /// Get the versification of the specifieds scripture, or the fallback versification
  /// if the scripture is not available. No matter if there are no scriptures loaded or
  /// the scripture name does not exist, the fallback versification is always returned.
  /// \return versification
  ///
  [[nodiscard]] auto versification_or_fallback(const scripture_params& params) const -> versification_wrapper_type;

  ///
  /// Get passage from scripture. If no scripture name is provided in the params,
  /// the scripture defined in the settings will be used.
  /// \return passage, or an unexpected result in case of failure
  ///
  [[nodiscard]] auto passage(const passage_params& params) const -> passage_result;

public: // Modifiers
  ///
  /// Take the scripture files of the folder named in the params into the scripture folder and
  /// load them, \see core_scripture_store::import. The work is done outside of the calling thread.
  /// \note import_ended is emitted once the import is over, also if it took over nothing.
  ///
  auto import_scriptures(const import_params& params) -> void;

private: // Implementation
  [[nodiscard]] auto scripture_names() const -> std::vector<std::string>;
  [[nodiscard]] auto selected_name(const std::optional<std::string>& name) const -> std::optional<std::string>;
  [[nodiscard]] auto stored(const std::optional<std::string>& name) const -> std::shared_ptr<bible::scripture>;
  template<script_manifest M>
  [[nodiscard]] auto run_script(typename M::input input) const -> std::optional<typename M::output>;
  auto update_scripts() -> void;
  auto update_scripture_name_setting() -> void;
};

} // namespace bibstd::workflow
