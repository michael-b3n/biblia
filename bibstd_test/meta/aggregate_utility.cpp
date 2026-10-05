#include <bibstd/meta/aggregate_utility.hpp>

#include <catch2/catch_test_macros.hpp>

#include <tuple>
#include <type_traits>

namespace bibstd::meta
{

template<typename T1 = int, typename T2 = int, typename T3 = int>
struct test_aggregate final
{
  T1 t1;
  T2 t2;
  T3 t3;
};

TEST_CASE("aggregate_utility", "[meta]")
{
  static_assert(data_member_count<test_aggregate<int&>>() == 3);
  static_assert(std::is_same_v<to_tuple_t<test_aggregate<>>, std::tuple<int, int, int>>);
  static_assert(std::is_same_v<to_tuple_t<test_aggregate<int&, int&&, const int&>>, std::tuple<int, int, int>>);

  using test_aggregate_t = test_aggregate<int, double, test_aggregate<int, double, int>>;
  constexpr auto aggregate = test_aggregate_t{
    .t1 = 1, .t2 = 2.0, .t3 = test_aggregate{.t1 = 3, .t2 = 4.0, .t3 = 5}
  };
  constexpr auto tuple = to_tuple(aggregate);
  CHECK(std::get<0>(tuple) == 1);
  CHECK(std::get<1>(tuple) == 2);

  constexpr auto tuple_inner = to_tuple(std::get<2>(tuple));
  CHECK(std::get<0>(tuple_inner) == 3);
  CHECK(std::get<1>(tuple_inner) == 4.0);
  CHECK(std::get<2>(tuple_inner) == 5);

  constexpr auto to_aggregate = from_tuple<test_aggregate_t>(tuple);
  CHECK(to_aggregate.t1 == 1);
  CHECK(to_aggregate.t2 == 2.0);
  CHECK(to_aggregate.t3.t1 == 3);
  CHECK(to_aggregate.t3.t2 == 4.0);
  CHECK(to_aggregate.t3.t3 == 5);
}

} // namespace bibstd::meta
