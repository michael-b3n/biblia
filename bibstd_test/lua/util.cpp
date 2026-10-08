#include "test_utils/registrations.hpp"

#include <bibstd/lua/embedded_scripts.hpp>
#include <bibstd/lua/state_owner.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <ranges>
#include <string>

namespace bibstd::lua
{

TEST_CASE("lua_util", "[lua]")
{
  auto owner = state_owner{};
  auto s = owner.lock();
  auto keep = test_utils::registrations{};
  const auto check = [&](const std::string& code) { return s->script("return " + code).get<bool>(); };

  SECTION("strings")
  {
    CHECK(check("table.concat(root.interface.util.split('a,,b', ','), '|') == 'a||b'"));
    CHECK(check("#root.interface.util.split('', ',') == 1"));
    CHECK(check("root.interface.util.split('a--b', '--')[2] == 'b'"));
    CHECK_THROWS(s->script("root.interface.util.split('a', '')"));
    CHECK(check("root.interface.util.trim('  a b \\t') == 'a b'"));
    CHECK(check("root.interface.util.starts_with('abc', 'ab') and not root.interface.util.starts_with('abc', 'bc')"));
    CHECK(check(
      "root.interface.util.ends_with('abc', 'bc') and root.interface.util.ends_with('abc', '') and not "
      "root.interface.util.ends_with('abc', 'ab')"
    ));
  }

  SECTION("tables")
  {
    CHECK(check("#root.interface.util.keys({a = 1, b = 2}) == 2"));
    CHECK(check("root.interface.util.contains({1, 2}, 2) and not root.interface.util.contains({1, 2}, 3)"));
    CHECK(check("table.concat(root.interface.util.map({1, 2}, function(v) return v * 2 end), ',') == '2,4'"));
    CHECK(check("table.concat(root.interface.util.filter({1, 2, 3}, function(v) return v ~= 2 end), ',') == '1,3'"));
    s->script("original = {list = {1}} original.self = original copied = root.interface.util.copy(original)");
    CHECK(check("copied ~= original and copied.list ~= original.list and copied.list[1] == 1"));
    CHECK(check("copied.self == copied"));
    // A protected metatable hands out its __metatable field instead of itself
    CHECK(check("getmetatable(root.interface.util.copy(setmetatable({}, {__metatable = 'locked'}))) == nil"));
  }

  SECTION("inspection")
  {
    CHECK(
      s->script("return root.interface.util.dump({b = 'x', a = {1}})").get<std::string>() ==
      "{\n  [\"a\"] = {\n    [1] = 1\n  },\n  [\"b\"] = \"x\"\n}"
    );
    CHECK(check("root.interface.util.dump({}) == '{}'"));
    s->script("cyclic = {} cyclic.self = cyclic");
    CHECK(check("root.interface.util.dump(cyclic) == '{\\n  [\"self\"] = <cycle>\\n}'"));
    CHECK(keep(s.register_function("a.b.value", []() { return 3; })));
    CHECK(check("root.interface.util.at('a.b.value')() == 3 and root.interface.util.at('a.missing.value') == nil"));
  }
}

TEST_CASE("lua_state_register_takes_no_name_of_the_state", "[lua]")
{
  auto owner = state_owner{};
  auto s = owner.lock();
  auto keep = test_utils::registrations{};
  CHECK(s->script("return util == nil").get<bool>());
  CHECK_FALSE(keep(s.register_function("util", []() { return 1; })));
  CHECK_FALSE(keep(s.register_function("util.split", []() { return 1; })));
  // Neither what scripts have of Lua
  CHECK_FALSE(keep(s.register_function("pairs", []() { return 1; })));
  CHECK_FALSE(keep(s.register_function("string.upper", []() { return 1; })));
  CHECK(s->script("return type(root.interface.util.split) == 'function'").get<bool>());
}

TEST_CASE("lua_embedded_scripts_add_themselves", "[lua]")
{
  const auto owner = state_owner{};
  auto s = owner.lock();
  // Each at the node it names
  CHECK(s->script("return type(root.interface.util) == 'table'").get<bool>());
  CHECK(s->script("return type(root.system.readonly) == 'function'").get<bool>());
  CHECK(s->script("return type(root.system.util.remove) == 'function'").get<bool>());
  // So does init.lua with what scripts run on
  CHECK(s->script("return root.system.sandbox == root.system.readonly(root.interface)").get<bool>());
  // Nothing else is left behind, neither a way to load files
  CHECK(s->script("return require == nil and util == nil and readonly == nil").get<bool>());
  CHECK(s->script("return root.system.embedded('missing.lua') == nil").get<bool>());
}

TEST_CASE("lua_system_util_removes_a_path", "[lua]")
{
  const auto owner = state_owner{};
  auto s = owner.lock();
  const auto check = [&](const std::string& code) { return s->script("local i = root.interface " + code).get<bool>(); };
  s->script("root.interface.a = {b = {c = 1, d = 2}, e = 3} root.interface.f = {g = {h = 1}}");
  s->script("root.system.util.remove('a.b.c')");
  CHECK(check("return i.a.b.c == nil and i.a.b.d == 2 and i.a.e == 3"));
  // With the tables it leaves empty, up to the first one that holds more
  s->script("root.system.util.remove('a.b.d')");
  CHECK(check("return i.a.b == nil and i.a.e == 3"));
  s->script("root.system.util.remove('f.g.h')");
  CHECK(check("return i.f == nil and type(i.util) == 'table'"));
  // A whole table, a path that is not there, no path
  s->script("root.interface.k = {l = {m = 1}}");
  s->script("root.system.util.remove('k.l')");
  CHECK(check("return i.k == nil"));
  CHECK_NOTHROW(
    s->script("root.system.util.remove('a.missing.x') root.system.util.remove('a.e.x') root.system.util.remove('')")
  );
  CHECK(check("return i.a.e == 3 and type(i.util.split) == 'function'"));
}

TEST_CASE("lua_scripts_compile", "[lua]")
{
  const auto owner = state_owner{};
  auto s = owner.lock();
  REQUIRE_FALSE(internal::all().empty());
  CHECK(internal::find("init.lua"));
  CHECK_FALSE(internal::find("missing.lua"));
  REQUIRE_FALSE(bundled::all().empty());
  for(const auto& script : std::array{internal::all(), bundled::all()} | std::views::join)
  {
    INFO(script.name);
    CHECK(s->load(script.code, std::string{script.name}).valid());
  }
}

} // namespace bibstd::lua
