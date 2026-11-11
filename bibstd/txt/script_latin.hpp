#pragma once

#include <array>
#include <string_view>

namespace bibstd::txt
{

///
/// Chars struct containing special char lists.
///
struct script_latin final
{
  // Constants
  ///
  /// All english alphabet letter chars in an array.
  ///
  static constexpr auto letters_en = std::array{
    std::string_view("A"), std::string_view("a"), std::string_view("B"), std::string_view("b"), std::string_view("C"),
    std::string_view("c"), std::string_view("D"), std::string_view("d"), std::string_view("E"), std::string_view("e"),
    std::string_view("F"), std::string_view("f"), std::string_view("G"), std::string_view("g"), std::string_view("H"),
    std::string_view("h"), std::string_view("I"), std::string_view("i"), std::string_view("J"), std::string_view("j"),
    std::string_view("K"), std::string_view("k"), std::string_view("L"), std::string_view("l"), std::string_view("M"),
    std::string_view("m"), std::string_view("N"), std::string_view("n"), std::string_view("O"), std::string_view("o"),
    std::string_view("P"), std::string_view("p"), std::string_view("Q"), std::string_view("q"), std::string_view("R"),
    std::string_view("r"), std::string_view("S"), std::string_view("s"), std::string_view("T"), std::string_view("t"),
    std::string_view("U"), std::string_view("u"), std::string_view("V"), std::string_view("v"), std::string_view("W"),
    std::string_view("w"), std::string_view("X"), std::string_view("x"), std::string_view("Y"), std::string_view("y"),
    std::string_view("Z"), std::string_view("z"),
  };

  ///
  /// All german alphabet letter chars in an array.
  ///
  static constexpr auto letters_de = std::array{
    std::string_view("A"), std::string_view("a"), std::string_view("B"), std::string_view("b"), std::string_view("C"),
    std::string_view("c"), std::string_view("D"), std::string_view("d"), std::string_view("E"), std::string_view("e"),
    std::string_view("F"), std::string_view("f"), std::string_view("G"), std::string_view("g"), std::string_view("H"),
    std::string_view("h"), std::string_view("I"), std::string_view("i"), std::string_view("J"), std::string_view("j"),
    std::string_view("K"), std::string_view("k"), std::string_view("L"), std::string_view("l"), std::string_view("M"),
    std::string_view("m"), std::string_view("N"), std::string_view("n"), std::string_view("O"), std::string_view("o"),
    std::string_view("P"), std::string_view("p"), std::string_view("Q"), std::string_view("q"), std::string_view("R"),
    std::string_view("r"), std::string_view("S"), std::string_view("s"), std::string_view("T"), std::string_view("t"),
    std::string_view("U"), std::string_view("u"), std::string_view("V"), std::string_view("v"), std::string_view("W"),
    std::string_view("w"), std::string_view("X"), std::string_view("x"), std::string_view("Y"), std::string_view("y"),
    std::string_view("Z"), std::string_view("z"), std::string_view("Ä"), std::string_view("ä"), std::string_view("Ö"),
    std::string_view("ö"), std::string_view("Ü"), std::string_view("ü"),
  };
};

} // namespace bibstd::txt
