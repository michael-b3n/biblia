#include <bibstd/framework/setting_validator.hpp>
#include <bibstd/util/exception.hpp>
#include <bibstd/util/language.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace bibstd::framework
{

TEST_CASE("setting_validator_range_limits_plain_values", "[framework]")
{
  using namespace std::chrono_literals;

  SECTION("integers, up to but without the upper bound")
  {
    const auto validator = setting_validator_range<std::int32_t>{0, 10};
    CHECK(validator.validate(5) == 5);
    CHECK(validator.validate(-3) == 0);
    CHECK(validator.validate(10) == 9);
    CHECK(validator.contains(0));
    CHECK(validator.contains(9));
    CHECK_FALSE(validator.contains(10));
    CHECK_FALSE(validator.contains(-1));
  }

  SECTION("floating point numbers, with the upper bound")
  {
    const auto validator = setting_validator_range<double>{0.0, 1.0};
    CHECK(validator.validate(0.5) == 0.5);
    CHECK(validator.validate(-0.5) == 0.0);
    CHECK(validator.validate(1.5) == 1.0);
    CHECK(validator.contains(1.0));
    CHECK_FALSE(validator.contains(1.5));
  }

  SECTION("durations by their count")
  {
    const auto validator = setting_validator_range<std::chrono::milliseconds>{100ms, 1s};
    CHECK(validator.validate(500ms) == 500ms);
    CHECK(validator.validate(50ms) == 100ms);
    CHECK(validator.validate(2s) == 999ms);
    CHECK(validator.contains(999ms));
    CHECK_FALSE(validator.contains(1s));
  }

  SECTION("but not with an empty range")
  {
    CHECK_THROWS_AS(setting_validator_range<std::int32_t>(5, 5), util::exception);
    CHECK_THROWS_AS(setting_validator_range<std::vector<std::int32_t>>(5, 5), util::exception);
    // Only an optional has a value for it, \see below
    CHECK_NOTHROW(setting_validator_range<std::optional<std::int32_t>>(5, 5));
  }
}

TEST_CASE("setting_validator_range_limits_wrapped_values", "[framework]")
{
  using namespace std::chrono_literals;

  SECTION("of an optional")
  {
    using value_type = std::optional<std::int32_t>;
    const auto validator = setting_validator_range<value_type>{0, 10};
    CHECK(validator.validate(value_type{15}) == value_type{9});
    CHECK(validator.validate(value_type{-3}) == value_type{0});
    CHECK(validator.contains(value_type{5}));
    CHECK_FALSE(validator.contains(value_type{10}));
    // No value is out of no range
    CHECK(validator.validate(value_type{}) == value_type{});
    CHECK(validator.contains(value_type{}));
  }

  SECTION("of an optional with an empty range")
  {
    using value_type = std::optional<std::int32_t>;
    const auto validator = setting_validator_range<value_type>{5, 5};
    // Nothing is in it, so no value is left
    CHECK(validator.validate(value_type{5}) == value_type{});
    CHECK_FALSE(validator.contains(value_type{5}));
    CHECK(validator.contains(value_type{}));
  }

  SECTION("of a vector, each of them")
  {
    using value_type = std::vector<std::chrono::seconds>;
    const auto validator = setting_validator_range<value_type>{1s, 60s};
    CHECK(validator.validate(value_type{0s, 30s, 90s}) == value_type{1s, 30s, 59s});
    CHECK(validator.validate(value_type{}) == value_type{});
    CHECK(validator.contains(value_type{1s, 59s}));
    CHECK_FALSE(validator.contains(value_type{1s, 60s}));
    CHECK(validator.contains(value_type{}));
    // And one of them alone
    CHECK(validator.validate(90s) == 59s);
    CHECK(validator.contains(59s));
    CHECK_FALSE(validator.contains(60s));
  }
}

TEST_CASE("setting_validator_range_leaves_other_types_alone", "[framework]")
{
  using names_type = std::vector<std::string>;

  SECTION("a text")
  {
    const auto validator = setting_validator_range<std::string>{"b", "c"};
    CHECK(validator.validate("z") == "z");
    CHECK(validator.contains("z"));
  }

  SECTION("texts")
  {
    const auto validator = setting_validator_range<names_type>{"b", "c"};
    CHECK(validator.validate(names_type{"a", "z"}) == names_type{"a", "z"});
    CHECK(validator.contains(names_type{"a", "z"}));
    CHECK(validator.validate(std::string{"z"}) == "z");
    CHECK(validator.contains(std::string{"z"}));
  }
}

TEST_CASE("setting_validator_range_type_erased_wraps_a_validator", "[framework]")
{
  using namespace std::chrono_literals;

  SECTION("of a plain value")
  {
    const auto validator = std::make_shared<setting_validator_range<std::int32_t>>(0, 10);
    const auto erased = setting_validator_range_type_erased<std::int32_t>{validator};
    CHECK(erased.validate(15) == 9);
    CHECK(erased.contains(9));
    CHECK_FALSE(erased.contains(10));
    CHECK(erased.connector == validator);
  }

  SECTION("of an optional")
  {
    using value_type = std::optional<std::int32_t>;
    const auto validator = std::make_shared<setting_validator_range<value_type>>(0, 10);
    const auto erased = setting_validator_range_type_erased<value_type>{validator};
    CHECK(erased.validate(value_type{15}) == value_type{9});
    CHECK(erased.validate(value_type{}) == value_type{});
    CHECK(erased.contains(value_type{}));
    CHECK_FALSE(erased.contains(value_type{10}));
  }

  SECTION("of a vector")
  {
    using value_type = std::vector<std::chrono::seconds>;
    const auto validator = std::make_shared<setting_validator_range<value_type>>(1s, 60s);
    const auto erased = setting_validator_range_type_erased<value_type>{validator};
    CHECK(erased.validate(value_type{0s, 90s}) == value_type{1s, 59s});
    CHECK(erased.contains(value_type{1s, 59s}));
    CHECK_FALSE(erased.contains(value_type{60s}));
    // One of them alone
    CHECK(erased.validate(90s) == 59s);
    CHECK(erased.contains(59s));
    CHECK_FALSE(erased.contains(60s));
  }
}

TEST_CASE("setting_validator_list_holds_the_available_values", "[framework]")
{
  using names_type = std::vector<std::string>;

  SECTION("sorted and without duplicates")
  {
    auto validator = setting_validator_list<std::string>{
      names_type{"b", "a", "b"}
    };
    CHECK_FALSE(validator.empty());
    CHECK(validator.available() == names_type{"a", "b"});
    CHECK(validator.contains("a"));
    CHECK_FALSE(validator.contains("c"));
    // Replaced as a whole
    REQUIRE(validator.available(names_type{"d", "c", "c"}));
    CHECK(validator.available() == names_type{"c", "d"});
    CHECK_FALSE(validator.contains("a"));
  }

  SECTION("none until they are given")
  {
    const auto validator = setting_validator_list<std::string>{};
    CHECK(validator.empty());
    CHECK(validator.available().empty());
    CHECK_FALSE(validator.contains("a"));
  }

  SECTION("a plain value needs one to hold")
  {
    auto validator = setting_validator_list<std::string>{names_type{"a"}};
    CHECK_FALSE(validator.available(names_type{}));
    CHECK(validator.available() == names_type{"a"});
  }

  SECTION("an optional may hold none")
  {
    using value_type = std::optional<std::string>;
    auto validator = setting_validator_list<value_type>{names_type{"a"}};
    CHECK(validator.contains(value_type{"a"}));
    CHECK_FALSE(validator.contains(value_type{"b"}));
    CHECK(validator.contains(value_type{}));
    CHECK(validator.available(names_type{}));
    CHECK(validator.empty());
    CHECK(validator.contains(value_type{}));
    CHECK_FALSE(validator.contains(value_type{"a"}));
  }

  SECTION("a vector holds any of them, or none")
  {
    auto validator = setting_validator_list<names_type>{
      names_type{"a", "b"}
    };
    CHECK(validator.contains(names_type{"b", "a"}));
    CHECK(validator.contains(names_type{}));
    CHECK_FALSE(validator.contains(names_type{"a", "c"}));
    // One of them alone
    CHECK(validator.contains(std::string{"a"}));
    CHECK_FALSE(validator.contains(std::string{"c"}));
    CHECK(validator.available(names_type{}));
    CHECK(validator.empty());
  }

  SECTION("and tells that they changed")
  {
    auto validator = setting_validator_list<std::string>{};
    auto changes = 0;
    validator.connect_on_changed([&]() { ++changes; });
    REQUIRE(validator.available(names_type{"a"}));
    CHECK(changes == 1);
  }
}

TEST_CASE("setting_validator_list_type_erased_wraps_a_validator", "[framework]")
{
  using names_type = std::vector<std::string>;

  SECTION("of an enum, by the names of its values")
  {
    const auto validator = std::make_shared<setting_validator_list<util::language>>(std::vector{util::language::german});
    const auto erased = setting_validator_list_type_erased<std::string>{validator};
    CHECK_FALSE(erased.empty());
    CHECK(erased.available() == names_type{"german"});
    CHECK(erased.contains("german"));
    CHECK_FALSE(erased.contains("english"));
    CHECK(erased.connector == validator);
    // Follows the validator
    REQUIRE(validator->available(std::vector{util::language::german, util::language::english}));
    CHECK(erased.available() == names_type{"english", "german"});
    CHECK(erased.contains("english"));
  }

  SECTION("of an optional")
  {
    using value_type = std::optional<std::string>;
    const auto validator = std::make_shared<setting_validator_list<value_type>>();
    const auto erased = setting_validator_list_type_erased<value_type>{validator};
    CHECK(erased.empty());
    REQUIRE(validator->available(names_type{"a"}));
    CHECK(erased.contains(value_type{"a"}));
    CHECK(erased.contains(value_type{}));
    CHECK_FALSE(erased.contains(value_type{"b"}));
  }

  SECTION("of a vector")
  {
    const auto validator = std::make_shared<setting_validator_list<names_type>>(names_type{"a", "b"});
    const auto erased = setting_validator_list_type_erased<names_type>{validator};
    CHECK(erased.available() == names_type{"a", "b"});
    CHECK(erased.contains(names_type{"b", "a"}));
    CHECK_FALSE(erased.contains(names_type{"c"}));
    // One of them alone
    CHECK(erased.contains(std::string{"a"}));
    CHECK_FALSE(erased.contains(std::string{"c"}));
  }
}

} // namespace bibstd::framework
