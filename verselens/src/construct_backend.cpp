#include "src/construct_backend.hpp"
#include "res/version.hpp"

#include <arcstd/cache/cache_store.hpp>
#include <arcstd/lua/lua_engine.hpp>
#include <arcstd/setting/setting_store.hpp>
#include <arcstd/web/web_fetcher.hpp>
#include <bibstd/lua/bundled_scripts.hpp>
#include <bibstd/workflow/workflow_bible_ref_copy.hpp>
#include <bibstd/workflow/workflow_bible_ref_lookup.hpp>
#include <bibstd/workflow/workflow_bible_ref_ocr.hpp>
#include <bibstd/workflow/workflow_bible_ref_ocr_auto.hpp>
#include <bibstd/workflow/workflow_hotkey.hpp>
#include <bibstd/workflow/workflow_scripture.hpp>

#include <memory>

namespace verselens
{

///
///
auto construct_backend() -> backend_instance
{
  // Init backend
  // clang-format off
  auto setting_store = std::make_shared<arcstd::setting::setting_store>(version::data_folder_name);
  auto lua_engine = std::make_shared<arcstd::lua::lua_engine>(setting_store, bibstd::lua::bundled::all);
  auto web_fetcher = std::make_shared<arcstd::web::web_fetcher>(setting_store, lua_engine);
  auto cache_store = std::make_shared<arcstd::cache::cache_store>(setting_store->data_folder(), lua_engine);
  auto workflow_hotkey = std::make_shared<bibstd::workflow::workflow_hotkey>(setting_store);
  auto workflow_scripture = std::make_shared<bibstd::workflow::workflow_scripture>(setting_store, lua_engine);
  auto workflow_bible_ref_ocr = std::make_shared<bibstd::workflow::workflow_bible_ref_ocr>(setting_store, workflow_scripture);
  auto workflow_bible_ref_ocr_auto = std::make_shared<bibstd::workflow::workflow_bible_ref_ocr_auto>(setting_store, workflow_bible_ref_ocr);
  auto workflow_bible_ref_lookup = std::make_shared<bibstd::workflow::workflow_bible_ref_lookup>(setting_store, lua_engine);
  auto workflow_bible_ref_copy = std::make_shared<bibstd::workflow::workflow_bible_ref_copy>(setting_store, workflow_scripture);
  // clang-format on
  lua_engine->load_scripts();

  // Construct workflows here. The returned scoped guard will deinitialize the workflows when it goes out of scope.
  return backend_instance{
    .setting_store{std::move(setting_store)},
    .lua_engine{std::move(lua_engine)},
    .web_fetcher{std::move(web_fetcher)},
    .cache_store{std::move(cache_store)},
    .workflow_hotkey{std::move(workflow_hotkey)},
    .workflow_scripture{std::move(workflow_scripture)},
    .workflow_bible_ref_ocr{std::move(workflow_bible_ref_ocr)},
    .workflow_bible_ref_ocr_auto{std::move(workflow_bible_ref_ocr_auto)},
    .workflow_bible_ref_lookup{std::move(workflow_bible_ref_lookup)},
    .workflow_bible_ref_copy{std::move(workflow_bible_ref_copy)}
  };
}

///
///
auto shutdown_backend(const backend_instance& backend) -> void
{
  backend.lua_engine->shutdown();
}

} // namespace verselens
