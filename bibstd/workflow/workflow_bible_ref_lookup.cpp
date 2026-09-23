#include "bibstd/workflow/workflow_bible_ref_lookup.hpp"
#include "bibstd/core/core_lookup_bibleserver.hpp"
#include "bibstd/system/locale.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/log.hpp"

namespace bibstd::workflow
{
namespace
{

///
/// Get the translations looked up by default for the language the user prefers.
/// \return default translations
///
[[nodiscard]] auto default_translations(const util::language language) -> std::vector<bible::translation>
{
  // No default case, so that a new language warns about its missing translations.
  switch(language)
  {
  case util::language::english: return {bible::translation::niv, bible::translation::esv};
  case util::language::german: return {bible::translation::ngu, bible::translation::elb};
  }
  throw util::exception{"unsupported language"};
}

} // namespace

///
///
workflow_bible_ref_lookup_settings::workflow_bible_ref_lookup_settings(std::shared_ptr<workflow_settings> workflow_settings)
  : framework::settings_base{std::move(workflow_settings)}
  , translations{
      workflow_settings_->create_setting("lookup.translations", default_translations(system::locale::preferred_language()))
    }
{
}

///
///
workflow_bible_ref_lookup::workflow_bible_ref_lookup(std::shared_ptr<workflow_settings> workflow_settings)
  : workflow_base{std::move(workflow_settings)}
  , thread_pool_guard_{framework::thread_pool::init()}
  , core_lookup_bibleserver_{std::make_unique<core::core_lookup_bibleserver>()}
{
}

///
///
workflow_bible_ref_lookup::~workflow_bible_ref_lookup() noexcept = default;

///
///
auto workflow_bible_ref_lookup::lookup(const params& params) -> void
{
  try
  {
    framework::thread_pool::queue_task(
      [this, params]() mutable
      {
        const auto translations = settings().translations->value();
        try
        {
          std::ranges::for_each(
            params->references,
            [&](const auto& reference_range) { core_lookup_bibleserver_->open(reference_range, translations); }
          );
          notify(&signals_type::ended, params.process_id());
        }
        catch(...)
        {
          LOG_ERROR("exception occurred: {}", util::exception_report());
          notify(&signals_type::ended, params.process_id());
        }
      },
      strand_id_
    );
  }
  catch(...)
  {
    LOG_ERROR("exception occurred: {}", util::exception_report());
    notify(&signals_type::ended, params.process_id());
  }
}

} // namespace bibstd::workflow
