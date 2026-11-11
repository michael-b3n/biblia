#include <arcstd/util/ranges.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <vector>

namespace bibstd::util::ranges
{
namespace
{

///
/// Collect a view into a vector so it can be compared against an expected sequence.
///
constexpr auto to_vector(const auto& view) -> std::vector<std::size_t>
{
  auto result = std::vector<std::size_t>{};
  for(const auto i : view)
  {
    result.push_back(static_cast<std::size_t>(i));
  }
  return result;
}

} // namespace

TEST_CASE("index_view", "[util]")
{
  static constexpr auto values = std::array{1, 2, 3};
  CHECK(to_vector(index_view(values)) == std::vector<std::size_t>{0, 1, 2});
  CHECK(to_vector(index_view(std::array<int, 0>{})).empty());
  CHECK(to_vector(index_view(std::vector{1, 2, 3, 4})) == std::vector<std::size_t>{0, 1, 2, 3});
}

TEST_CASE("index_view_from", "[util]")
{
  static constexpr auto values = std::array{1, 2, 3, 4};
  CHECK(to_vector(index_view_from(values, 0)) == std::vector<std::size_t>{0, 1, 2, 3});
  CHECK(to_vector(index_view_from(values, 2)) == std::vector<std::size_t>{2, 3});
  CHECK(to_vector(index_view_from(values, 4)).empty());
  // A begin index beyond the range size yields an empty view instead of an invalid one.
  CHECK(to_vector(index_view_from(values, 10)).empty());
}

TEST_CASE("index_view_to with range", "[util]")
{
  static constexpr auto values = std::array{1, 2, 3, 4};
  CHECK(to_vector(index_view_to(values, 0)).empty());
  CHECK(to_vector(index_view_to(values, 2)) == std::vector<std::size_t>{0, 1});
  CHECK(to_vector(index_view_to(values, 4)) == std::vector<std::size_t>{0, 1, 2, 3});
  // An end index beyond the range size is clamped to the range size.
  CHECK(to_vector(index_view_to(values, 10)) == std::vector<std::size_t>{0, 1, 2, 3});
}

TEST_CASE("index_view_to with end index", "[util]")
{
  CHECK(to_vector(index_view_to(0)).empty());
  CHECK(to_vector(index_view_to(3)) == std::vector<std::size_t>{0, 1, 2});
  CHECK(to_vector(index_view_to(std::size_t{3})) == std::vector<std::size_t>{0, 1, 2});
  // Negative end indices yield an empty view.
  CHECK(to_vector(index_view_to(-5)).empty());
}

TEST_CASE("index_view_between", "[util]")
{
  CHECK(to_vector(index_view_between(0, 3)) == std::vector<std::size_t>{0, 1, 2});
  CHECK(to_vector(index_view_between(2, 5)) == std::vector<std::size_t>{2, 3, 4});
  CHECK(to_vector(index_view_between(3, 3)).empty());
  // The arguments are ordered, so a reversed pair spans the same range.
  CHECK(to_vector(index_view_between(5, 2)) == std::vector<std::size_t>{2, 3, 4});
  CHECK(to_vector(index_view_between(std::size_t{1}, 4)) == std::vector<std::size_t>{1, 2, 3});
}

} // namespace bibstd::util::ranges
