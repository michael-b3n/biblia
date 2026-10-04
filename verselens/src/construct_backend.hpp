#pragma once

#include <bibstd/workflow/workflow_base.hpp>
#include <bibstd/workflow/workflow_settings.hpp>

#include <memory>

namespace verselens
{

///
/// Struct containing all backend components.
///
struct backend_instance final
{
  std::shared_ptr<bibstd::workflow::workflow_settings> workflow_settings;
  std::shared_ptr<bibstd::workflow::workflow_ground> workflow_script;
  std::shared_ptr<bibstd::workflow::workflow_ground> workflow_web;
  std::shared_ptr<bibstd::workflow::workflow_ground> workflow_cache;
  std::shared_ptr<bibstd::workflow::workflow_ground> workflow_hotkey;
  std::shared_ptr<bibstd::workflow::workflow_ground> workflow_scripture;
  std::shared_ptr<bibstd::workflow::workflow_ground> workflow_bible_ref_ocr;
  std::shared_ptr<bibstd::workflow::workflow_ground> workflow_bible_ref_ocr_auto;
  std::shared_ptr<bibstd::workflow::workflow_ground> workflow_bible_ref_lookup;

  std::shared_ptr<bibstd::workflow::workflow_ground> workflow_template;
};

///
/// Initialize backend components.
/// \return backend instance
///
auto construct_backend() -> backend_instance;

///
/// Stop the scripts of \p backend for good. Called first once the app ends,
/// so no workflow waits for a script that never returns when it is destroyed.
///
auto shutdown_backend(const backend_instance& backend) -> void;

} // namespace verselens
