#pragma once

#include "bibstd/bible/reference.hpp"
#include "bibstd/bible/reference_range.hpp"
#include "bibstd/workflow/process_params.hpp"
#include "bibstd/workflow/workflow_base.hpp"
#include "bibstd/workflow/workflow_settings_base.hpp"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace bibstd::workflow
{

class workflow_scripture;

///
/// Settings corresponding to workflow bible reference copy.
///
class workflow_bible_ref_copy_settings final : public workflow_settings_base
{
public: // Structors
  workflow_bible_ref_copy_settings(std::shared_ptr<arcstd::setting::setting_store> setting_store);
  ~workflow_bible_ref_copy_settings() noexcept override = default;

public: // Variables
  const setting_type<std::string> mask;
};

///
/// Workflow for copying bible references. The setting "copy.mask" writes a reference as text, \see bible::reference_mask.
/// Books and verses are those of the scripture of the settings.
///
class workflow_bible_ref_copy final : public workflow_base<workflow_bible_ref_copy_settings>
{
  // Typedefs
  struct params_t final
  {
    bible::reference_range range;
  };
  using text_t = process_result<std::string>;
  using verse_t = std::pair<bible::reference::verse_type, std::string>;

  // Variables
  const std::shared_ptr<workflow_scripture> workflow_scripture_;

public: // Typedefs
  using params = process_params<params_t>;

public: // Structors
  workflow_bible_ref_copy(
    std::shared_ptr<arcstd::setting::setting_store> setting_store, std::shared_ptr<workflow_scripture> workflow_scripture
  );
  ~workflow_bible_ref_copy() noexcept override;

public: // Modifiers
  ///
  /// Copy the text of the reference to the clipboard. Blocks while a script answers.
  /// \return true if copied, false if the text is empty, a verse is missing or the clipboard fails
  ///
  [[nodiscard]] auto copy(const params& params) -> bool;

private: // Implementation
  [[nodiscard]] auto text(const bible::reference_range& range) const -> text_t;
  [[nodiscard]] auto verses(const bible::reference_range& range, const std::optional<std::string>& scripture_name) const
    -> std::optional<std::vector<verse_t>>;
};

} // namespace bibstd::workflow
