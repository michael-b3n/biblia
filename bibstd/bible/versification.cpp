#include "bibstd/bible/versification.hpp"
#include "bibstd/bible/common.hpp"
#include "bibstd/bible/reference.hpp"
#include "bibstd/bible/versification_common.hpp"
#ifdef BIBSTD_DEBUG_DATA
  #include "arca/util/log.hpp"
#endif

#include <algorithm>
#ifdef BIBSTD_DEBUG_DATA
  #include <fstream>
#endif
#include <ranges>
#include <string_view>

namespace bibstd::bible
{

///
///
versification::versification(const std::string_view name, const std::vector<reference>& references)
  : data_{
      [&]
      {
        auto data = [&]()
        {
          auto refs = references;
          auto result = decltype(refs){};
          result.reserve(refs.size());
          std::ranges::sort(refs, std::greater{});
          const auto ret =
            std::ranges::unique(refs, std::equal_to{}, [](const auto& r) { return std::make_tuple(r.book(), r.chapter()); });
          refs.erase(std::ranges::begin(ret), std::ranges::end(ret));
          std::ranges::reverse(refs);
          refs.shrink_to_fit();
          return refs;
        }();
        auto book_start_indices = versification_common::generate_book_start_indices(data);
        const auto count =
          std::ranges::fold_left(data, std::uint32_t{0}, [](const auto n, const auto& r) { return n + r.verse().value; });
        return versification_dynamic{
          .name{name}, .data{std::move(data)}, .book_start_indices{book_start_indices}, .reference_count = count
        };
      }()
    }
{
  if(!std::ranges::all_of(visit_book_start_indices(data_), [size = visit_size(data_)](const auto i) { return i < size; }))
  {
    throw util::exception{"invalid versification data: all book start indices must be smaller than the size of the data"};
  }
#ifdef BIBSTD_DEBUG_DATA
  const auto versification_found = [&]()
  {
    using info_t = meta::pack_info<all_defaults_variant>;
    return [&]<std::size_t... I>(std::index_sequence<I...>)
    { return ((versification{info_t::type_at<I>{}} == *this) || ...); }(std::make_index_sequence<info_t::size>{});
  }();

  if(versification_found)
  {
    LOG_DEBUG("versification data matches default versification: \"{}\"", name);
    return;
  }
  static constexpr auto nl = "\n";
  std::ofstream versification_out(std::format("dd_versification_{}.hpp", name), std::ios::out | std::ios::trunc);
  // clang-format off
  versification_out
    << "#pragma once" << nl
    << nl
    << "#include \"bibstd/bible/versification_common.hpp\"" << nl
    << nl
    << "#include <algorithm>" << nl
    << nl
    << "namespace bibstd::bible" << nl
    << "{" << nl << nl
    << "///" << nl
    << "/// Default versification data for " << name << "." << nl
    << "///" << nl
    << "struct versification_default_" << name << " final" << nl
    << "{" << nl
    << "  // Constants" << nl
    << "  static constexpr std::string_view name = \"" << name << " Versification\";" << nl
    << "  static constexpr auto data = std::array{" << nl;
  // clang-format on
  std::visit(
    [&](const auto& d)
    {
      std::ranges::for_each(
        d.data,
        [&](const auto& r)
        {
          versification_out << "    reference::create_unguarded(book_id::" << util::enum_name(r.book()) << ", "
                            << r.chapter().value << ", " << r.verse().value << ")," << nl;
        }
      );
    },
    data_
  );
  // clang-format off
  versification_out
    << "  };" << nl
    << "  static constexpr auto reference_count = std::ranges::fold_left(data, std::uint32_t{0}, [](const auto n, const auto& r) { return n + r.verse().value; });" << nl
    << "  static constexpr auto book_start_indices = versification_common::generate_book_start_indices(data);" << nl
    << nl
    << "  // Checks" << nl
    << "  static_assert(std::ranges::all_of(book_start_indices, [](const auto i) { return i < data.size(); }));" << nl
    << "};" << nl
    << nl
    << "} // namespace bibstd::bible" << nl;
  // clang-format on
  versification_out.close();
#endif
}

} // namespace bibstd::bible
