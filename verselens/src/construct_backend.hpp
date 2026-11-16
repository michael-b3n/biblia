#pragma once

#include <arcstd/cache/cache_store.hpp>
#include <arcstd/lua/lua_engine.hpp>
#include <arcstd/setting/setting_store.hpp>
#include <arcstd/web/web_fetcher.hpp>
#include <bibstd/workflow/workflow_base.hpp>

#include <filesystem>
#include <memory>

namespace verselens
{

///
/// Struct containing all backend components.
///
struct backend_instance final
{
  std::shared_ptr<arcstd::setting::setting_store> setting_store;
  std::shared_ptr<arcstd::lua::lua_engine> lua_engine;
  std::shared_ptr<arcstd::web::web_fetcher> web_fetcher;
  std::shared_ptr<arcstd::cache::cache_store> cache_store;
  std::shared_ptr<bibstd::workflow::workflow_ground> workflow_hotkey;
  std::shared_ptr<bibstd::workflow::workflow_ground> workflow_scripture;
  std::shared_ptr<bibstd::workflow::workflow_ground> workflow_bible_ref_ocr;
  std::shared_ptr<bibstd::workflow::workflow_ground> workflow_bible_ref_ocr_auto;
  std::shared_ptr<bibstd::workflow::workflow_ground> workflow_bible_ref_lookup;
  std::shared_ptr<bibstd::workflow::workflow_ground> workflow_bible_ref_copy;
};

///
/// Initialize backend components, their data is kept in \p data_folder.
/// \return backend instance
///
[[nodiscard]] auto construct_backend(const std::filesystem::path& data_folder) -> backend_instance;

///
/// Stop the scripts of \p backend for good. Called first once the app ends,
/// so no workflow waits for a script that never returns when it is destroyed.
///
auto shutdown_backend(const backend_instance& backend) -> void;

} // namespace verselens
