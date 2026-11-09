#include <arca/util/bitflags.hpp>

#include <catch2/catch_test_macros.hpp>

namespace bibstd::util
{

///
/// Test enum for bitflags testing.
///
enum class test_flags
{
  flag_a,
  flag_b,
  flag_c,
  flag_d,
  flag_e,
};

TEST_CASE("bitflags construction", "[util]")
{
  using flags = bitflags<test_flags>;
  using enum test_flags;

  static_assert(flags{}.none());
  static_assert(!flags{}.test(flag_a));
  static_assert(!flags{flag_a}.none());
  static_assert(flags{flag_a}.test(flag_a));
  static_assert(!flags{flag_a}.test(flag_b));
  static_assert(flags{flag_a, flag_b}.test(flag_a));
  static_assert(flags{flag_a, flag_b}.test(flag_b));
  static_assert(!flags{flag_a, flag_b}.test(flag_c));
  static_assert(flags{flag_a, flag_b, flag_c}.test(flag_a));
  static_assert(flags{flag_a, flag_b, flag_c}.test(flag_b));
  static_assert(flags{flag_a, flag_b, flag_c}.test(flag_c));
  static_assert(!flags{flag_a, flag_b, flag_c}.test(flag_d));
}

TEST_CASE("bitflags set/reset/flip", "[util]")
{
  using flags = bitflags<test_flags>;
  using enum test_flags;
  {
    constexpr auto test_set = []()
    {
      auto f = flags{};
      f.set(flag_a);
      return f.test(flag_a) && !f.test(flag_b);
    };
    static_assert(test_set());
  }
  {
    constexpr auto test_set_multiple = []()
    {
      auto f = flags{};
      f.set(flag_a);
      f.set(flag_b);
      f.set(flag_c);
      return f.test(flag_a) && f.test(flag_b) && f.test(flag_c) && !f.test(flag_d);
    };
    static_assert(test_set_multiple());
  }
  {
    constexpr auto test_reset = []()
    {
      auto f = flags{flag_a, flag_b};
      f.reset(flag_a);
      return !f.test(flag_a) && f.test(flag_b);
    };
    static_assert(test_reset());
  }
  {
    constexpr auto test_reset_all_individual = []()
    {
      auto f = flags{flag_a, flag_b, flag_c};
      f.reset(flag_a);
      f.reset(flag_b);
      f.reset(flag_c);
      return f.none();
    };
    static_assert(test_reset_all_individual());
  }
  {
    constexpr auto test_reset_all = []()
    {
      auto f = flags{flag_a, flag_b, flag_c};
      f.reset();
      return f.none();
    };
    static_assert(test_reset_all());
  }
  {
    constexpr auto test_reset_all_empty = []()
    {
      auto f = flags{};
      f.reset();
      return f.none();
    };
    static_assert(test_reset_all_empty());
  }
  {
    constexpr auto test_flip = []()
    {
      auto f = flags{flag_a};
      f.flip(flag_a);
      const auto first = !f.test(flag_a);
      f.flip(flag_a);
      const auto second = f.test(flag_a);
      return first && second;
    };
    static_assert(test_flip());
  }
  {
    constexpr auto test_flip_chaining = []()
    {
      auto f = flags{};
      f.flip(flag_a).flip(flag_b);
      return f.test(flag_a) && f.test(flag_b);
    };
    static_assert(test_flip_chaining());
  }
  {
    constexpr auto test_flip_all = []()
    {
      auto f = flags{flag_a, flag_c};
      f.flip();
      return !f.test(flag_a) && f.test(flag_b) && !f.test(flag_c) && f.test(flag_d) && f.test(flag_e);
    };
    static_assert(test_flip_all());
  }
  {
    constexpr auto test_flip_multiple = []()
    {
      auto f = flags{};
      f.flip(flag_b);
      f.flip(flag_b);
      f.flip(flag_b);
      return f.test(flag_b);
    };
    static_assert(test_flip_multiple());
  }
}

TEST_CASE("bitflags test/has", "[util]")
{
  using flags = bitflags<test_flags>;
  using enum test_flags;

  static_assert(flags{flag_a}.test(flag_a));
  static_assert(!flags{flag_a}.test(flag_b));
  static_assert(!flags{}.test(flag_a));
  static_assert(flags{flag_a}.has(flag_a));
  static_assert(!flags{flag_a}.has(flag_b));
  static_assert(!flags{}.has(flag_a));
  static_assert(flags{flag_a, flag_b}.test(flag_a) == flags{flag_a, flag_b}.has(flag_a));
  static_assert(flags{flag_a, flag_b}.test(flag_c) == flags{flag_a, flag_b}.has(flag_c));
}

TEST_CASE("bitflags has_any/has_all", "[util]")
{
  using flags = bitflags<test_flags>;
  using enum test_flags;

  static_assert(!flags{}.has_any(flags{}));
  static_assert(!flags{}.has_any(flags{flag_a}));
  static_assert(!flags{flag_a}.has_any(flags{}));
  static_assert(flags{flag_a}.has_any(flags{flag_a}));
  static_assert(!flags{flag_a}.has_any(flags{flag_b}));
  static_assert(flags{flag_a, flag_b}.has_any(flags{flag_a}));
  static_assert(flags{flag_a, flag_b}.has_any(flags{flag_b}));
  static_assert(flags{flag_a, flag_b}.has_any(flags{flag_a, flag_b}));
  static_assert(flags{flag_a, flag_b}.has_any(flags{flag_a, flag_c}));
  static_assert(!flags{flag_a, flag_b}.has_any(flags{flag_c, flag_d}));
  static_assert(flags{}.has_all(flags{}));
  static_assert(!flags{}.has_all(flags{flag_a}));
  static_assert(flags{flag_a}.has_all(flags{}));
  static_assert(flags{flag_a}.has_all(flags{flag_a}));
  static_assert(!flags{flag_a}.has_all(flags{flag_b}));
  static_assert(!flags{flag_a}.has_all(flags{flag_a, flag_b}));
  static_assert(flags{flag_a, flag_b}.has_all(flags{flag_a}));
  static_assert(flags{flag_a, flag_b}.has_all(flags{flag_b}));
  static_assert(flags{flag_a, flag_b}.has_all(flags{flag_a, flag_b}));
  static_assert(!flags{flag_a, flag_b}.has_all(flags{flag_a, flag_b, flag_c}));
  static_assert(!flags{flag_a, flag_b}.has_all(flags{flag_c}));
}

TEST_CASE("bitflags all/any/none", "[util]")
{
  using flags = bitflags<test_flags>;
  using enum test_flags;

  static_assert(flags{}.none());
  static_assert(!flags{flag_a}.none());
  static_assert(!flags{flag_a, flag_b}.none());
  static_assert(!flags{}.any());
  static_assert(flags{flag_a}.any());
  static_assert(flags{flag_a, flag_b}.any());
  static_assert(flags{flag_a, flag_b, flag_c, flag_d, flag_e}.all());
  static_assert(!flags{flag_a, flag_b, flag_c, flag_d}.all());
  static_assert(!flags{}.all());
  {
    constexpr auto test_none = []()
    {
      auto f = flags{flag_a};
      f.reset(flag_a);
      return f.none();
    };
    static_assert(test_none());
  }
}

TEST_CASE("bitflags count/size", "[util]")
{
  using flags = bitflags<test_flags>;
  using enum test_flags;

  static_assert(flags{}.count() == 0);
  static_assert(flags{flag_a}.count() == 1);
  static_assert(flags{flag_a, flag_b}.count() == 2);
  static_assert(flags{flag_a, flag_b, flag_c}.count() == 3);
  static_assert(flags{flag_a, flag_b, flag_c, flag_d, flag_e}.count() == 5);
  static_assert(flags{}.size() == 5);
  static_assert(flags{flag_a}.size() == 5);
  static_assert(flags{flag_a, flag_b, flag_c, flag_d, flag_e}.size() == 5);
}

TEST_CASE("bitflags operator|=", "[util]")
{
  using flags = bitflags<test_flags>;
  using enum test_flags;
  {
    constexpr auto test_or_flags = []()
    {
      auto f1 = flags{flag_a};
      auto f2 = flags{flag_b};
      f1 |= f2;
      return f1.test(flag_a) && f1.test(flag_b);
    };
    static_assert(test_or_flags());
  }
  {
    constexpr auto test_or_enum = []()
    {
      auto f = flags{flag_a};
      f |= flag_b;
      return f.test(flag_a) && f.test(flag_b);
    };
    static_assert(test_or_enum());
  }
  {
    constexpr auto test_or_multiple = []()
    {
      auto f = flags{};
      f |= flag_a;
      f |= flag_b;
      f |= flags{flag_c, flag_d};
      return f.test(flag_a) && f.test(flag_b) && f.test(flag_c) && f.test(flag_d);
    };
    static_assert(test_or_multiple());
  }
}

TEST_CASE("bitflags operator&=", "[util]")
{
  using flags = bitflags<test_flags>;
  using enum test_flags;
  {
    constexpr auto test_and_flags = []()
    {
      auto f1 = flags{flag_a, flag_b};
      auto f2 = flags{flag_a, flag_c};
      f1 &= f2;
      return f1.test(flag_a) && !f1.test(flag_b) && !f1.test(flag_c);
    };
    static_assert(test_and_flags());
  }
  {
    constexpr auto test_and_enum = []()
    {
      auto f = flags{flag_a, flag_b};
      f &= flag_a;
      return f.test(flag_a) && !f.test(flag_b);
    };
    static_assert(test_and_enum());
  }
  {
    constexpr auto test_and_empty = []()
    {
      auto f = flags{flag_a, flag_b};
      f &= flags{flag_c, flag_d};
      return f.none();
    };
    static_assert(test_and_empty());
  }
}

TEST_CASE("bitflags operator^=", "[util]")
{
  using flags = bitflags<test_flags>;
  using enum test_flags;

  {
    constexpr auto test_xor_flags = []()
    {
      auto f1 = flags{flag_a, flag_b};
      auto f2 = flags{flag_b, flag_c};
      f1 ^= f2;
      return f1.test(flag_a) && !f1.test(flag_b) && f1.test(flag_c);
    };
    static_assert(test_xor_flags());
  }
  {
    constexpr auto test_xor_enum = []()
    {
      auto f = flags{flag_a, flag_b};
      f ^= flag_b;
      return f.test(flag_a) && !f.test(flag_b);
    };
    static_assert(test_xor_enum());
  }
  {
    constexpr auto test_xor_toggle = []()
    {
      auto f = flags{flag_a};
      f ^= flag_a;
      const auto first = f.none();
      f ^= flag_a;
      const auto second = f.test(flag_a);
      return first && second;
    };
    static_assert(test_xor_toggle());
  }
}

TEST_CASE("bitflags operator~", "[util]")
{
  using flags = bitflags<test_flags>;
  using enum test_flags;
  {
    constexpr auto test_not = []()
    {
      auto f = flags{flag_a, flag_b};
      auto inverted = ~f;
      // After NOT, flag_a and flag_b should be cleared, others set
      return !inverted.test(flag_a) && !inverted.test(flag_b) && inverted.test(flag_c);
    };
    static_assert(test_not());
  }
  {
    constexpr auto test_double_not = []()
    {
      auto f = flags{flag_a};
      auto inverted = ~f;
      auto double_inverted = ~inverted;
      // Double negation should restore flag_a
      return double_inverted.test(flag_a);
    };
    static_assert(test_double_not());
  }
  {
    constexpr auto test_not_empty = []()
    {
      auto f = flags{};
      auto inverted = ~f;
      // All flags should be set after NOT on empty
      return inverted.test(flag_a) && inverted.test(flag_b) && inverted.test(flag_c);
    };
    static_assert(test_not_empty());
  }
}

TEST_CASE("bitflags operator==", "[util]")
{
  using flags = bitflags<test_flags>;
  using enum test_flags;

  static_assert(flags{} == flags{});
  static_assert(flags{flag_a} == flags{flag_a});
  static_assert(flags{flag_a, flag_b} == flags{flag_a, flag_b});
  static_assert(flags{flag_a, flag_b} == flags{flag_b, flag_a}); // Order doesn't matter
  static_assert(flags{} != flags{flag_a});
  static_assert(flags{flag_a} != flags{flag_b});
  static_assert(flags{flag_a, flag_b} != flags{flag_a, flag_c});
}

TEST_CASE("bitflags complex scenarios", "[util]")
{
  using flags = bitflags<test_flags>;
  using enum test_flags;
  {
    constexpr auto test_build = []()
    {
      auto f = flags{};
      f.set(flag_a);
      f |= flag_b;
      f.flip(flag_c);
      return f.test(flag_a) && f.test(flag_b) && f.test(flag_c) && !f.test(flag_d);
    };
    static_assert(test_build());
  }
  {
    constexpr auto test_modify = []()
    {
      auto f = flags{flag_a, flag_b, flag_c};
      f.reset(flag_b);
      f.flip(flag_d);
      return f.has_all(flags{flag_a, flag_c, flag_d}) && !f.test(flag_b);
    };
    static_assert(test_modify());
  }
  {
    constexpr auto test_combine = []()
    {
      auto f1 = flags{flag_a, flag_b};
      auto f2 = flags{flag_c, flag_d};
      f1 |= f2;
      return f1.has_all(flags{flag_a, flag_b, flag_c, flag_d});
    };
    static_assert(test_combine());
  }
  {
    constexpr auto test_filter = []()
    {
      auto f = flags{flag_a, flag_b, flag_c, flag_d};
      f &= flags{flag_a, flag_c};
      return f.has_all(flags{flag_a, flag_c}) && !f.test(flag_b) && !f.test(flag_d);
    };
    static_assert(test_filter());
  }
  {
    constexpr auto test_xor_multi = []()
    {
      auto f = flags{flag_a, flag_c};
      f ^= flags{flag_b, flag_c};
      return f.test(flag_a) && f.test(flag_b) && !f.test(flag_c);
    };
    static_assert(test_xor_multi());
  }
}

} // namespace bibstd::util
