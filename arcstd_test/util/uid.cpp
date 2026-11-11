#include <arcstd/util/uid.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <format>
#include <set>
#include <type_traits>

namespace arcstd::util
{

struct uid_test_tag_a final
{};
struct uid_test_tag_b final
{};

TEST_CASE("uid typedefs", "[util]")
{
  static_assert(std::is_same_v<uid<uid_test_tag_a>::tag_type, uid_test_tag_a>);
  static_assert(std::is_same_v<uid<uid_test_tag_a>::underlying_type, std::uint64_t>);
  static_assert(!std::is_same_v<uid<uid_test_tag_a>, uid<uid_test_tag_b>>);
  static_assert(!std::is_convertible_v<uid<uid_test_tag_a>, uid<uid_test_tag_b>>);
  static_assert(!std::is_constructible_v<uid<uid_test_tag_a>, std::uint64_t>);
}

TEST_CASE("uid is unique", "[util]")
{
  using uid_type = uid<uid_test_tag_a>;

  const auto first = uid_type{};
  const auto second = uid_type{};
  const auto third = uid_type::new_uid();

  CHECK(first != second);
  CHECK(second != third);
  CHECK(first != third);

  auto ids = std::set<uid_type>{};
  for(auto i = 0; i < 100; ++i)
  {
    ids.insert(uid_type::new_uid());
  }
  CHECK(ids.size() == 100);
}

TEST_CASE("uid is copyable and comparable", "[util]")
{
  using uid_type = uid<uid_test_tag_a>;

  const auto id = uid_type{};
  const auto copy = id;
  CHECK(copy == id);
  CHECK(!(copy < id));
  CHECK(!(copy > id));
}

TEST_CASE("uid counters are per tag type", "[util]")
{
  const auto a = uid<uid_test_tag_b>{};
  const auto b = uid<uid_test_tag_b>{};
  CHECK(a != b);
}

TEST_CASE("uid formatter", "[util]")
{
  using uid_type = uid<uid_test_tag_a>;

  const auto id = uid_type{};
  CHECK(!std::format("{}", id).empty());
  // The formatter forwards to the underlying integer formatter.
  const auto next = uid_type{};
  CHECK(std::format("{}", id) != std::format("{}", next));
  CHECK(std::format("{:>8}", id).size() >= 8);
}

} // namespace arcstd::util
