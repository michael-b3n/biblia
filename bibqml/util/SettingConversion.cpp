#include "bibqml/util/SettingConversion.hpp"

#include <bibstd/framework/setting_type_erased.hpp>
#include <bibstd/framework/setting_validator.hpp>
#include <bibstd/meta/chrono.hpp>
#include <bibstd/meta/type_traits.hpp>
#include <bibstd/util/exception.hpp>
#include <bibstd/util/log.hpp>
#include <bibstd/util/numeric_cast.hpp>
#include <bibstd/util/visit_helper.hpp>

#include <QColor>
#include <QJSValue>
#include <QLatin1StringView>
#include <QMetaType>

#include <algorithm>
#include <chrono>
#include <concepts>
#include <cstdint>
#include <format>
#include <optional>
#include <ranges>
#include <type_traits>
#include <vector>

namespace bibqml
{
namespace
{

// Typedefs
template<typename T>
using setting_ptr = bibstd::util::non_owning_ptr<bibstd::framework::setting_type_erased<T>>;
// clang-format off
template<typename T>
struct setting_value_type { using type = T; };
template<typename T>
struct setting_value_type<std::optional<T>> { using type = T; };
template<typename T>
struct setting_value_type<std::vector<T>> { using type = T; };
// clang-format on
template<typename T>
using setting_raw_value_type = typename setting_value_type<typename std::remove_pointer_t<T>::value_type>::type;
template<typename T>
inline constexpr bool always_false_v = false;
template<typename T>
using validator_range_sptr = bibstd::framework::setting_validator_range_type_erased<T>::sptr_type;
template<typename T>
using validator_list_sptr = bibstd::framework::setting_validator_list_type_erased<T>::sptr_type;
using validator_unbound_sptr = bibstd::framework::setting_validator_unbound::sptr_type;

///
/// Protected numeric cast between integers.
/// \return optional casted integer, std::nullopt if cast fails
///
template<std::integral T>
auto integer_cast(const std::integral auto value) -> std::optional<T>
{
  try
  {
    const auto v = numeric_cast<T>(value);
    return v;
  }
  catch(...)
  {
    LOG_ERROR("bad numeric cast to {}:  {}", typeid(T).name(), bibstd::util::exception_report());
    return std::nullopt;
  }
}

///
/// Report an unexpected type mismatch and return false.
/// \return false in any case needed for setValue overload
///
auto throw_type_mismatch_error(const auto setting, const auto& value) -> bool
{
  using s_type = typename std::remove_pointer_t<std::remove_const_t<decltype(setting)>>::value_type;
  using v_type = std::remove_cvref_t<decltype(value)>;
  throw bibstd::util::exception{
    std::format(R"(type mismatch: setting_type="{}", value_type="{}")", typeid(s_type).name(), typeid(v_type).name())
  };
  return false;
}

///
/// \return QVariant converted from a boolean
///
auto toVariant(const bool& value) -> QVariant
{
  return {value};
}

///
/// \return QVariant converted from an integral
///
template<std::integral T>
auto toVariant(const T value) -> QVariant
{
  const auto v = integer_cast<int>(value);
  return v ? QVariant(*v) : QVariant{};
}

///
/// \return QVariant converted from a double
///
auto toVariant(const double& value) -> QVariant
{
  return {value};
}

///
/// Convert a duration to the count of the period it is stored in, so that a value survives the
/// way to QML and back unrounded. The unit of that period names the setting, \see toQmlPostfix.
/// \return QVariant converted from a duration
///
template<typename T>
  requires(bibstd::meta::is_duration_v<T>)
auto toVariant(const T& value) -> QVariant
{
  return toVariant(value.count());
}

///
/// \return QVariant containing QString converted from a std::string
///
auto toVariant(const std::string& value) -> QVariant
{
  return QString::fromStdString(value);
}

///
/// \return QVariant containing QString converted from a path
///
auto toVariant(const std::filesystem::path& value) -> QVariant
{
  return QString::fromStdString(value.string());
}

///
/// \return QVariant with a value or empty converted from a std::optional
///
template<typename T>
auto toVariant(const std::optional<T>& value) -> QVariant
{
  return value ? toVariant(*value) : QVariant{};
}

///
/// \return QVariant containing QVariantList converted from a std::vector type
///
template<typename T>
auto toVariant(const std::vector<T>& list) -> QVariant
{
  QList<QVariant> result;
  result.reserve(static_cast<int>(list.size()));
  std::ranges::for_each(list, [&](const auto& item) { result.emplace_back(toVariant(item)); });
  return result;
}

///
/// Unit of the period a duration setting is stored in.
/// \return unit the duration is displayed with
///
template<typename T>
  requires(bibstd::meta::is_duration_v<T>)
constexpr auto durationPostfix() -> QLatin1StringView
{
  // clang-format off
  if constexpr(std::is_same_v<T, std::chrono::milliseconds>) { return QLatin1StringView{"ms"}; }
  else if constexpr(std::is_same_v<T, std::chrono::seconds>) { return QLatin1StringView{"s"}; }
  else if constexpr(std::is_same_v<T, std::chrono::minutes>) { return QLatin1StringView{"min"}; }
  else { static_assert(always_false_v<T>, "Unsupported duration setting type"); }
  // clang-format on
}

///
/// Try to convert a QJSValue type inside a QVariant to a proper QVariant type.
/// This is needed since QML might provide special Java script types that do
/// not belong to a non user meta type.
/// \returns QVariant type that is converted to QVariant if it was a user type.
///
auto tryNormalizeQVariant(const QVariant& variant) -> QVariant
{
  const auto typeId = variant.metaType().id();
  if(typeId >= QMetaType::User && variant.canConvert<QJSValue>())
  {
    // came through as a JS array wrapped in QJSValue — unwrap it properly
    return variant.value<QJSValue>().toVariant();
  }
  return variant;
}

///
/// Set a convertible (canonically convertible) value of a type erased setting from a QVariant.
/// \return true if the value was set, false otherwise
///
auto setValueConvertible(const auto setting, const auto& value) -> bool
{
  if constexpr(std::is_convertible_v<std::remove_cvref_t<decltype(value)>, setting_raw_value_type<decltype(setting)>>)
  {
    return setting->value(value);
  }
  else
  {
    return throw_type_mismatch_error(setting, value);
  }
}

///
/// Set a convertible vector value of a type erased setting from a QVariant.
/// \return true if the value was set, false otherwise
///
auto setValueConvertibleVec(const auto setting, const auto& value) -> bool
{
  using vector_value_type = bibstd::meta::remove_wrapper_t<decltype(value)>;
  static_assert(std::is_same_v<std::vector<vector_value_type>, std::remove_cvref_t<decltype(value)>>);
  if constexpr(std::is_same_v<vector_value_type, setting_raw_value_type<decltype(setting)>>)
  {
    return setting->value(value);
  }
  else if constexpr(std::is_convertible_v<vector_value_type, setting_raw_value_type<decltype(setting)>>)
  {
    return setting->value(
      value | std::views::transform([&](const auto& v) { return static_cast<setting_raw_value_type<decltype(setting)>>(v); }) |
      std::ranges::to<std::vector>()
    );
  }
  else
  {
    return throw_type_mismatch_error(setting, value);
  }
}

///
/// Set an integer value of a type erased setting from a QVariant.
/// \return true if the value was set, false otherwise
///
auto setValueInteger(const auto setting, const auto& value) -> bool
{
  static_assert(std::integral<setting_raw_value_type<decltype(setting)>>);
  if constexpr(std::integral<std::remove_cvref_t<decltype(value)>>)
  {
    const auto v = integer_cast<setting_raw_value_type<decltype(setting)>>(value);
    return v ? setting->value(*v) : false;
  }
  else
  {
    return throw_type_mismatch_error(setting, value);
  }
}

///
/// Set an integer vector value of a type erased setting from a QVariant.
/// \return true if the value was set, false otherwise
///
auto setValueIntegerVec(const auto setting, const auto& value) -> bool
{
  static_assert(std::integral<setting_raw_value_type<decltype(setting)>>);
  using vector_value_type = bibstd::meta::remove_wrapper_t<decltype(value)>;
  static_assert(std::is_same_v<std::vector<vector_value_type>, std::remove_cvref_t<decltype(value)>>);
  if constexpr(std::is_same_v<vector_value_type, setting_raw_value_type<decltype(setting)>>)
  {
    return setting->value(value);
  }
  else if constexpr(std::integral<vector_value_type>)
  {
    auto dest = std::vector<setting_raw_value_type<decltype(setting)>>{};
    dest.reserve(value.size());
    auto result = std::ranges::all_of(
      value,
      [&](const auto v)
      {
        if(const auto i = integer_cast<setting_raw_value_type<decltype(setting)>>(v))
        {
          dest.push_back(*i);
          return true;
        }
        return false;
      }
    );
    if(result)
    {
      result = setting->value(dest);
    }
    return result;
  }
  else
  {
    return throw_type_mismatch_error(setting, value);
  }
}

///
/// Set a duration value of a type erased setting from a QVariant. The value counts the period
/// the setting is stored in, \see toVariant.
/// \return true if the value was set, false otherwise
///
auto setValueDuration(const auto setting, const auto value) -> bool
{
  using duration_type = setting_raw_value_type<decltype(setting)>;
  static_assert(bibstd::meta::is_duration_v<duration_type>);
  if constexpr(std::integral<std::remove_cvref_t<decltype(value)>>)
  {
    const auto count = integer_cast<typename duration_type::rep>(value);
    return count ? setting->value(duration_type{*count}) : false;
  }
  else
  {
    return throw_type_mismatch_error(setting, value);
  }
}

///
/// Set a duration vector value of a type erased setting from a QVariant.
/// \return true if the value was set, false otherwise
///
auto setValueDurationVec(const auto setting, const auto& value) -> bool
{
  using duration_type = setting_raw_value_type<decltype(setting)>;
  using vector_value_type = bibstd::meta::remove_wrapper_t<decltype(value)>;
  static_assert(bibstd::meta::is_duration_v<duration_type>);
  static_assert(std::is_same_v<std::vector<vector_value_type>, std::remove_cvref_t<decltype(value)>>);
  if constexpr(std::integral<vector_value_type>)
  {
    auto dest = std::vector<duration_type>{};
    dest.reserve(value.size());
    auto result = std::ranges::all_of(
      value,
      [&](const auto v)
      {
        if(const auto count = integer_cast<typename duration_type::rep>(v))
        {
          dest.emplace_back(duration_type{*count});
          return true;
        }
        return false;
      }
    );
    if(result)
    {
      result = setting->value(dest);
    }
    return result;
  }
  else
  {
    return throw_type_mismatch_error(setting, value);
  }
}

} // anonymous namespace

///
///
auto toDefaultSettingValue(const QVariant& value) -> std::optional<bibstd::framework::setting_type_erased_variant>
{
  const auto v = tryNormalizeQVariant(value);
  switch(v.metaType().id())
  {
  case QMetaType::Bool: return v.toBool();
  case QMetaType::Int: [[fallthrough]];
  case QMetaType::UInt: [[fallthrough]];
  case QMetaType::LongLong: [[fallthrough]];
  case QMetaType::ULongLong: return static_cast<std::int64_t>(v.toLongLong());
  case QMetaType::Float: [[fallthrough]];
  case QMetaType::Double: return v.toDouble();
  case QMetaType::QString: return v.toString().toStdString();
  case QMetaType::QColor: return v.value<QColor>().name(QColor::HexArgb).toStdString();
  default: return std::nullopt;
  }
}

///
///
auto toQmlValue(const SettingVariantType& setting) -> QVariant
{
  return std::visit([](const auto s) { return toVariant(s->value()); }, setting);
}

///
///
auto setQmlValue(const SettingVariantType& setting, const QVariant& value) -> bool
{
  static constexpr auto toVector = [](const auto& v, auto&& converter)
  {
    const auto& list = v.toList();
    auto vec = std::vector<std::invoke_result_t<decltype(converter), QVariant>>{};
    vec.reserve(static_cast<std::size_t>(list.size()));
    std::ranges::for_each(list, [&](const auto& variant) { vec.emplace_back(converter(variant)); });
    return vec;
  };

  const auto v = tryNormalizeQVariant(value);
  // clang-format off
  return bibstd::util::visit_lambdas(
    setting,
    [&](const setting_ptr<bool> s) { return setValueConvertible(s, v.toBool()); },
    [&](const setting_ptr<std::int32_t> s) { return setValueInteger(s, v.toInt()); },
    [&](const setting_ptr<std::int64_t> s) { return setValueInteger(s, v.toInt()); },
    [&](const setting_ptr<std::uint32_t> s) { return setValueInteger(s, v.toInt()); },
    [&](const setting_ptr<std::uint64_t> s) { return setValueInteger(s, v.toInt()); },
    [&](const setting_ptr<double> s) { return setValueConvertible(s, v.toDouble()); },
    [&](const setting_ptr<std::string> s) { return setValueConvertible(s, v.toString().toStdString()); },
    [&](const setting_ptr<std::chrono::milliseconds> s) { return setValueDuration(s, v.toInt()); },
    [&](const setting_ptr<std::chrono::seconds> s) { return setValueDuration(s, v.toInt()); },
    [&](const setting_ptr<std::chrono::minutes> s) { return setValueDuration(s, v.toInt()); },
    [&](const setting_ptr<std::filesystem::path> s) { return setValueConvertible(s, v.toString().toStdString()); },
    [&](const setting_ptr<std::optional<bool>> s) { return setValueConvertible(s, v.toBool()); },
    [&](const setting_ptr<std::optional<std::int32_t>> s) { return setValueInteger(s, v.toInt()); },
    [&](const setting_ptr<std::optional<std::int64_t>> s) { return setValueInteger(s, v.toInt()); },
    [&](const setting_ptr<std::optional<std::uint32_t>> s) { return setValueInteger(s, v.toInt()); },
    [&](const setting_ptr<std::optional<std::uint64_t>> s) { return setValueInteger(s, v.toInt()); },
    [&](const setting_ptr<std::optional<double>> s) { return setValueConvertible(s, v.toDouble()); },
    [&](const setting_ptr<std::optional<std::string>> s) { return setValueConvertible(s, v.toString().toStdString()); },
    [&](const setting_ptr<std::optional<std::chrono::milliseconds>> s) { return setValueDuration(s, v.toInt()); },
    [&](const setting_ptr<std::optional<std::chrono::seconds>> s) { return setValueDuration(s, v.toInt()); },
    [&](const setting_ptr<std::optional<std::chrono::minutes>> s) { return setValueDuration(s, v.toInt()); },
    [&](const setting_ptr<std::optional<std::filesystem::path>> s) { return setValueConvertible(s, v.toString().toStdString()); },
    [&](const setting_ptr<std::vector<std::int32_t>> s) { return setValueIntegerVec(s, toVector(v, [](const auto& e) { return e.toInt(); })); },
    [&](const setting_ptr<std::vector<std::int64_t>> s) { return setValueIntegerVec(s, toVector(v, [](const auto& e) { return e.toInt(); })); },
    [&](const setting_ptr<std::vector<std::uint32_t>> s) { return setValueIntegerVec(s, toVector(v, [](const auto& e) { return e.toInt(); })); },
    [&](const setting_ptr<std::vector<std::uint64_t>> s) { return setValueIntegerVec(s, toVector(v, [](const auto& e) { return e.toInt(); })); },
    [&](const setting_ptr<std::vector<double>> s) { return setValueConvertibleVec(s, toVector(v, [](const auto& e) { return e.toDouble(); })); },
    [&](const setting_ptr<std::vector<std::string>> s) { return setValueConvertibleVec(s, toVector(v, [](const auto& e) { return e.toString().toStdString(); })); },
    [&](const setting_ptr<std::vector<std::chrono::milliseconds>> s) { return setValueDurationVec(s, toVector(v, [](const auto& e) { return e.toInt(); })); },
    [&](const setting_ptr<std::vector<std::chrono::seconds>> s) { return setValueDurationVec(s, toVector(v, [](const auto& e) { return e.toInt(); })); },
    [&](const setting_ptr<std::vector<std::chrono::minutes>> s) { return setValueDurationVec(s, toVector(v, [](const auto& e) { return e.toInt(); })); },
    [&](const setting_ptr<std::vector<std::filesystem::path>> s) { return setValueConvertibleVec(s, toVector(v, [](const auto& e) { return e.toString().toStdString(); })); }
  );
  // clang-format on
}

///
///
auto toQmlPostfix(const SettingVariantType& setting) -> QString
{
  return std::visit(
    [](const auto s) -> QString
    {
      if(!s->postfix.empty())
      {
        return QString::fromStdString(s->postfix);
      }
      using value_type = setting_raw_value_type<std::remove_cvref_t<decltype(s)>>;
      if constexpr(bibstd::meta::is_duration_v<value_type>)
      {
        return durationPostfix<value_type>();
      }
      else
      {
        return {};
      }
    },
    setting
  );
}

///
///
auto toQmlValidatorValues(const SettingVariantType& setting) -> QList<QVariant>
{
  return std::visit(
    [](const auto s)
    {
      using value_type = std::remove_pointer_t<std::remove_cvref_t<decltype(s)>>::value_type;
      return bibstd::util::visit_lambdas(
        s->validator,
        []([[maybe_unused]] const validator_unbound_sptr&) { return QList<QVariant>{}; },
        []([[maybe_unused]] const validator_range_sptr<value_type>&) { return QList<QVariant>{}; },
        [](const validator_list_sptr<value_type>& v)
        {
          const auto available = v->available();
          QList<QVariant> result;
          result.reserve(static_cast<int>(available.size()));
          std::ranges::for_each(available, [&](const auto& item) { result.push_back(toVariant(item)); });
          return result;
        }
      );
    },
    setting
  );
}

} // namespace bibqml
