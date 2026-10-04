#include <bibstd/lua/script_table.hpp>
#include <bibstd/lua/sol.hpp>
#include <bibstd/lua/value_cast.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace bibstd::lua
{
namespace
{

///
/// \return \p v converted to Lua and back
///
template<value_castable T>
[[nodiscard]] auto round_trip(sol::state_view lua, const T& v) -> std::optional<T>
{
  return value_cast<T>::from(value_cast<T>::to(lua, v));
}

///
/// \return what Lua \p expression is as T
///
template<value_castable T>
[[nodiscard]] auto cast(sol::state& lua, const std::string& expression) -> std::optional<T>
{
  return value_cast<T>::from(lua.script("return " + expression).get<sol::object>());
}

} // namespace

TEST_CASE("lua_value_cast_converts_basic_values", "[lua]")
{
  auto lua = sol::state{};
  CHECK(round_trip(lua, true) == true);
  CHECK(round_trip(lua, std::int64_t{-7}) == -7);
  CHECK(round_trip(lua, 0.25) == 0.25);
  CHECK(round_trip(lua, std::string{"text"}) == "text");
  CHECK(round_trip(lua, std::chrono::seconds{3}) == std::chrono::seconds{3});
  CHECK(round_trip(lua, std::filesystem::path{"a/b"}) == std::filesystem::path{"a/b"});
  CHECK(cast<std::filesystem::path>(lua, "'a/b'") == std::filesystem::path{"a/b"});

  // A float is no integer, nor is a value out of the range
  CHECK(cast<std::int32_t>(lua, "4 // 2") == 2);
  CHECK_FALSE(cast<std::int32_t>(lua, "2.0"));
  CHECK_FALSE(cast<std::int32_t>(lua, "2.5"));
  CHECK_FALSE(cast<std::int8_t>(lua, "300"));
  CHECK_FALSE(cast<std::uint32_t>(lua, "-1"));
  CHECK(cast<double>(lua, "1") == 1.0);
  // No conversion between types
  CHECK_FALSE(cast<std::int32_t>(lua, "'2'"));
  CHECK_FALSE(cast<std::string>(lua, "2"));
  CHECK_FALSE(cast<bool>(lua, "nil"));
}

TEST_CASE("lua_value_cast_throws_for_an_integer_lua_can_not_hold", "[lua]")
{
  auto lua = sol::state{};
  using limits = std::numeric_limits<std::uint64_t>;
  const auto largest = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
  CHECK(round_trip(lua, largest) == largest);
  // It would be another, negative number for the script
  CHECK_THROWS(value_cast<std::uint64_t>::to(lua, largest + 1));
  CHECK_THROWS(value_cast<std::uint64_t>::to(lua, limits::max()));
  CHECK_THROWS(value_cast<std::vector<std::uint64_t>>::to(lua, {1, limits::max()}));
  CHECK(round_trip(lua, std::numeric_limits<std::int64_t>::min()) == std::numeric_limits<std::int64_t>::min());
}

TEST_CASE("lua_value_cast_converts_containers", "[lua]")
{
  auto lua = sol::state{};
  using optional_text = std::optional<std::string>;
  CHECK(round_trip(lua, optional_text{"x"}) == optional_text{"x"});
  CHECK(cast<optional_text>(lua, "nil") == std::optional<optional_text>{optional_text{}});
  CHECK_FALSE(cast<optional_text>(lua, "1"));

  using list = std::vector<std::int64_t>;
  CHECK(round_trip(lua, list{1, 2, 3}) == list{1, 2, 3});
  CHECK(cast<list>(lua, "{}") == list{});
  CHECK_FALSE(cast<list>(lua, "{1, 'a'}"));
  CHECK_FALSE(cast<list>(lua, "1"));
  // A table with other keys is no list, else one by key would be read as an empty list
  CHECK_FALSE(cast<list>(lua, "{a = 1}"));
  CHECK_FALSE(cast<list>(lua, "{1, 2, n = 2}"));
  CHECK_FALSE(cast<list>(lua, "{[2] = 2}"));

  using map = std::map<std::string, std::vector<std::string>>;
  CHECK(
    round_trip(
      lua,
      map{
        {"a", {"x"}},
        {"b",    {}}
  }
    ) == map{{"a", {"x"}}, {"b", {}}}
  );
  CHECK_FALSE(cast<map>(lua, "{[1] = {}}"));
  CHECK_FALSE(cast<map>(lua, "{a = {1}}"));
  // Keys of any type but nil
  using numbers = std::map<std::int64_t, std::string>;
  CHECK(
    cast<numbers>(lua, "{[2] = 'b', [10] = 'j'}") == numbers{
                                                       { 2, "b"},
                                                       {10, "j"}
  }
  );
  CHECK(
    round_trip(
      lua,
      numbers{
        {-1, "x"}
  }
    ) == numbers{{-1, "x"}}
  );
  using flags = std::map<bool, std::int64_t>;
  CHECK(
    cast<flags>(lua, "{[true] = 1, [false] = 0}") == flags{
                                                       { true, 1},
                                                       {false, 0}
  }
  );
  CHECK_FALSE(cast<flags>(lua, "{x = 1}"));
}

TEST_CASE("lua_value_cast_takes_plain_value_types_only", "[lua]")
{
  STATIC_REQUIRE(value_castable<bool>);
  STATIC_REQUIRE(value_castable<std::int64_t>);
  STATIC_REQUIRE(value_castable<std::chrono::seconds>);
  STATIC_REQUIRE(value_castable<std::map<std::string, std::vector<std::optional<double>>>>);
  // Const types, a const bool would even be taken as an integer
  STATIC_REQUIRE_FALSE(value_castable<const bool>);
  STATIC_REQUIRE_FALSE(value_castable<const int>);
  STATIC_REQUIRE_FALSE(value_castable<const double>);
  STATIC_REQUIRE_FALSE(value_castable<const std::string>);
  STATIC_REQUIRE_FALSE(value_castable<const std::chrono::seconds>);
  // References, from would hand out one to a value that is gone
  STATIC_REQUIRE_FALSE(value_castable<int&>);
  STATIC_REQUIRE_FALSE(value_castable<const int&>);
  STATIC_REQUIRE_FALSE(value_castable<const std::string&>);
  STATIC_REQUIRE_FALSE(value_castable<std::chrono::seconds&>);
  STATIC_REQUIRE_FALSE(value_castable<const std::chrono::seconds&>);
  // Pointers and views
  STATIC_REQUIRE_FALSE(value_castable<int*>);
  STATIC_REQUIRE_FALSE(value_castable<const char*>);
  STATIC_REQUIRE_FALSE(value_castable<std::string*>);
  STATIC_REQUIRE_FALSE(value_castable<std::string_view>);
  // Neither inside the containers
  STATIC_REQUIRE_FALSE(value_castable<std::vector<const char*>>);
  STATIC_REQUIRE_FALSE(value_castable<std::optional<std::string_view>>);
  STATIC_REQUIRE_FALSE(value_castable<std::map<std::string, const int>>);
  STATIC_REQUIRE_FALSE(value_castable<std::map<int*, int>>);
}

TEST_CASE("lua_value_cast_converts_script_tables", "[lua]")
{
  auto lua = sol::state{};
  using table =
    script_table<field<"name", std::string>, field<"count", std::int64_t>, field<"note", std::optional<std::string>>>;
  CHECK(round_trip(lua, table{"a", 1, std::nullopt}) == table{"a", 1, std::nullopt});
  CHECK(cast<table>(lua, "{name = 'a', count = 2, note = 'n'}") == table{"a", 2, std::string{"n"}});
  // nil is an empty table, which lacks the values that are no optional
  CHECK(cast<script_table<field<"note", std::optional<std::string>>>>(lua, "nil"));
  CHECK_FALSE(cast<table>(lua, "nil"));
  // A key of no field or a value of another type
  CHECK_FALSE(cast<table>(lua, "{name = 'a', count = 2, other = 1}"));
  CHECK_FALSE(cast<table>(lua, "{name = 'a', count = 'two'}"));
  CHECK(table{"a", 1, std::nullopt}.get<"count">() == 1);
}

} // namespace bibstd::lua
