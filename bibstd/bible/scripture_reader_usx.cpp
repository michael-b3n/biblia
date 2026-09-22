#include "bibstd/bible/scripture_reader_usx.hpp"
#include "bibstd/bible/common.hpp"
#include "bibstd/bible/reference.hpp"
#include "bibstd/bible/scripture_types.hpp"
#include "bibstd/io/zip_file_reader.hpp"
#include "bibstd/txt/script_common.hpp"
#include "bibstd/util/const_map.hpp"
#include "bibstd/util/contains.hpp"
#include "bibstd/util/enum.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/log.hpp"
#include "bibstd/util/string.hpp"
#include "bibstd/util/timer.hpp"
#include "bibstd/util/uid.hpp"

#include <pugixml.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace bibstd::bible
{
namespace
{

///
/// \return text of the node tree, stripped of all markup
///
auto text_of(const pugi::xml_node& node) -> std::string
{
  auto result = std::string{};
  std::ranges::for_each(
    node.children(),
    [&result](const auto& child)
    {
      const auto type = child.type();
      if(type == pugi::node_pcdata || type == pugi::node_cdata)
      {
        result.append(child.value());
      }
      else if(type == pugi::node_element)
      {
        result.append(text_of(child));
      }
    }
  );
  return result;
}

///
/// The whitespace of a scripture is the UTF-8 one, not the ASCII one, so the text is classified by
/// txt::script_common. Telling whitespace from the rest of it needs no letters.
/// \return the text without its leading and trailing whitespace, a view of the given one
///
auto trimmed(const std::string_view text) -> std::string_view
{
  static constexpr auto no_letters = std::array<std::string_view, 0>{};
  auto begin = text.size();
  auto end = std::size_t{0};
  txt::script_common::for_each_char(
    no_letters,
    text,
    [&](const auto character, const auto position, const auto category)
    {
      if(category == txt::script_common::category::whitespace)
      {
        return;
      }
      begin = std::min(begin, position); // the first character that is none bounds the result
      end = position + character.size();
    }
  );
  return begin < end ? text.substr(begin, end - begin) : std::string_view{};
}

///
/// Looks entries up in the archive a bundle ships in.
///
struct archive_entries final
{
  ///
  /// A bundle may hold the same file name at several depths, "metadata.xml" sits both in the bundle
  /// root and in the "release" directory, and only the root document carries the complete metadata.
  /// Looking the name up directly would resolve to whichever comes first, so the shallowest match is
  /// taken.
  /// \return The matching entry closest to the archive root, or std::nullopt if not found
  ///
  static auto find_root(const io::zip_file_reader& archive, const std::string_view file_name)
    -> std::optional<io::zip_file_reader::zip_entry>
  {
    static constexpr auto separator = '/';
    static constexpr auto base_name = [](const std::string_view name)
    {
      const auto pos = name.rfind(separator);
      return pos == std::string_view::npos ? name : name.substr(pos + 1);
    };
    static constexpr auto equal_ignoring_case = [](const std::string_view lhs, const std::string_view rhs)
    {
      static constexpr auto lower = [](const char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); };
      return std::ranges::equal(lhs, rhs, [](const char l, const char r) { return lower(l) == lower(r); });
    };

    const auto entries = archive.entries();
    auto candidates =
      entries | std::views::filter([&](const auto& entry)
                                   { return entry.name && equal_ignoring_case(base_name(*entry.name), file_name); });

    const auto found =
      std::ranges::min_element(candidates, {}, [](const auto& entry) { return std::ranges::count(*entry.name, separator); });
    if(found == std::ranges::end(candidates))
    {
      return std::nullopt;
    }
    return *found;
  }

  ///
  /// \return content of the named archive entry, or std::nullopt if the archive holds no such entry
  ///
  static auto load(const io::zip_file_reader& archive, const std::string& entry_name) -> std::optional<std::string>
  {
    using query_flag = io::zip_file_reader::query_flag;
    const auto data = archive.entry(entry_name, {query_flag::exclude_directories, query_flag::case_insensitive});
    if(!data)
    {
      LOG_ERROR("failed to load entry: expected \"{}\" file within archive", entry_name);
      return std::nullopt;
    }
    return archive.read_entry_as_string(*data);
  }
};

///
/// Tree walker for finding nodes with a certain depth. The walker is initialized with criteria paths,
/// which are paths in the XML tree that specify which nodes to find.
///
class node_path_finder_walker : public pugi::xml_tree_walker
{
  // Typedefs
  using string_matrix_type = std::vector<std::vector<std::string>>;

  struct criteria_data final
  {
    bool starts_with_wildcard = false;
    typename string_matrix_type::value_type path_sections;
  };

  struct walker_data final
  {
    criteria_data criteria;
    std::map<int, std::vector<pugi::xml_node>> found_nodes;
  };

  // Constants
  static constexpr std::string_view wildcard = "...";
  static constexpr char section_delimiter = '/';
  static constexpr auto is_wildcard = [](const std::string_view data) { return data == wildcard; };

  // Variables
  std::vector<walker_data> data_;
  decltype(walker_data::found_nodes) found_nodes_;

public: // Typedefs
  using result_type = decltype(found_nodes_);
  using string_list_type = typename string_matrix_type::value_type;

public: // Structors
  ///
  /// Construct a node depth finder walker with the given criteria path to match nodes against.
  /// Multiple criteria paths can be provided, the first matching path will be used. Each criteria
  /// path is a string that represents a path in the XML tree, with sections separated by '/'.
  /// The path can contain wildcards ("...").
  ///
  node_path_finder_walker(const auto& criteria_paths)
    : data_{parse_criteria(criteria_paths)}
  {
  }

public: // Accessors
  ///
  /// Get the found nodes grouped by their depth in the XML tree.
  /// \return A map where the key is the depth and the value is a vector of XML nodes found at that depth.
  ///
  auto found() const -> const result_type& { return found_nodes_; }

private: // Implementation
  static auto parse_criteria(const auto& criteria_paths) -> std::vector<walker_data>
  {
    auto result = std::vector<walker_data>{};
    std::ranges::for_each(
      criteria_paths,
      [&](const auto& criteria_path)
      {
        const auto sections = bibstd::util::string::split(criteria_path, section_delimiter);
        if(sections.empty())
        {
          throw util::exception(std::format(R"(invalid criteria path: reason="empty criteria", path="{}")", criteria_path));
        }
        if(is_wildcard(sections.back()))
        {
          throw util::exception(
            std::format(R"(invalid criteria path: reason="cannot end with wildcard", path="{}")", criteria_path)
          );
        }
        result.emplace_back(criteria_data{is_wildcard(sections.front()), parse_path_sections(criteria_path)});
      }
    );
    return result;
  }

  static auto parse_path_sections(const std::string_view criteria_path) -> string_list_type
  {
    auto path_sections = bibstd::util::string::split(criteria_path, wildcard);

    std::ranges::for_each(
      path_sections | std::views::filter([](const auto& section) { return !section.empty(); }),
      [&](auto& element)
      {
        if(util::string::starts_with(element, section_delimiter))
        {
          element = element.substr(1);
        }
        if(util::string::ends_with(element, section_delimiter))
        {
          element.pop_back();
        }
      }
    );
    std::erase_if(path_sections, [](const auto& section) { return section.empty(); });
    return path_sections;
  }

  static auto matches_criteria(const pugi::xml_node& node, const criteria_data& criteria) -> bool
  {
    if(criteria.path_sections.empty())
    {
      return false;
    }
    decltype(auto) path = node.path();
    auto checker = [&path, pos = decltype(std::string::npos){0}](const auto& path_section) mutable
    {
      const auto found_pos = path.find(path_section, pos);
      const auto found = found_pos != std::string::npos;
      if(found)
      {
        pos = found_pos + path_section.size();
      }
      return found;
    };

    decltype(auto) front = criteria.path_sections.front();
    auto result = checker(front);
    if(criteria.starts_with_wildcard)
    {
      result = util::string::starts_with(path, std::format("{}{}", section_delimiter, front));
    }
    auto rest = criteria.path_sections | std::views::drop(1);
    return result && std::ranges::all_of(rest, [&](const auto& path_section) { return checker(path_section); });
  }

public: // Overrides
  auto for_each(pugi::xml_node& node) -> bool override
  {
    std::ranges::for_each(
      data_ | std::views::filter([&](const auto& element) { return matches_criteria(node, element.criteria); }),
      [&](auto& d) { d.found_nodes[depth()].push_back(node); }
    );
    return true;
  }

  auto end([[maybe_unused]] pugi::xml_node& /*node*/) -> bool override
  {
    std::ranges::for_each(
      data_ | std::views::take_while([&]([[maybe_unused]] const auto&) { return found_nodes_.empty(); }),
      [&](auto& data) { found_nodes_ = std::move(data.found_nodes); }
    );
    return true;
  }
};

///
/// The descriptor that identifies a USX bundle and carries the information about its scripture.
///
struct metadata final
{
  // Constants
  static constexpr auto file_name = std::string_view{"metadata.xml"};

  ///
  /// Load and parse the descriptor sitting in the root of the bundle.
  /// \return true if the document was loaded and parsed, false otherwise
  ///
  static auto load(const io::zip_file_reader& archive, pugi::xml_document& doc) -> bool
  {
    const auto entry = archive_entries::find_root(archive, file_name);
    if(!entry)
    {
      LOG_ERROR("failed to load entry: expected \"{}\" file within archive", file_name);
      return false;
    }
    const auto data = archive.read_entry_as_string(*entry);
    const auto parse_result = doc.load_string(data.c_str());
    if(!parse_result)
    {
      LOG_ERROR("failed to parse \"{}\": {}", file_name, parse_result.description());
      return false;
    }
    return true;
  }

  ///
  /// Read the text content of the node that the first matching criteria path names.
  /// \return Content of the matching node, or std::nullopt if none of the criteria paths matched
  ///
  static auto read_content(const pugi::xml_document& doc, const auto& criteria_paths) -> std::optional<std::string>
  {
    auto root = doc.root();
    auto walker = node_path_finder_walker{criteria_paths};
    root.traverse(walker);

    auto result = std::optional<std::string>{};
    std::ranges::for_each(
      walker.found() | std::views::values | std::views::filter([](const auto& e) { return !e.empty(); }) | std::views::take(1),
      [&](const auto& e) { result = text_of(e.front()); }
    );
    if(!result)
    {
      LOG_ERROR(
        "expected node in \"{}\" not found: criteria_paths=\"{}\"", file_name, bibstd::util::string::join(criteria_paths, ", ")
      );
    }
    return result;
  }
};

///
/// The style a USX paragraph is marked with. A paragraph either carries scripture text or
/// introduces some.
///
struct paragraph_style final
{
  // Constants
  ///
  /// Styles carrying the name of a book: "h" is the running header, "toc1" to "toc3" are the table
  /// of contents entries and "mt.." is the major title.
  ///
  // clang-format off
  static constexpr auto book_names = util::string::to_string_view_array("h", "toc1", "toc2", "toc3", "mt", "mt1", "mt2", "mt3");
  // clang-format on

  ///
  /// Book name styles carrying each of the name forms, most preferred first. Not every scripture
  /// ships all of them, so every form falls back to the closest alternative.
  ///
  // clang-format off
  static constexpr auto abbreviations = util::string::to_string_view_array("toc3", "h", "toc2");
  static constexpr auto short_names = util::string::to_string_view_array("toc2", "h", "toc1");
  static constexpr auto long_names = util::string::to_string_view_array("toc1", "mt1", "mt", "toc2");
  // clang-format on

  ///
  /// Styles carrying no scripture text: the header of a book, the headings between its sections
  /// together with their reference lines, the title of a psalm, the label of a chapter and editorial
  /// remarks. The introduction of a book is left out here, all of its styles begin with "i".
  ///
  // clang-format off
  static constexpr auto text_free = util::string::to_string_view_array("h", "toc", "mt", "s", "ms", "sd", "sp", "r", "sr", "mr", "d", "cl", "cd", "rem");
  // clang-format on

  ///
  /// \return true if the style carries the name of the book
  ///
  static auto is_book_name(const std::string_view style) -> bool { return util::contains(book_names, style); }

  ///
  /// Taking a paragraph without scripture text over would append it to the verse before it.
  /// \return true if the style carries no scripture text
  ///
  static auto is_text_free(const std::string_view style) -> bool
  {
    return util::string::starts_with(style, 'i') ||
           std::ranges::any_of(text_free, [style](const auto base) { return is_leveled(style, base); });
  }

  ///
  /// USX numbers the levels of a style, "s1" and "s2" are both section headings.
  /// \return true if the style is the given one, at any level
  ///
  static auto is_leveled(const std::string_view style, const std::string_view base) -> bool
  {
    static constexpr auto levels = std::string_view{"0123456789"};
    return style.starts_with(base) && style.find_first_not_of(levels, base.size()) == std::string_view::npos;
  }
};

///
/// Turns the content of a USX node into the markup of a passage, \see bible::passage_markup.
///
struct markup final
{
  ///
  /// \return text with the characters that carry meaning in XML replaced by their entity
  ///
  static auto escaped(const std::string_view text) -> std::string
  {
    static constexpr auto entities = util::make_const_map<char, std::string_view>({
      {'&', "&amp;"},
      {'<',  "&lt;"},
      {'>',  "&gt;"}
    });
    auto result = std::string{};
    result.reserve(text.size());
    std::ranges::for_each(
      text,
      [&result](const auto character)
      {
        if(entities.contains(character))
        {
          result.append(entities.at(character));
        }
        else
        {
          result.push_back(character);
        }
      }
    );
    return result;
  }

  ///
  /// \return markup node a USX character style is kept as, or std::nullopt if there is none for it
  ///
  static auto node_of(const std::string_view char_style) -> std::optional<std::string_view>
  {
    static constexpr auto nodes = util::make_const_map<std::string_view, std::string_view>({
      { "bd",                passage_markup::bold},
      { "it",              passage_markup::italic},
      { "em",              passage_markup::italic},
      { "nd",         passage_markup::name_of_god},
      {"add", passage_markup::translator_addition}
    });
    return nodes.contains(char_style) ? std::make_optional(nodes.at(char_style)) : std::nullopt;
  }

  ///
  /// A character style that has no markup node contributes its text only, notes are no part of the
  /// verse text and are left out.
  /// \return markup of the node content
  ///
  static auto serialize(const pugi::xml_node& node) -> std::string
  {
    auto result = std::string{};
    std::ranges::for_each(
      node.children(),
      [&result](const auto& child)
      {
        const auto type = child.type();
        if(type == pugi::node_pcdata || type == pugi::node_cdata)
        {
          result.append(escaped(child.value()));
        }
        else if(type != pugi::node_element)
        {
          return;
        }
        else if(const auto name = std::string_view{child.name()}; name == "char")
        {
          const auto inner = serialize(child);
          const auto markup_node = node_of(std::string_view{child.attribute("style").value()});
          result.append(markup_node ? std::format("<{0}>{1}</{0}>", *markup_node, inner) : inner);
        }
        else if(name != "note")
        {
          result.append(serialize(child));
        }
      }
    );
    return result;
  }
};

///
/// Collects the passages of a book out of its USX document. USX marks a verse with a milestone
/// instead of nesting it, so a verse reaches from its marker up to the next one and may span
/// several paragraphs.
///
class verse_collector final
{
  // Typedefs
  using paragraph_id_type = util::uid<struct paragraph_id_tag>;

  ///
  /// The part of a verse that sits inside one paragraph.
  ///
  struct segment final
  {
    std::optional<paragraph_id_type> paragraph; // std::nullopt for content outside of a paragraph
    std::string_view position;
    std::string content;
  };

  // Variables
  const book_id book_;
  scripture::passage_map_type passages_;
  std::vector<segment> segments_;
  std::vector<std::string> cross_references_;
  std::optional<reference::chapter_type> chapter_;
  std::optional<reference::verse_type> verse_;
  std::optional<paragraph_id_type> paragraph_;
  bool paragraph_filled_{false};

public: // Structors
  explicit verse_collector(const book_id book)
    : book_{book}
  {
  }

public: // Modifiers
  ///
  /// Walk the document of the book and collect the passage of every verse it carries.
  /// \return passages of the book
  ///
  [[nodiscard]] auto collect(const pugi::xml_node& usx_node) -> scripture::passage_map_type
  {
    std::ranges::for_each(usx_node.children(), [this](const auto& child) { visit(child); });
    flush();
    return std::move(passages_);
  }

private: // Implementation
  ///
  /// A chapter and a verse are marked by a milestone carrying their number, the milestone ending
  /// them carries none.
  /// \return number the marker carries, or std::nullopt if it ends a chapter or a verse
  ///
  static auto marker_number(const pugi::xml_node& marker) -> std::optional<std::uint32_t>
  {
    const auto number = marker.attribute("number");
    if(number.empty())
    {
      return std::nullopt;
    }
    return number.as_uint();
  }

  ///
  /// \return cross reference texts of a note of the style "x"
  ///
  static auto cross_references_of(const pugi::xml_node& note) -> std::vector<std::string>
  {
    static constexpr auto is_cross_reference = [](const pugi::xml_node& node)
    {
      return node.type() == pugi::node_element && std::string_view{node.name()} == "char" &&
             std::string_view{node.attribute("style").value()} == "xt";
    };
    auto result = note.children() | std::views::filter(is_cross_reference) |
                  std::views::transform([](const auto& node) { return text_of(node); }) |
                  std::ranges::to<std::vector<std::string>>();
    std::erase_if(result, [](const auto& text) { return text.empty(); });
    return result;
  }

  auto visit(const pugi::xml_node& node) -> void
  {
    const auto type = node.type();
    if(type == pugi::node_pcdata || type == pugi::node_cdata)
    {
      append(markup::escaped(node.value()));
    }
    else if(type == pugi::node_element)
    {
      if(std::string_view{node.name()} != "para")
      {
        visit_element(node);
      }
      // a paragraph carrying no scripture text introduces what follows it, it is no content of a verse
      else if(!paragraph_style::is_text_free(std::string_view{node.attribute("style").value()}))
      {
        visit_paragraph(node);
      }
    }
  }

  auto visit_element(const pugi::xml_node& element) -> void
  {
    const auto name = std::string_view{element.name()};
    if(name == "chapter")
    {
      // the marker of a chapter ends the verse before it
      if(const auto number = marker_number(element))
      {
        flush();
        chapter_ = reference::chapter_type{*number};
        verse_ = std::nullopt;
      }
    }
    else if(name == "verse")
    {
      if(const auto number = marker_number(element))
      {
        flush();
        verse_ = reference::verse_type{*number};
      }
    }
    else if(name == "note")
    {
      if(std::string_view{element.attribute("style").value()} == "x")
      {
        std::ranges::move(cross_references_of(element), std::back_inserter(cross_references_));
      }
    }
    else
    {
      append(markup::serialize(element));
    }
  }

  auto visit_paragraph(const pugi::xml_node& paragraph) -> void
  {
    paragraph_ = paragraph_id_type{};
    paragraph_filled_ = false;
    std::ranges::for_each(paragraph.children(), [this](const auto& child) { visit(child); });
    paragraph_ = std::nullopt;
  }

  auto append(std::string content) -> void
  {
    if(!chapter_ || !verse_ || content.empty())
    {
      return;
    }
    if(!segments_.empty() && segments_.back().paragraph == paragraph_)
    {
      segments_.back().content.append(content);
      return;
    }
    if(trimmed(content).empty())
    {
      // the whitespace a document is laid out with is no content of its own
      return;
    }
    // the verse that puts the first content into a paragraph opens it, the ones after it continue it
    auto position = passage_markup::paragraph_undefined;
    if(paragraph_)
    {
      position = paragraph_filled_ ? passage_markup::paragraph_continue : passage_markup::paragraph_begin;
      paragraph_filled_ = true;
    }
    segments_.push_back(segment{.paragraph = paragraph_, .position = position, .content = std::move(content)});
  }

  auto flush() -> void
  {
    if(chapter_ && verse_)
    {
      auto content = std::string{};
      for(const auto& segment : segments_)
      {
        content.append(
          std::format(
            R"(<{0} {1}="{2}">{3}</{0}>)",
            passage_markup::paragraph,
            passage_markup::paragraph_attribute,
            segment.position,
            segment.content
          )
        );
      }
      if(!content.empty())
      {
        const auto ref = reference::create_unguarded(book_, *chapter_, *verse_);
        passages_.emplace(
          ref, passage{.ref = ref, .content = std::move(content), .cross_references = std::move(cross_references_)}
        );
      }
    }
    // a verse without content is no passage, its cross references go with it
    segments_.clear();
    cross_references_.clear();
  }
};

///
/// Read the names of the book from the header paragraphs preceding its first chapter.
/// \return Names of the book, forms without a matching header paragraph are empty
///
auto parse_name(const pugi::xml_node& usx_node) -> book_name
{
  auto headers = std::map<std::string, std::string, std::less<>>{};

  const auto is_not_chapter_node = [](const auto& node) { return std::string_view{node.name()} != "chapter"; };
  // the header block introduces the book and therefore ends where its text begins
  for(auto child : usx_node.children() | std::views::take_while(is_not_chapter_node))
  {
    const auto style = std::string_view{child.attribute("style").value()};
    if(std::string_view{child.name()} == "para" && paragraph_style::is_book_name(style))
    {
      const auto text = text_of(child);
      if(const auto name = trimmed(text); !name.empty())
      {
        // a style repeated further down does not override the first occurrence
        headers.emplace(style, name);
      }
    }
  }

  const auto preferred = [&headers](const auto& styles)
  {
    for(const auto style : styles)
    {
      if(const auto found = headers.find(style); found != std::cend(headers))
      {
        return found->second;
      }
    }
    return std::string{};
  };
  return book_name{
    .abbreviation = preferred(paragraph_style::abbreviations),
    .short_name = preferred(paragraph_style::short_names),
    .long_name = preferred(paragraph_style::long_names)
  };
}

} // namespace

///
///
auto scripture_reader_usx::name() const -> name_type
{
  return "USX";
}

///
///
auto scripture_reader_usx::recognizes(const io::zip_file_reader& archive) const -> bool
{
  return archive_entries::find_root(archive, metadata::file_name).has_value();
}

///
///
auto scripture_reader_usx::read(const io::zip_file_reader& archive) const -> std::unique_ptr<scripture>
{
  SCOPED_TIMER_LOG();
  auto info_data = parse_metadata(archive);
  if(!info_data)
  {
    LOG_ERROR("failed to load scripture information data");
    return nullptr;
  }
  auto content = load_books(archive);
  if(!content)
  {
    LOG_ERROR("failed to load scripture book data");
    return nullptr;
  }
  LOG_INFO(
    "loaded scripture: name=\"{}\", abbreviation=\"{}\", language=\"{}\", copyright=\"{}\", verses={}, named_books={}",
    info_data->name,
    info_data->abbreviation,
    info_data->language,
    info_data->copyright.value_or("not found"),
    content->passages.size(),
    content->book_names.size()
  );
  return std::make_unique<scripture>(std::move(*info_data), std::move(content->book_names), std::move(content->passages));
}

///
///
auto scripture_reader_usx::parse_metadata(const io::zip_file_reader& archive) -> std::optional<scripture_info>
{
  SCOPED_TIMER_LOG();
  // clang-format off
  static constexpr auto name_paths = std::array{"/.../identification/nameLocal", "/.../identification/name", "/.../name"};
  static constexpr auto abbreviation_paths = std::array{"/.../identification/abbreviationLocal", "/.../identification/abbreviation", "/.../abbreviation"};
  static constexpr auto language_paths = std::array{"/.../language/nameLocal", "/.../language/name", "/.../language"};
  static constexpr auto copyright_paths = std::array{"/.../copyright/.../statementContent", "/.../copyright"};
  // clang-format on
  pugi::xml_document doc;
  if(!metadata::load(archive, doc))
  {
    return std::nullopt;
  }
  return scripture_info{
    .name = metadata::read_content(doc, name_paths).value_or(unknown_name),
    .abbreviation = metadata::read_content(doc, abbreviation_paths).value_or(unknown_abbreviation),
    .language = metadata::read_content(doc, language_paths).value_or(unknown_language),
    .copyright = metadata::read_content(doc, copyright_paths)
  };
}

///
///
auto scripture_reader_usx::parse_book_document(const book_id book, const std::string& usx_content)
  -> std::optional<book_document>
{
  pugi::xml_document doc;
  const auto parse_result = doc.load_string(usx_content.c_str(), pugi::parse_default | pugi::parse_ws_pcdata);
  if(!parse_result)
  {
    LOG_ERROR("failed to parse USX content for {}: {}", util::enum_name(book), parse_result.description());
    return std::nullopt;
  }

  const auto usx_node = doc.child("usx");
  if(!usx_node)
  {
    LOG_ERROR("no <usx> root element found for {}", util::enum_name(book));
    return std::nullopt;
  }
  return book_document{.name = parse_name(usx_node), .passages = verse_collector{book}.collect(usx_node)};
}

///
///
auto scripture_reader_usx::load_books(const io::zip_file_reader& archive) -> std::optional<scripture_content>
{
  try
  {
    auto result = scripture_content{};
    for(const auto& [id, abbreviation] : books)
    {
      const auto usx_content = archive_entries::load(archive, std::format("{}.usx", abbreviation));
      if(!usx_content.has_value() || usx_content->empty())
      {
        LOG_ERROR("failed to load \"{}\" data: expected \"{}.usx\" file within archive", util::enum_name(id), abbreviation);
        return std::nullopt;
      }
      auto document = parse_book_document(id, *usx_content);
      if(!document)
      {
        return std::nullopt;
      }
      result.book_names.emplace(id, std::move(document->name));
      result.passages.merge(document->passages);
    }
    return result;
  }
  catch(...)
  {
    LOG_ERROR("exception while loading book data: {}", util::exception_report());
    return std::nullopt;
  }
}

} // namespace bibstd::bible
