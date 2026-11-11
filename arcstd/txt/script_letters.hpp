#pragma once

#include "arcstd/txt/script_latin.hpp"
#include "arcstd/util/exception.hpp"
#include "arcstd/util/language.hpp"

#include <functional>
#include <variant>

namespace arcstd::txt
{

///
/// Chars struct containing special char lists.
///
struct script_letters final
{
  // Typedefs
  using letters_variant = std::variant<
    std::reference_wrapper<decltype(txt::script_latin::letters_en)>,
    std::reference_wrapper<decltype(txt::script_latin::letters_de)>>;

  // Static Methods
  ///
  /// Visit letters variant based on language.
  /// \return Result of the function call
  ///
  static constexpr auto visit(util::language language, const auto& function) -> auto;
};

///
///
constexpr auto script_letters::visit(const util::language language, const auto& function) -> auto
{
  const auto letters_var = [&]() -> letters_variant
  {
    switch(language)
    {
    case util::language::german: return std::cref(txt::script_latin::letters_de);
    case util::language::english: return std::cref(txt::script_latin::letters_en);
    default: throw util::exception{"unsupported language"};
    }
  }();
  return std::visit([&](const auto& letters) { return function(letters.get()); }, letters_var);
}

} // namespace arcstd::txt
