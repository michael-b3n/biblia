#pragma once

#include "arcstd/util/exception.hpp"
#include "bibocr/util/language.hpp"
#include "bibstd/txt/script_latin.hpp"

#include <functional>
#include <variant>

namespace bibstd::txt
{

///
/// Chars struct containing special char lists.
///
struct script_letters final
{
  // Typedefs
  using letters_variant = std::variant<
    std::reference_wrapper<decltype(bibstd::txt::script_latin::letters_en)>,
    std::reference_wrapper<decltype(bibstd::txt::script_latin::letters_de)>>;

  // Static Methods
  ///
  /// Visit letters variant based on language.
  /// \return Result of the function call
  ///
  static constexpr auto visit(bibocr::util::language language, const auto& function) -> auto;
};

///
///
constexpr auto script_letters::visit(const bibocr::util::language language, const auto& function) -> auto
{
  const auto letters_var = [&]() -> letters_variant
  {
    switch(language)
    {
    case bibocr::util::language::german: return std::cref(bibstd::txt::script_latin::letters_de);
    case bibocr::util::language::english: return std::cref(bibstd::txt::script_latin::letters_en);
    default: throw arcstd::util::exception{"unsupported language"};
    }
  }();
  return std::visit([&](const auto& letters) { return function(letters.get()); }, letters_var);
}

} // namespace bibstd::txt
