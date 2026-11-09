#include <arca/util/visit_helper.hpp>

#include <catch2/catch_test_macros.hpp>

#include <stdexcept>
#include <string>
#include <variant>

namespace bibstd::util
{

TEST_CASE("visit_lambdas dispatches on the active alternative", "[util]")
{
  using variant_type = std::variant<int, double, std::string>;

  const auto to_name = [](const variant_type& v)
  {
    return visit_lambdas(
      v,
      [](const int&) { return std::string{"int"}; },
      [](const double&) { return std::string{"double"}; },
      [](const std::string&) { return std::string{"string"}; }
    );
  };

  CHECK(to_name(variant_type{1}) == "int");
  CHECK(to_name(variant_type{1.0}) == "double");
  CHECK(to_name(variant_type{std::string{"a"}}) == "string");
}

TEST_CASE("visit_lambdas with a generic fallback", "[util]")
{
  using variant_type = std::variant<int, double>;

  const auto is_int = [](const variant_type& v)
  { return visit_lambdas(v, [](const int&) { return true; }, [](const auto&) { return false; }); };

  CHECK(is_int(variant_type{1}));
  CHECK(!is_int(variant_type{1.0}));
}

TEST_CASE("visit_lambdas can mutate the visited variant", "[util]")
{
  auto v = std::variant<int, double>{1};
  visit_lambdas(v, [](int& e) { e += 1; }, [](double& e) { e += 0.5; });
  CHECK(std::get<int>(v) == 2);

  v = 1.0;
  visit_lambdas(v, [](int& e) { e += 1; }, [](double& e) { e += 0.5; });
  CHECK(std::get<double>(v) == 0.5 + 1.0);
}

TEST_CASE("visit_lambdas returns a reference when the lambdas do", "[util]")
{
  auto v = std::variant<int, double>{1};
  auto& ref = visit_lambdas(
    v, [](int& e) -> int& { return e; }, [](double&) -> int& { throw std::logic_error{"unexpected alternative"}; }
  );
  ref = 42;
  CHECK(std::get<int>(v) == 42);
}

} // namespace bibstd::util
