#include "src/construct_backend.hpp"
#include "res/version.hpp"

#include <bibstd/workflow/workflow_bible_ref_lookup.hpp>
#include <bibstd/workflow/workflow_bible_ref_ocr.hpp>
#include <bibstd/workflow/workflow_bible_ref_ocr_auto.hpp>
#include <bibstd/workflow/workflow_hotkey.hpp>
#include <bibstd/workflow/workflow_scripture.hpp>
#include <bibstd/workflow/workflow_settings.hpp>
#include <bibstd/workflow/workflow_template.hpp>

#include <memory>

namespace verselens
{

///
///
auto construct_backend() -> backend_instance
{
  // Init backend
  // clang-format off
  auto workflow_settings = std::make_shared<bibstd::workflow::workflow_settings>(version::data_folder_name);
  auto workflow_hotkey = std::make_shared<bibstd::workflow::workflow_hotkey>(workflow_settings);
  auto workflow_scripture = std::make_shared<bibstd::workflow::workflow_scripture>(workflow_settings);
  auto workflow_bible_ref_ocr = std::make_shared<bibstd::workflow::workflow_bible_ref_ocr>(workflow_settings, workflow_scripture);
  auto workflow_bible_ref_ocr_auto = std::make_shared<bibstd::workflow::workflow_bible_ref_ocr_auto>(workflow_settings, workflow_bible_ref_ocr);
  auto workflow_bible_ref_lookup = std::make_shared<bibstd::workflow::workflow_bible_ref_lookup>(workflow_settings);
  auto workflow_template = std::make_shared<bibstd::workflow::workflow_template>(workflow_settings);
  // clang-format on

  // Construct workflows here. The returned scoped guard will deinitialize the workflows when it goes out of scope.
  return backend_instance{
    .workflow_settings{std::move(workflow_settings)},
    .workflow_hotkey{std::move(workflow_hotkey)},
    .workflow_scripture{std::move(workflow_scripture)},
    .workflow_bible_ref_ocr{std::move(workflow_bible_ref_ocr)},
    .workflow_bible_ref_ocr_auto{std::move(workflow_bible_ref_ocr_auto)},
    .workflow_bible_ref_lookup{std::move(workflow_bible_ref_lookup)},
    .workflow_template{std::move(workflow_template)}
  };
}

} // namespace verselens
