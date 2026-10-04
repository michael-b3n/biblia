#include "test_utils/registrations.hpp"

#include <bibstd/framework/property_tree.hpp>
#include <bibstd/framework/setting.hpp>
#include <bibstd/lua/state.hpp>
#include <bibstd/lua/state_owner.hpp>
#include <bibstd/util/exception.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <chrono>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <format>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace bibstd::lua
{
namespace
{

enum class color
{
  red,
  green
};

template<framework::underlying_setting_type T>
[[nodiscard]] auto make_setting(
  framework::property_tree& tree,
  const std::string& name,
  T value,
  std::string postfix = {},
  framework::setting_validator<T> validator = std::make_shared<framework::setting_validator_unbound>()
) -> std::unique_ptr<framework::setting<T>>
{
  return std::make_unique<framework::setting<T>>(
    name,
    tree.create_property(framework::property_tree::path_type{name}, std::move(value)),
    std::move(postfix),
    std::move(validator)
  );
}

} // namespace

TEST_CASE("lua_state_register_function", "[lua]")
{
  const auto state_owner = lua::state_owner{};
  auto s = state_owner.lock();
  auto keep = test_utils::registrations{};
  CHECK(keep(s.register_function("a.b.twice", [](int x) { return 2 * x; })));
  CHECK(s->script("return root.interface.a.b.twice(21)").get<int>() == 42);
  // A wrong argument type is an error instead of a crash
  CHECK_FALSE(s->safe_script("return root.interface.a.b.twice('x')", sol::script_pass_on_error).valid());
}

TEST_CASE("lua_state_register_function_converts_the_values", "[lua]")
{
  const auto state_owner = lua::state_owner{};
  auto s = state_owner.lock();
  auto keep = test_utils::registrations{};
  const auto check = [&](const std::string& code)
  {
    const auto result = s->safe_script("local f = root.interface.f return " + code, sol::script_pass_on_error);
    return result.valid() && result.get<bool>();
  };
  auto noted = std::string{};

  // Functions of C++ values only, each converted like the values of a script table
  REQUIRE(keep(s.register_function(
    "f.join",
    [](const std::string& text, const std::optional<std::int64_t> count, const std::vector<std::string>& list)
    { return std::format("{} {} {}", text, count.value_or(-1), list.size()); }
  )));
  REQUIRE(keep(s.register_function("f.note", [&noted](std::string text) { noted = std::move(text); })));
  REQUIRE(
    keep(s.register_function("f.find", [](const bool found) { return found ? std::optional{std::string{"x"}} : std::nullopt; }))
  );
  REQUIRE(keep(s.register_function(
    "f.read",
    [](const bool readable) -> std::expected<std::vector<std::int64_t>, std::string>
    {
      if(!readable)
      {
        return std::unexpected{std::string{"unreadable"}};
      }
      return std::vector<std::int64_t>{1, 2};
    }
  )));

  CHECK(check("f.join('a', 2, {'x', 'y'}) == 'a 2 2'"));
  // An argument left out is nil, more than the parameters are ignored
  CHECK(check("f.join('a', nil, {}) == 'a -1 0'"));
  CHECK(check("f.join('a', nil, {}, 'more') == 'a -1 0'"));
  // No result, nil for none
  CHECK(check("select('#', f.note('text')) == 0"));
  CHECK(noted == "text");
  CHECK(check("f.find(true) == 'x' and f.find(false) == nil"));
  // A value, or nil and the reason of the failure
  CHECK(check("#f.read(true) == 2 and select('#', f.read(true)) == 1"));
  CHECK(check("select('#', f.read(false)) == 2 and f.read(false) == nil and select(2, f.read(false)) == 'unreadable'"));
  // An argument of another type is an error for the script, which names it
  CHECK(check("not pcall(f.join, 1, 2, {})"));
  CHECK(check("not pcall(f.join, 'a', 2.5, {})"));
  CHECK(check("not pcall(f.join, 'a', 2)"));
  CHECK(check("select(2, pcall(f.join, 'a', 'b', {})):find('argument 2') ~= nil"));
  CHECK(noted == "text");

  // Only callables of one signature and of values value_cast converts
  STATIC_REQUIRE(function_castable<int (*)(const std::string&)>);
  STATIC_REQUIRE_FALSE(function_castable<int (*)(const char*)>);
  STATIC_REQUIRE_FALSE(function_castable<void (*)(std::string&)>);
  STATIC_REQUIRE_FALSE(function_castable<std::string_view (*)()>);
  STATIC_REQUIRE_FALSE(function_castable<int>);
  const auto generic = [](const auto& value) { return value; };
  STATIC_REQUIRE_FALSE(function_castable<decltype(generic)>);
}

TEST_CASE("lua_state_register_rejects_a_path", "[lua]")
{
  const auto state_owner = lua::state_owner{};
  auto s = state_owner.lock();
  auto keep = test_utils::registrations{};
  REQUIRE(keep(s.register_function("a.b", []() { return 1; })));

  CHECK_FALSE(keep(s.register_function("", []() { return 2; })));
  // Taken, inside or above a registration
  CHECK_FALSE(keep(s.register_function("a.b", []() { return 2; })));
  CHECK_FALSE(keep(s.register_function("a.b.c", []() { return 2; })));
  CHECK_FALSE(keep(s.register_function("a", []() { return 2; })));
  // Taken by the helpers
  CHECK_FALSE(keep(s.register_function("util", []() { return 2; })));
  CHECK_FALSE(keep(s.register_function("util.split", []() { return 2; })));

  CHECK(s->script("return root.interface.a.b() == 1 and type(root.interface.util.split) == 'function'").get<bool>());
}

TEST_CASE("lua_state_functions_go_with_the_state", "[lua]")
{
  const auto captured = std::make_shared<int>(1);
  {
    const auto state_owner = lua::state_owner{};
    auto s = state_owner.lock();
    auto keep = test_utils::registrations{};
    REQUIRE(keep(s.register_function("a.f", [captured]() { return *captured; })));
    // As on shutdown, before the workflows behind the functions are destroyed
    state_owner.shutdown();
    CHECK_FALSE(s.run_script("call.lua", "return a.f()"));
    CHECK(captured.use_count() > 1);
  }
  CHECK(captured.use_count() == 1);
}

TEST_CASE("lua_state_takes_a_registration_back", "[lua]")
{
  const auto state_owner = lua::state_owner{};
  auto s = state_owner.lock();
  const auto runs = [&](const std::string& code) { return s.run_script("test.lua", code).has_value(); };
  auto twice = s.register_function("a.b.twice", [](int x) { return 2 * x; });
  auto value = s.register_function("a.value", []() { return 1; });
  const auto other = s.register_function("other.value", []() { return 2; });
  REQUIRE((twice && value && other));
  // A script holding the function
  REQUIRE(runs("held = a.b.twice assert(held(2) == 4)"));
  const auto holding = s.run_script("holding.lua", "local held = a.b.twice return function() return held(2) end");
  REQUIRE(holding);

  SECTION("its own function only")
  {
    twice = {};
    CHECK_FALSE(twice);
    // Gone with the table it left empty, the others stay
    CHECK(runs("assert(a.b == nil and a.value() == 1 and other.value() == 2)"));
    // Held by a script it is an error instead of a call
    CHECK_FALSE(s.call(holding->as<sol::protected_function>()));
    CHECK(runs("assert(not pcall(a.value, 1, 2) or true)"));
    // The path is free again
    const auto again = s.register_function("a.b.twice", [](int x) { return 3 * x; });
    REQUIRE(again);
    CHECK(runs("assert(a.b.twice(2) == 6)"));
    CHECK_FALSE(s.call(holding->as<sol::protected_function>()));
  }

  SECTION("all of a table")
  {
    twice = {};
    value = {};
    CHECK(runs("assert(a == nil and other.value() == 2)"));
  }

  SECTION("of a setting")
  {
    const auto tree = std::make_shared<framework::property_tree>();
    auto setting = make_setting<std::int32_t>(
      *tree, "numbers.number", 5, "ms", std::make_shared<framework::setting_validator_range<std::int32_t>>(0, 10)
    );
    {
      const auto registered = s.register_setting("numbers.number", *setting);
      REQUIRE(registered);
      CHECK(runs("held = numbers.number assert(held.get() == 5)"));
    }
    CHECK(runs("assert(numbers == nil)"));
  }

  SECTION("several at once")
  {
    auto all = lua::registration{};
    CHECK_FALSE(all);
    // Added one after the other, also one of nothing
    all << std::move(twice) << std::move(value) << s.register_function("a.value", []() { return 0; })
        << s.register_function("more.value", []() { return 3; });
    CHECK(all);
    CHECK_FALSE((twice || value));
    CHECK(runs("assert(a.b.twice(1) == 2 and a.value() == 1 and more.value() == 3)"));
    all = {};
    CHECK(runs("assert(a == nil and more == nil and other.value() == 2)"));
  }

  SECTION("once the state is shut down")
  {
    state_owner.shutdown();
    // No Lua code runs anymore, neither to take it back nor to call it
    twice = {};
    CHECK_FALSE(twice);
    CHECK_FALSE(runs("return a.b.twice(1)"));
  }

  SECTION("after the state is gone")
  {
    auto late = lua::registration{};
    {
      const auto owner = lua::state_owner{};
      late = owner.lock().register_function("f", []() { return 1; });
      REQUIRE(late);
    }
    late = {};
    CHECK_FALSE(late);
  }
}

TEST_CASE("lua_state_registration_waits_for_the_running_script", "[lua]")
{
  const auto state_owner = lua::state_owner{};
  auto started = std::promise<void>{};
  auto proceed = std::promise<void>{};
  auto wait = state_owner.lock().register_function(
    "f.wait",
    [&]()
    {
      started.set_value();
      proceed.get_future().wait();
    }
  );
  auto value = state_owner.lock().register_function("f.value", []() { return 5; });
  REQUIRE((wait && value));

  // The script is inside f.wait, with f.value still to call
  auto script = std::async(
    std::launch::async,
    [&]()
    {
      const auto result = state_owner.lock().run_script("waiting.lua", "f.wait() return f.value()");
      return result ? result->as<int>() : -1;
    }
  );
  started.get_future().wait();
  auto taken_back = std::async(std::launch::async, [&]() { value = {}; });
  CHECK(taken_back.wait_for(std::chrono::milliseconds{200}) == std::future_status::timeout);
  proceed.set_value();
  // The script ran to its end with the function, only then it was taken back
  CHECK(script.get() == 5);
  REQUIRE(taken_back.wait_for(std::chrono::seconds{5}) == std::future_status::ready);
  CHECK_FALSE(state_owner.lock().run_script("after.lua", "return f.value()"));
  CHECK(state_owner.lock().run_script("after.lua", "return 1"));
}

TEST_CASE("lua_state_register_setting", "[lua]")
{
  // The properties reach the tree through shared_from_this
  const auto tree = std::make_shared<framework::property_tree>();
  const auto state_owner = lua::state_owner{};
  auto s = state_owner.lock();
  auto keep = test_utils::registrations{};
  const auto run = [&](const std::string& code) { return s->safe_script(code, sol::script_pass_on_error); };

  SECTION("range validated")
  {
    auto setting = make_setting<std::int32_t>(
      *tree, "numbers.number", 1, "ms", std::make_shared<framework::setting_validator_range<std::int32_t>>(0, 10)
    );
    REQUIRE(keep(s.register_setting("numbers.number", *setting)));
    CHECK(run("return root.interface.numbers.number.get()").get<std::int32_t>() == 1);
    CHECK(run("return root.interface.numbers.number.postfix()").get<std::string>() == "ms");
    CHECK(run("return root.interface.numbers.number.set(5)").get<bool>());
    CHECK(setting->value() == 5);
    CHECK_FALSE(run("return root.interface.numbers.number.set(20)").get<bool>());
    CHECK(setting->value() <= 10);
    CHECK_FALSE(run("return root.interface.numbers.number.set('x')").valid());
  }

  SECTION("enum")
  {
    auto setting = make_setting(*tree, "color", color::red);
    REQUIRE(keep(s.register_setting("color", *setting)));
    CHECK(run("return root.interface.color.get()").get<std::string>() == "red");
    CHECK(run("return root.interface.color.set('green')").get<bool>());
    CHECK(setting->value() == color::green);
    CHECK_FALSE(run("return root.interface.color.set('blue')").valid());
  }

  SECTION("optional")
  {
    auto setting = make_setting(*tree, "name", std::optional<std::string>{});
    REQUIRE(keep(s.register_setting("name", *setting)));
    CHECK(run("return root.interface.name.get() == nil").get<bool>());
    CHECK(run("return root.interface.name.set('kjv')").get<bool>());
    CHECK(setting->value() == "kjv");
    CHECK(run("return root.interface.name.set(nil)").get<bool>());
    CHECK(setting->value() == std::nullopt);
  }

  SECTION("list")
  {
    auto setting = make_setting(*tree, "list", std::vector<std::int32_t>{1, 2});
    REQUIRE(keep(s.register_setting("list", *setting)));
    CHECK(run("local l = root.interface.list.get() return #l == 2 and l[1] == 1 and l[2] == 2").get<bool>());
    CHECK(run("return root.interface.list.set({3, 4, 5})").get<bool>());
    CHECK(setting->value() == std::vector<std::int32_t>{3, 4, 5});
  }

  SECTION("duration and path")
  {
    auto duration = make_setting(*tree, "duration", std::chrono::seconds{3});
    auto folder = make_setting(*tree, "folder", std::filesystem::path{"a/b"});
    REQUIRE(keep(s.register_setting("duration", *duration)));
    REQUIRE(keep(s.register_setting("folder", *folder)));
    CHECK(run("return root.interface.duration.get()").get<std::int64_t>() == 3);
    CHECK(run("return root.interface.duration.set(4)").get<bool>());
    CHECK(duration->value() == std::chrono::seconds{4});
    CHECK(run("return root.interface.folder.get()").get<std::string>() == "a/b");
    CHECK(run("return root.interface.folder.set('c')").get<bool>());
    CHECK(folder->value() == std::filesystem::path{"c"});
  }
}

TEST_CASE("lua_state_calls_functions", "[lua]")
{
  const auto state_owner = lua::state_owner{};
  auto s = state_owner.lock();
  s->script(R"(
    function twice(a) return a .. a end
    function nothing() end
    function broken() error("broken") end
  )");
  const auto twice = s.call((*s)["twice"], "x");
  REQUIRE(twice);
  CHECK(twice->as<std::string>() == "xx");
  const auto nothing = s.call((*s)["nothing"]);
  REQUIRE(nothing);
  CHECK(nothing->get_type() == sol::type::lua_nil);
  CHECK_FALSE(s.call((*s)["broken"]));
}

TEST_CASE("lua_state_raises_exceptions_as_errors", "[lua]")
{
  const auto state_owner = lua::state_owner{};
  auto s = state_owner.lock();
  auto keep = test_utils::registrations{};
  REQUIRE(keep(s.register_function("throwing", []() -> int { throw util::exception{"thrown"}; })));
  CHECK(s.run_script("caught.lua", "local ok, message = pcall(throwing) assert(not ok and message:find('thrown'))"));
  CHECK_FALSE(s.run_script("uncaught.lua", "throwing()"));
}

TEST_CASE("lua_state_stops_scripts", "[lua]")
{
  const auto state_owner = lua::state_owner{};
  auto s = state_owner.lock();
  auto keep = test_utils::registrations{};
  s->script(R"(
    function endless() while true do end end
    -- Few instructions run between the calls
    function calls() for i = 1, 1e9 do local s = string.rep('x', 1e7) end end
    function catching() while true do pcall(endless) end end
    function coroutines() while true do coroutine.wrap(endless)() end end
    function nested() while true do root.interface.endless() end end
    function closing() local x <close> = setmetatable({}, {__close = endless}) endless() end
    function value() return 1 end
  )");
  REQUIRE(keep(s.register_function("endless", [&]() { return s.call((*s)["endless"]).has_value(); })));
  // Where the hook reads it, out of reach of scripts like all of root.system
  CHECK(s->script("return type(root.system.shutdown_flag) == 'userdata'").get<bool>());
  CHECK(s.run_script("flag.lua", "assert(root == nil and _G == nil)"));
  // Each with a state of its own, a stop is for good
  const auto function = GENERATE(as<std::string>{}, "endless", "calls", "catching", "coroutines", "nested", "closing");
  CAPTURE(function);
  // From another thread, the running code holds the lock
  const auto stopping = std::jthread{[&]()
                                     {
                                       std::this_thread::sleep_for(std::chrono::milliseconds{100});
                                       state_owner.shutdown();
                                     }};
  CHECK_FALSE(s.call((*s)[function]));
  // For good
  CHECK_FALSE(s.call((*s)["value"]));
  CHECK_FALSE(s.run_script("after.lua", "return 1"));
}

TEST_CASE("lua_state_protects_shared_tables", "[lua]")
{
  const auto state_owner = lua::state_owner{};
  auto s = state_owner.lock();
  auto keep = test_utils::registrations{};
  REQUIRE(keep(s.register_function("a.f", []() { return 1; })));
  const auto runs = [&](const std::string& code) { return s.run_script("test.lua", code).has_value(); };

  SECTION("scripts read them")
  {
    CHECK(runs("assert(a.f() == 1 and util.split('a,b', ',')[2] == 'b')"));
    CHECK(runs("assert(#util.keys({1}) == 1 and ('abc'):upper() == 'ABC' and string.upper('a') == 'A')"));
    // Like tables: pairs, ipairs and the length
    s->script("root.interface.list = {'x', 'y', nested = {1, 2, 3}}");
    CHECK(runs("local n = 0 for k, v in pairs(list) do n = n + 1 end assert(n == 3 and #list == 2 and #list.nested == 3)"));
    CHECK(runs("local t = '' for i, v in ipairs(list) do t = t .. i .. v end assert(t == '1x2y')"));
    CHECK(runs("for k, v in pairs(list) do if k == 'nested' then assert(getmetatable(v) == 'read only') end end"));
    CHECK(runs("assert(util.contains(util.keys(util), 'split'))"));
    // What the app keeps for itself is out of reach
    CHECK(runs("assert(root == nil and _G == nil and require == nil and print == nil and rawset == nil)"));
    // Globals of the script itself stay writable, also one named like a shared one
    CHECK(runs("x = 1 assert(x == 1)"));
    CHECK(runs("util = 1 assert(util == 1)"));
    CHECK(runs("assert(type(util) == 'table')"));
    // A copy is a table of the script
    CHECK(runs("local copy = util.copy(a) copy.g = 2 assert(copy.g == 2)"));
  }

  SECTION("scripts reach no metatable of them")
  {
    // A registered function is a plain function, the views and strings hand out a text instead of their metatable
    CHECK(runs("assert(type(a.f) == 'function' and getmetatable(a.f) == nil)"));
    // Also one with captures, as the workflows register them
    REQUIRE(keep(s.register_function("c.f", [value = 2]() { return value; })));
    CHECK(runs("assert(type(c.f) == 'function' and getmetatable(c.f) == nil and c.f() == 2)"));
    CHECK(runs("assert(getmetatable(a) == 'read only' and getmetatable(util) == 'read only')"));
    CHECK(runs("assert(getmetatable(string) == 'read only' and getmetatable('') == 'read only')"));
    CHECK_FALSE(runs("setmetatable(util, {})"));
    CHECK_FALSE(runs("getmetatable('').__index = {}"));
    // The environment of a script is its own table, what it falls back to stays the read-only view
    CHECK(runs("assert(getmetatable(getmetatable(_ENV).__index) == 'read only')"));
    CHECK_FALSE(runs("getmetatable(_ENV).__index.util = nil"));
    CHECK_FALSE(runs("getmetatable(_ENV).__index.a.f = nil"));
    CHECK_FALSE(runs("setmetatable(_ENV, {__gc = function() end})"));
    // Raw access sees the empty view, not the table behind it
    CHECK(runs("assert(rawget(util, 'split') == nil and next(util) == nil and rawlen(a) == 0)"));
  }

  SECTION("scripts do not change them")
  {
    CHECK_FALSE(runs("a.x = 1"));
    CHECK_FALSE(runs("a.f = nil"));
    CHECK_FALSE(runs("util.split = nil"));
    CHECK_FALSE(runs("string.upper = nil"));
    CHECK_FALSE(runs("setmetatable(a, {})"));
    CHECK_FALSE(runs("load('a.x = 1')()"));
    CHECK_FALSE(runs("table.insert(a, 1)"));
    CHECK_FALSE(runs("setmetatable({}, {__gc = function() end})"));
    // Neither a placeholder, Lua would run a function set in its place later
    CHECK_FALSE(runs("setmetatable({}, {__gc = true})"));
    CHECK(runs("assert(collectgarbage == nil and getmetatable(setmetatable({}, {__index = {}})) ~= nil)"));
    CHECK(runs("assert(getmetatable('') == 'read only' and getmetatable(a) == 'read only')"));
    // A loaded chunk gets globals of its own
    CHECK(runs("load('y = 1')() assert(y == nil)"));
    CHECK(s->script(
             "return root.interface.a.f() == 1 and root.interface.a.x == nil and "
             "type(root.interface.util.split) == 'function' and string.upper('a') == 'A'"
    )
            .get<bool>());
  }

  // The app still changes them
  REQUIRE(keep(s.register_function("b.f", []() { return 2; })));
  CHECK(s->script("return root.interface.b.f()").get<int>() == 2);
}

} // namespace bibstd::lua
