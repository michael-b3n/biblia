#include "bibstd/workflow/workflow_bible_ref_copy.hpp"
#include "bibstd/bible/reference_mask.hpp"
#include "bibstd/bible/scripture_types.hpp"
#include "bibstd/bible/versification.hpp"
#include "bibstd/system/clipboard.hpp"
#include "bibstd/system/locale.hpp"
#include "bibstd/workflow/workflow_scripture.hpp"

#include <arcstd/util/exception.hpp>
#include <arcstd/util/format.hpp>
#include <arcstd/util/log.hpp>

#include <algorithm>
#include <cstdint>
#include <expected>
#include <format>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

namespace bibstd::workflow
{
namespace
{

///
/// \return the mask a reference is written with in the language of the user
///
[[nodiscard]] auto default_mask() -> std::string_view
{
  switch(bibstd::system::locale::preferred_language())
  {
    // clang-format off
  case bibocr::util::language::german: return R"("[content]" ({[book][nbsp]}[#chapter_begin], [#verse_begin]{[nbsp]- {[book_end] }{[#chapter_end],}[#verse_end]}{ [scripture_abbr]}))";
  case bibocr::util::language::english: return R"("[content]" ({[book][nbsp]}[#chapter_begin]: [#verse_begin]{[nbsp]- {[book_end] }{[#chapter_end]:}[#verse_end]}{ [scripture_abbr]}))";
    // clang-format on
  default: throw arcstd::util::exception{"unsupported language"};
  }
}

///
/// \return the text of \p verses joined by a space, verses without text left out
///
[[nodiscard]] auto joined_text(const auto& verses, const bool numbered) -> std::string
{
  const auto text = [numbered](const auto& verse)
  { return numbered && !verse.second.empty() ? std::format("{} {}", verse.first, verse.second) : verse.second; };
  const auto filled = [](const std::string& plain) { return !plain.empty(); };
  const auto texts = verses | std::views::transform(text) | std::views::filter(filled) | std::ranges::to<std::vector>();
  return arcstd::util::format::join(texts, " ");
}

} // namespace

///
///
workflow_bible_ref_copy_settings::workflow_bible_ref_copy_settings(
  std::shared_ptr<arcstd::workflow::workflow_settings> workflow_settings
)
  : arcstd::workflow::workflow_settings_base{std::move(workflow_settings)}
  , mask{workflow_settings_->create_setting("copy.mask", std::string{default_mask()})}
{
}

///
///
workflow_bible_ref_copy::workflow_bible_ref_copy(
  std::shared_ptr<arcstd::workflow::workflow_settings> workflow_settings, std::shared_ptr<workflow_scripture> workflow_scripture
)
  : workflow_base{std::move(workflow_settings)}
  , workflow_scripture_{std::move(workflow_scripture)}
{
}

///
///
workflow_bible_ref_copy::~workflow_bible_ref_copy() noexcept = default;

///
///
auto workflow_bible_ref_copy::copy(const params& params) -> bool
{
  const auto copied = text(params->range);
  if(!copied)
  {
    return false;
  }
  if(!bibstd::system::clipboard::set_text(*copied))
  {
    LOG_WARN("failed to write the clipboard: reference=\"{}\"", params->range);
    return false;
  }
  return true;
}

///
///
auto workflow_bible_ref_copy::text(const bible::reference_range& range) const -> text_t
{
  try
  {
    using scripture_params = workflow_scripture::scripture_params::value_type;
    const auto mask = settings().mask->value();
    const auto scripture = scripture_params{workflow_scripture_->settings().default_scripture_name->value()};
    const auto information = workflow_scripture_->information(scripture).value_or(bible::scripture_info{});
    const auto book = workflow_scripture_->book_information(scripture, range.begin().book()).value_or(bible::book_name{});
    const auto book_end = workflow_scripture_->book_information(scripture, range.end().book()).value_or(bible::book_name{});

    auto values = bible::reference_mask::values{
      .range = range,
      .book = book.short_name,
      .book_long = book.long_name,
      .book_abbreviation = book.abbreviation,
      .book_end = book_end.short_name,
      .scripture = information.name,
      .scripture_abbreviation = information.abbreviation,
      .copyright = information.copyright.value_or(""),
    };
    if(bible::reference_mask::uses_content(mask))
    {
      const auto plain_verses = verses(range, scripture.scripture_name);
      if(!plain_verses)
      {
        return std::unexpected{arcstd::framework::process_result_unexpected::failure};
      }
      values.content = joined_text(*plain_verses, false);
      values.content_numbered = joined_text(*plain_verses, true);
    }
    auto text = bible::reference_mask::render(mask, values);
    if(text.empty())
    {
      LOG_WARN("nothing to copy: mask=\"{}\", reference=\"{}\"", mask, range);
      return std::unexpected{arcstd::framework::process_result_unexpected::failure};
    }
    return text;
  }
  catch(...)
  {
    LOG_ERROR("exception occurred: {}", arcstd::util::exception_report());
    return std::unexpected{arcstd::framework::process_result_unexpected::failure};
  }
}

///
///
auto workflow_bible_ref_copy::verses(
  const bible::reference_range& range, const std::optional<std::string>& scripture_name
) const -> std::optional<std::vector<verse_t>>
{
  static constexpr auto references_of = [](const bible::reference_range& chapter)
  {
    const auto begin = chapter.begin();
    const auto to_reference = [begin](const std::uint32_t verse)
    { return bible::reference::create_unguarded(begin.book(), begin.chapter().value, verse); };
    return std::views::iota(begin.verse().value, chapter.end().verse().value + 1) | std::views::transform(to_reference);
  };
  const auto versification = workflow_scripture_->versification_or_fallback({{scripture_name}});
  auto result = std::vector<verse_t>{};
  const auto read = [&](const bible::reference& ref)
  {
    const auto passage = workflow_scripture_->passage(
      workflow_scripture::passage_params::value_type{.reference = ref, .scripture_name = scripture_name}
    );
    if(!passage)
    {
      LOG_WARN("nothing copied, a verse is missing: reference=\"{}\", error=\"{}\"", ref, passage.error());
      return false;
    }
    result.emplace_back(ref.verse(), passage->passage.plain());
    return true;
  };
  const auto chapters = versification.get().split_by_chapter(range);
  auto references = chapters | std::views::transform(references_of) | std::views::join;
  return std::ranges::all_of(references, read) ? std::make_optional(std::move(result)) : std::nullopt;
}

} // namespace bibstd::workflow
