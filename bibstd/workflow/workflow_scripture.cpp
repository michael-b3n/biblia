#include "bibstd/workflow/workflow_scripture.hpp"
#include "arca/util/contains.hpp"
#include "arca/util/enum.hpp"
#include "arca/util/exception.hpp"
#include "arca/util/log.hpp"
#include "arca/util/visit_helper.hpp"
#include "arca/workflow/workflow_settings.hpp"
#include "arca/workflow/workflow_settings_base.hpp"
#include "bibstd/bible/common.hpp"
#include "bibstd/bible/versification.hpp"
#include "bibstd/workflow/workflow_scripture_store.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <tuple>
#include <utility>
#include <vector>

namespace bibstd::workflow
{
namespace
{

///
/// \return the list validator of \p setting
///
[[nodiscard]] auto list_validator(const auto& setting)
{
  return std::get<framework::setting_validator_list<std::optional<std::string>>::sptr_type>(setting->validator);
}

} // namespace

///
///
workflow_scripture_settings::workflow_scripture_settings(std::shared_ptr<workflow_settings> workflow_settings)
  : workflow_settings_base{std::move(workflow_settings)}
  , default_scripture_name{workflow_settings_->create_setting(
      "scripture.default_name",
      setting_value_t<decltype(default_scripture_name)>{},
      std::make_shared<framework::setting_validator_list<setting_value_t<decltype(default_scripture_name)>>>()
    )}
  , fallback_versification{workflow_settings_->create_setting(
      "scripture.fallback_versification_name",
      std::string{bible::versification::default_kjv::name},
      std::make_shared<framework::setting_validator_list<setting_value_t<decltype(fallback_versification)>>>(
        workflow_scripture::default_versifications | std::views::keys |
        std::views::transform([](const auto& n) { return std::string{n}; }) | std::ranges::to<std::vector>()
      )
    )}
{
  // The default value set above must be one of the default versifications, else it would be invalid.
  static_assert(meta::contains_v<bible::versification::all_defaults_variant, bible::versification::default_kjv>);
}

///
///
workflow_scripture::versification_wrapper::versification_wrapper(std::shared_ptr<bible::scripture_outline> outline)
  : data_{std::move(outline)}
{
}

///
///
workflow_scripture::versification_wrapper::versification_wrapper(bible::versification versification)
  : data_{std::move(versification)}
{
}

///
///
auto workflow_scripture::versification_wrapper::get() const -> const bible::versification&
{
  return util::visit_lambdas(
    data_,
    [](const bible::versification& versification) -> const bible::versification& { return versification; },
    [](const std::shared_ptr<bible::scripture_outline>& outline) -> const bible::versification& { return outline->versification(); }
  );
}

///
///
workflow_scripture::workflow_scripture(
  std::shared_ptr<workflow_settings> workflow_settings, std::shared_ptr<workflow_script> workflow_script
)
  : workflow_base{workflow_settings}
  , workflow_script_{workflow_script}
  , workflow_scripture_store_{
      std::make_unique<workflow_scripture_store>(std::move(workflow_settings), std::move(workflow_script))
    }
{
  update_scripture_name_setting();

  workflow_script_->connect_queued(&workflow_script_sigs::scripts_loaded, [this]() { update_scripts(); }, connections_);

  static constexpr auto book_id_of = [](const std::string_view book_id) -> bible::book_id
  {
    const auto id = util::to_enum<bible::book_id>(book_id);
    if(!id)
    {
      throw util::exception{std::format("invalid book name: book=\"{}\"", book_id)};
    }
    return *id;
  };

  registrations_
    // Settings
    ///
    /// \lua The name of the default scripture, `Scripture > Default name`, `nil` for none. Functions called with `nil` as
    /// scripture name use it, so without a default scripture they find no scripture.
    ///
    << workflow_script_->register_setting(*settings().default_scripture_name)
    ///
    /// \lua The versification for scriptures that have none, including those of scripts,
    /// `Scripture > Fallback versification`.
    ///
    << workflow_script_->register_setting(*settings().fallback_versification)

    // Functions
    ///
    /// \lua
    /// \return the number of scriptures, including those of scripts
    ///
    << workflow_script_->register_function("scripture.scripture_count", [this]() { return scripture_count(); })
    ///
    /// \lua
    /// \return the identifiers of all books, e.g. `john`
    ///
    << workflow_script_->register_function(
         "scripture.book_identifiers",
         []() { return util::enum_names<bible::book_id>() | std::ranges::to<std::vector<std::string>>(); }
       )
    ///
    /// \lua
    /// \param scripture_name the scripture, `nil` for the default scripture
    /// \return its name, or `nil` if there is no such scripture
    ///
    << workflow_script_->register_function(
         "scripture.name",
         [this](const std::optional<std::string>& scripture_name)
         { return information({{scripture_name}}).transform([](const auto& info) { return info.name; }); }
       )
    ///
    /// \lua
    /// \param scripture_name the scripture, `nil` for the default scripture
    /// \return its abbreviation, or `nil` if there is no such scripture
    ///
    << workflow_script_->register_function(
         "scripture.abbreviation",
         [this](const std::optional<std::string>& scripture_name)
         { return information({{scripture_name}}).transform([](const auto& info) { return info.abbreviation; }); }
       )
    ///
    /// \lua
    /// \param scripture_name the scripture, `nil` for the default scripture
    /// \return its language, or `nil` if there is no such scripture
    ///
    << workflow_script_->register_function(
         "scripture.language",
         [this](const std::optional<std::string>& scripture_name)
         { return information({{scripture_name}}).transform([](const auto& info) { return info.language; }); }
       )
    ///
    /// \lua
    /// \param scripture_name the scripture, `nil` for the default scripture
    /// \return its copyright, or `nil` if it has none or there is no such scripture
    ///
    << workflow_script_->register_function(
         "scripture.copyright",
         [this](const std::optional<std::string>& scripture_name)
         { return information({{scripture_name}}).and_then([](const auto& info) { return info.copyright; }); }
       )
    ///
    /// \lua An unknown book raises an error.
    /// \param scripture_name the scripture, `nil` for the default scripture
    /// \param book_id the identifier of the book, e.g. `john`
    /// \return the abbreviation of the book, or `nil` if it has none or there is no such scripture
    ///
    << workflow_script_->register_function(
         "scripture.book_abbreviation",
         [this](const std::optional<std::string>& scripture_name, const std::string& book_id)
         {
           return book_information({{scripture_name}}, book_id_of(book_id))
             .transform([](const auto& names) { return names.abbreviation; });
         }
       )
    ///
    /// \lua An unknown book raises an error.
    /// \param scripture_name the scripture, `nil` for the default scripture
    /// \param book_id the identifier of the book, e.g. `john`
    /// \return the short name of the book, or `nil` if it has none or there is no such scripture
    ///
    << workflow_script_->register_function(
         "scripture.book_short_name",
         [this](const std::optional<std::string>& scripture_name, const std::string& book_id)
         {
           return book_information({{scripture_name}}, book_id_of(book_id))
             .transform([](const auto& names) { return names.short_name; });
         }
       )
    ///
    /// \lua An unknown book raises an error.
    /// \param scripture_name the scripture, `nil` for the default scripture
    /// \param book_id the identifier of the book, e.g. `john`
    /// \return the long name of the book, or `nil` if it has none or there is no such scripture
    ///
    << workflow_script_->register_function(
         "scripture.book_long_name",
         [this](const std::optional<std::string>& scripture_name, const std::string& book_id)
         {
           return book_information({{scripture_name}}, book_id_of(book_id))
             .transform([](const auto& names) { return names.long_name; });
         }
       )
    ///
    /// \lua A script that asks for a passage of its own scripture calls itself.
    /// \param scripture_name the scripture, `nil` for the default scripture
    /// \param book_id the identifier of the book, e.g. `john`
    /// \param chapter the chapter
    /// \param verse the verse
    /// \return the verse with the markup of the app, or `nil` and the reason, e.g. `scripture not found`
    ///
    << workflow_script_->register_function(
         "scripture.passage",
         [this](
           const std::optional<std::string>& scripture_name,
           const std::string& book_id,
           std::uint32_t chapter,
           std::uint32_t verse
         )
         {
           const auto reference = bible::reference::create_unguarded(book_id_of(book_id), chapter, verse);
           const auto params = passage_params_t{.reference = reference, .scripture_name = scripture_name};
           return passage(params).transform([](const auto& result) { return result.passage.xml(); });
         }
       );
}

///
///
workflow_scripture::~workflow_scripture() noexcept
{
  connections_.disconnect();
}

///
///
auto workflow_scripture::scripture_count() const -> std::size_t
{
  return scripture_names().size();
}

///
///
auto workflow_scripture::loaded() const -> bool
{
  const auto lock = std::scoped_lock{mtx_};
  return script_scriptures_.has_value();
}

///
///
auto workflow_scripture::information(const scripture_params& params) const -> std::optional<bible::scripture_info>
{
  using result_type = std::optional<bible::scripture_info>;
  try
  {
    const auto source = find(params->scripture_name);
    if(!source)
    {
      return std::nullopt;
    }
    return util::visit_lambdas(
      *source,
      [](const workflow_scripture_store::file_scripture& file) -> result_type { return file.outline->information(); },
      [this](const script_scriptures_t::value_type& entry) -> result_type
      {
        const auto output = run_script<information_manifest>(entry.second, {entry.first});
        if(!output)
        {
          return std::nullopt;
        }
        return bible::scripture_info{
          .name = entry.first,
          .abbreviation = output->get<"abbreviation">().value_or(""),
          .language = output->get<"language">().value_or(""),
          .copyright = output->get<"copyright">(),
        };
      }
    );
  }
  catch(...)
  {
    LOG_ERROR("exception occurred: {}", util::exception_report());
    return std::nullopt;
  }
}

///
///
auto workflow_scripture::book_information(const scripture_params& params, const bible::book_id book) const
  -> std::optional<bible::book_name>
{
  using result_type = std::optional<bible::book_name>;
  try
  {
    const auto source = find(params->scripture_name);
    if(!source)
    {
      return std::nullopt;
    }
    return util::visit_lambdas(
      *source,
      [book](const workflow_scripture_store::file_scripture& file) -> result_type
      { return file.outline->book_information(book); },
      [this, book](const script_scriptures_t::value_type& entry) -> result_type
      {
        const auto output = run_script<book_manifest>(entry.second, {entry.first, std::string{util::enum_name(book)}});
        if(!output || std::apply([](const auto&... names) { return !(names || ...); }, output->values()))
        {
          return std::nullopt;
        }
        return bible::book_name{
          .abbreviation = output->get<"abbreviation">().value_or(""),
          .short_name = output->get<"short_name">().value_or(""),
          .long_name = output->get<"long_name">().value_or(""),
        };
      }
    );
  }
  catch(...)
  {
    LOG_ERROR("exception occurred: {}", util::exception_report());
    return std::nullopt;
  }
}

///
///
auto workflow_scripture::versification_or_fallback(const scripture_params& params) const -> versification_wrapper_type
{
  const auto source = find(params->scripture_name);
  if(const auto* file = source ? std::get_if<workflow_scripture_store::file_scripture>(&*source) : nullptr)
  {
    return versification_wrapper_type{file->outline};
  }
  return versification_wrapper_type{default_versifications.at(settings().fallback_versification->value())};
}

///
///
auto workflow_scripture::passage(const passage_params& params) const -> passage_result
{
  static constexpr auto scripture_not_found = "scripture not found";
  static constexpr auto verse_not_found = "verse not found";
  static constexpr auto script_failed = "script failed";
  try
  {
    const auto& ref = params->reference;
    const auto source = find(params->scripture_name);
    if(!source)
    {
      return std::unexpected{std::string{scripture_not_found}};
    }
    // The reader of a file knows its scripture by the file name
    const auto scripture = util::visit_lambdas(
      *source,
      [](const workflow_scripture_store::file_scripture& file)
      { return script_scripture_t{.script = file.reader, .name = file.file_name}; },
      [](const script_scriptures_t::value_type& entry) { return entry.second; }
    );
    const auto output = run_script<passage_manifest>(
      scripture,
      {
        scripture.name,
        std::string{util::enum_name(ref.book())},
        static_cast<std::int64_t>(ref.chapter().value),
        static_cast<std::int64_t>(ref.verse().value),
      }
    );
    if(!output || !(output->get<"markup">() || output->get<"text">()))
    {
      // What the script reports, e.g. a web page it could not fetch
      const auto error = output ? output->get<"error">() : std::string{script_failed};
      return std::unexpected{error.value_or(std::string{verse_not_found})};
    }
    // The markup of a reader, the plain text of a web page, \see passage_manifest
    const auto& markup = output->get<"markup">();
    auto passage = markup ? bible::passage{ref, *markup, bible::passage_format::xml}
                          : bible::passage{ref, *output->get<"text">(), bible::passage_format::plain};
    passage.cross_references(output->get<"cross_references">().value_or(std::vector<std::string>{}));
    return passage_result_t{.passage = std::move(passage)};
  }
  catch(...)
  {
    auto report = util::exception_report();
    LOG_ERROR("exception occurred: {}", report);
    return std::unexpected{std::move(report)};
  }
}

///
///
auto workflow_scripture::import_scriptures(const import_params& params) -> std::size_t
{
  auto imported = std::size_t{0};
  try
  {
    imported = std::ranges::fold_left(
      params->sources | std::views::transform([this](const auto& source) { return workflow_scripture_store_->import(source); }),
      std::size_t{0},
      std::plus{}
    );
    if(imported != 0)
    {
      update_scripture_name_setting();
    }
  }
  catch(...)
  {
    LOG_ERROR("exception occurred: {}", util::exception_report());
  }
  return imported;
}

///
///
auto workflow_scripture::reload_scriptures() -> void
{
  try
  {
    workflow_scripture_store_->reload();
    update_scripture_name_setting();
  }
  catch(...)
  {
    LOG_ERROR("exception occurred: {}", util::exception_report());
  }
  notify(&signals_type::scriptures_changed);
}

///
///
auto workflow_scripture::scripture_names() const -> std::vector<std::string>
{
  const auto scriptures = workflow_scripture_store_->scriptures();
  auto names = scriptures | std::views::keys | std::ranges::to<std::vector>();
  const auto lock = std::scoped_lock{mtx_};
  if(script_scriptures_)
  {
    // A file shadows a script scripture of the same name, as in find
    names.append_range(
      *script_scriptures_ | std::views::keys | std::views::filter([&](const auto& n) { return !scriptures.contains(n); })
    );
  }
  return names;
}

///
///
auto workflow_scripture::selected_name(const std::optional<std::string>& name) const -> std::optional<std::string>
{
  return name ? name : settings().default_scripture_name->value();
}

///
///
auto workflow_scripture::find(const std::optional<std::string>& name) const -> std::optional<source_t>
{
  const auto selected = selected_name(name);
  if(!selected)
  {
    return std::nullopt;
  }
  const auto scriptures = workflow_scripture_store_->scriptures();
  if(const auto it = scriptures.find(*selected); it != std::ranges::end(scriptures))
  {
    return source_t{it->second};
  }
  const auto lock = std::scoped_lock{mtx_};
  if(!script_scriptures_)
  {
    return std::nullopt;
  }
  const auto it = script_scriptures_->find(*selected);
  return it != script_scriptures_->cend() ? std::optional{source_t{*it}} : std::nullopt;
}

///
///
template<script_manifest M>
auto workflow_scripture::run_script(const script_scripture_t& scripture, typename M::input input) const
  -> std::optional<typename M::output>
{
  // The script knows the scripture by its own name
  input.template get<"name">() = scripture.name;
  return workflow_script_->run<M>(scripture.script, input);
}

///
///
auto workflow_scripture::update_scripts() -> void
{
  // The name in the script, then the name of the script, e.g. "LUT (Bibleserver)"
  static constexpr auto script_scripture_name = "{} ({})";
  try
  {
    // The readers of the files are scripts too
    workflow_scripture_store_->load();
    const auto of_script = [this](const workflow_script::scripts_type::value_type& script)
    {
      const auto output = workflow_script_->run<names_manifest>(script.first, {});
      const auto names = output ? output->get<"names">() : std::vector<std::string>{};
      return names |
             std::views::transform(
               [&](const auto& name)
               {
                 return std::pair{
                   std::format(script_scripture_name, name, script.first), script_scripture_t{script.first, name}
                 };
               }
             ) |
             std::ranges::to<std::vector>();
    };
    const auto scripts = workflow_script_->scripts<names_manifest, information_manifest, book_manifest, passage_manifest>();
    auto scriptures = scripts | std::views::transform(of_script) | std::views::join | std::ranges::to<script_scriptures_t>();
    {
      const auto lock = std::scoped_lock{mtx_};
      script_scriptures_ = std::move(scriptures);
    }
    update_scripture_name_setting();
  }
  catch(...)
  {
    LOG_ERROR("exception occurred: {}", util::exception_report());
  }
  notify(&signals_type::scriptures_changed);
}

///
///
auto workflow_scripture::update_scripture_name_setting() -> void
{
  static constexpr auto has_kjv_versification = [](const auto& s)
  { return s.second.outline->versification() == bible::versification_kjv; };

  // Without the lock, the listeners of the setting may call back into the workflow
  const auto scriptures = workflow_scripture_store_->scriptures();
  const auto it = std::ranges::find_if(scriptures, has_kjv_versification);
  const auto preferred = it != std::ranges::cend(scriptures) ? std::make_optional(it->first) : std::nullopt;
  auto names = scripture_names();
  const auto scripts_loaded = loaded();
  const auto name = settings().default_scripture_name->value();
  if(!scripts_loaded && name)
  {
    // Until the scripts are loaded, the name may be one of theirs.
    // If not here, validator would drop the existing value.
    names.push_back(*name);
  }
  std::ignore = list_validator(settings().default_scripture_name)->available(names);

  // A setting naming a scripture that is not loaded leaves every lookup without one. That is what
  // a name left over from scriptures that are gone does, so it is pointed at a loaded scripture.
  const auto name_contained = name.has_value() && util::contains(names, *name);
  if(!(name_contained || names.empty()))
  {
    std::ignore = settings().default_scripture_name->value(preferred ? *preferred : names.front());
  }
}

} // namespace bibstd::workflow
