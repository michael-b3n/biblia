#pragma once

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace bibstd::meta
{

///
/// Default lossless conversion checker type. False by default.
/// \tparam F From type
/// \tparam T To type
///
template<typename F, typename T>
struct is_lossless_conversion : std::false_type
{
  using from_type = F;
  using to_type = T;
};

///
/// Checks conversion between same types. True by default.
/// \tparam F From type
/// \tparam T To type
///
template<typename F, typename T>
  requires(std::is_arithmetic_v<F> && std::is_same_v<std::remove_cvref_t<F>, std::remove_cvref_t<T>>)
struct is_lossless_conversion<F, T> : std::true_type
{
  using from_type = F;
  using to_type = T;
};

#define BIBSTD_REGISTER_LOSSLESS_CONVERSION(F, T)                                                                              \
  template<>                                                                                                                   \
  struct is_lossless_conversion<F, T> : std::true_type                                                                         \
  {                                                                                                                            \
    using from_type = F;                                                                                                       \
    using to_type = T;                                                                                                         \
  };

BIBSTD_REGISTER_LOSSLESS_CONVERSION(bool, std::int8_t);
BIBSTD_REGISTER_LOSSLESS_CONVERSION(bool, std::uint8_t);
BIBSTD_REGISTER_LOSSLESS_CONVERSION(bool, std::int16_t);
BIBSTD_REGISTER_LOSSLESS_CONVERSION(bool, std::uint16_t);
BIBSTD_REGISTER_LOSSLESS_CONVERSION(bool, std::int32_t);
BIBSTD_REGISTER_LOSSLESS_CONVERSION(bool, std::uint32_t);
BIBSTD_REGISTER_LOSSLESS_CONVERSION(bool, std::int64_t);
BIBSTD_REGISTER_LOSSLESS_CONVERSION(bool, std::uint64_t);
BIBSTD_REGISTER_LOSSLESS_CONVERSION(bool, float);
BIBSTD_REGISTER_LOSSLESS_CONVERSION(bool, double);

BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::int8_t, std::int16_t);
BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::int8_t, std::int32_t);
BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::int8_t, std::int64_t);
BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::int8_t, float);
BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::int8_t, double);

BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::uint8_t, std::int16_t);
BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::uint8_t, std::uint16_t);
BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::uint8_t, std::int32_t)
BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::uint8_t, std::uint32_t)
BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::uint8_t, std::int64_t)
BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::uint8_t, std::uint64_t)
BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::uint8_t, float)
BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::uint8_t, double)

BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::int16_t, std::int32_t);
BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::int16_t, std::int64_t);
BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::int16_t, float);
BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::int16_t, double);

BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::uint16_t, std::int32_t);
BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::uint16_t, std::uint32_t);
BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::uint16_t, std::int64_t)
BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::uint16_t, std::uint64_t)
BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::uint16_t, float)
BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::uint16_t, double)

BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::int32_t, std::int64_t);
BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::int32_t, double);

BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::uint32_t, std::int64_t);
BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::uint32_t, std::uint64_t);
BIBSTD_REGISTER_LOSSLESS_CONVERSION(std::uint32_t, double);

BIBSTD_REGISTER_LOSSLESS_CONVERSION(float, double);

#undef BIBSTD_REGISTER_LOSSLESS_CONVERSION

///
/// Boolean indicating if a conversion between two types is lossless or not.
/// \tparam F From type
/// \tparam T To type
///
template<typename F, typename T>
inline constexpr auto is_lossless_conversion_v = is_lossless_conversion<F, T>::value;

///
/// Concept checking if type F is lossless convertible to type T.
/// \tparam F From type
/// \tparam T To type
///
template<typename F, typename T>
concept lossless_convertible = is_lossless_conversion_v<F, T>;

///
/// Type trait struct to find a common type between two types, such that
/// both types can be converted without any loss to this type.
/// If no such type is found typedef `type` is void.
/// \tparam T1 First arithmetic type
/// \tparam T2 Second arithmetic type
/// \tparam ...T Arithmetic types
///
template<typename... T>
struct lossless_common_type;

///
/// \see lossless_common_type
///
template<>
struct lossless_common_type<void> final
{
  using type = void;
};

///
/// \see lossless_common_type
///
template<typename T>
  requires(std::is_arithmetic_v<T>)
struct lossless_common_type<T> final
{
  using type = std::remove_cvref_t<T>;
};

///
/// \see lossless_common_type
///
template<typename T1, typename T2, typename... T>
  requires(std::is_arithmetic_v<T1> && std::is_same_v<std::remove_cvref_t<T1>, std::remove_cvref_t<T2>>)
struct lossless_common_type<T1, T2, T...> final
{
  using type = typename lossless_common_type<std::remove_cvref_t<T1>, T...>::type;
};

///
/// \see lossless_common_type
///
template<std::integral TI, std::floating_point TF, typename... T>
struct lossless_common_type<TI, TF, T...> final
{
  using type = typename lossless_common_type<
    std::conditional_t<
      is_lossless_conversion_v<TI, TF>,
      std::remove_cvref_t<TF>,
      std::conditional_t<is_lossless_conversion_v<TI, double>, double, void>>,
    T...>::type;
};

///
/// \see lossless_common_type
///
template<std::floating_point TF, std::integral TI, typename... T>
struct lossless_common_type<TF, TI, T...> final
{
  using type = typename lossless_common_type<
    std::conditional_t<
      is_lossless_conversion_v<TI, TF>,
      std::remove_cvref_t<TF>,
      std::conditional_t<is_lossless_conversion_v<TI, double>, double, void>>,
    T...>::type;
};

///
/// \see lossless_common_type
///
template<std::floating_point T1, std::floating_point T2, typename... T>
  requires(!std::is_same_v<std::remove_cvref_t<T1>, std::remove_cvref_t<T2>>)
struct lossless_common_type<T1, T2, T...> final
{
  using type = typename lossless_common_type<double, T...>::type;
};

namespace detail
{

///
/// Helper for lossless integral common type.
/// \see lossless_common_type
///
template<typename T1, typename T2>
struct integral_common_type
{
private: // Typedefs
  using T1_ = std::remove_cvref_t<T1>;
  using T2_ = std::remove_cvref_t<T2>;

  template<typename I1, typename I2>
  using unsigned_common_type = std::conditional_t<
    is_lossless_conversion_v<I1, std::uint8_t> && is_lossless_conversion_v<I2, std::uint8_t>,
    std::uint8_t,
    std::conditional_t<
      is_lossless_conversion_v<I1, std::uint16_t> && is_lossless_conversion_v<I2, std::uint16_t>,
      std::uint16_t,
      std::conditional_t<
        is_lossless_conversion_v<I1, std::uint32_t> && is_lossless_conversion_v<I2, std::uint32_t>,
        std::uint32_t,
        std::conditional_t<
          is_lossless_conversion_v<I1, std::uint64_t> && is_lossless_conversion_v<I2, std::uint64_t>,
          std::uint64_t,
          void>>>>;

  template<typename I1, typename I2>
  using signed_common_type = std::conditional_t<
    is_lossless_conversion_v<I1, std::int8_t> && is_lossless_conversion_v<I2, std::int8_t>,
    std::int8_t,
    std::conditional_t<
      is_lossless_conversion_v<I1, std::int16_t> && is_lossless_conversion_v<I2, std::int16_t>,
      std::int16_t,
      std::conditional_t<
        is_lossless_conversion_v<I1, std::int32_t> && is_lossless_conversion_v<I2, std::int32_t>,
        std::int32_t,
        std::conditional_t<
          is_lossless_conversion_v<I1, std::int64_t> && is_lossless_conversion_v<I2, std::int64_t>,
          std::int64_t,
          void>>>>;

public: // Typedefs
  using type = std::conditional_t<
    std::is_unsigned_v<T1_> && std::is_unsigned_v<T2_>,
    unsigned_common_type<T1_, T2_>,
    signed_common_type<T1_, T2_>>;
};

template<typename T1, typename T2>
using integral_common_type_t = typename integral_common_type<T1, T2>::type;

} // namespace detail

///
/// \see lossless_common_type
///
template<std::integral T1, std::integral T2, typename... T>
  requires(!std::is_same_v<std::remove_cvref_t<T1>, std::remove_cvref_t<T2>>)
struct lossless_common_type<T1, T2, T...> final
{
public: // Typedefs
  using type = typename lossless_common_type<
    std::conditional_t<
      is_lossless_conversion_v<T1, T2>,
      std::remove_cvref_t<T2>,
      std::conditional_t<is_lossless_conversion_v<T2, T1>, std::remove_cvref_t<T1>, detail::integral_common_type_t<T1, T2>>>,
    T...>::type;
};

///
/// Helper type to directly extract the lossless common type.
/// \see lossless_common_type
///
template<typename... T>
using lossless_common_type_t = lossless_common_type<T...>::type;

///
/// Concept to check if two types share a common type
/// to which both types can be converted without loss.
/// \see lossless_common_type
///
template<typename... T>
concept has_lossless_common_type = !std::is_void_v<lossless_common_type_t<T...>>;

} // namespace bibstd::meta
