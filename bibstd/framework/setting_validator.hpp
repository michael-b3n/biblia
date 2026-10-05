#pragma once

#include "bibstd/framework/setting_common.hpp"
#include "bibstd/math/value_range.hpp"
#include "bibstd/meta/type_traits.hpp"
#include "bibstd/util/contains.hpp"
#include "bibstd/util/log.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <ranges>
#include <type_traits>

namespace bibstd::framework
{
namespace detail
{

///
/// Helper type to check if a type T is an optional.
///
template<typename T>
inline constexpr auto is_optional_v = std::is_same_v<std::optional<meta::remove_wrapper_t<T>>, T>;

///
/// Helper type to check if a type T is a vector.
///
template<typename T>
inline constexpr auto is_vector_v = std::is_same_v<std::vector<meta::remove_wrapper_t<T>>, T>;

///
/// Helper type removing either vector or optional wrappers if available.
/// If not available the type itself is returned.
///
template<typename T>
using remove_optional_or_vector_t = std::conditional_t<is_optional_v<T> || is_vector_v<T>, meta::remove_wrapper_t<T>, T>;

///
/// Concept for types that can be used as range type in setting_validator_range.
///
template<typename T>
concept setting_validator_range_type = underlying_setting_type<T> && std::is_arithmetic_v<T> && !std::is_same_v<T, bool>;

///
/// Helper type to extract validator type from smart pointer.
///
template<typename T>
using validator_t = std::remove_cvref_t<T>::element_type;

///
/// Helper concept to check if type erased validator can be constructed from specific validator type.
///
template<typename T, template<typename...> typename VT, typename V>
concept type_erased_validator_param_requirement =
  std::is_same_v<detail::validator_t<V>, VT<typename validator_t<V>::underlying_type>> &&
  std::is_same_v<T, setting_type_erased_type_from<typename validator_t<V>::underlying_type>>;

///
/// Connector class for setting validators. This class shall be used as base class for all setting validators.
///
class setting_validator_connector
{
  // Variables
  std::function<void()> on_changed_{nullptr};

public: // Structors
  setting_validator_connector() = default;
  virtual ~setting_validator_connector() = default;

public: // Accessors
  ///
  /// Register a callback to be called when the setting validator changes.
  ///
  auto connect_on_changed(std::function<void()> on_changed) -> void;

protected: // Functions
  ///
  /// Notify that the setting validator has changed.
  ///
  auto notify_changed() -> void;

protected: // Variables
  mutable std::mutex mtx_;
};

} // namespace detail

///
/// Class for unbound setting validators.
/// If a validator is unbound the setting value can have any value allowed by its type.
///
struct setting_validator_unbound final : public detail::setting_validator_connector
{
  // Typedefs
  using sptr_type = std::shared_ptr<setting_validator_unbound>;
};

///
/// Class for range type setting validators. The class range validator is only meaningful
/// for arithmetic types wrapped by optional or vectors.
/// The provided member functions `validate` and `contains` accept plain or optional types
/// and perform the range checks or validation on those single values. If the setting type
/// is a vector type, every single value of the vector is checked (with &&) or validated.
///
template<underlying_setting_type T>
class setting_validator_range final : public detail::setting_validator_connector
{
  // Typedefs
  using plain_underlying_type_ = detail::remove_optional_or_vector_t<T>;

  // Variables
  const math::value_range<
    std::conditional_t<detail::setting_validator_range_type<plain_underlying_type_>, plain_underlying_type_, int>>
    range_;

public: // Typedefs
  using sptr_type = std::shared_ptr<setting_validator_range<T>>;
  using underlying_type = T;
  using plain_underlying_type = plain_underlying_type_;

public: // Constants
  static constexpr auto is_plain_type = std::is_same_v<underlying_type, plain_underlying_type>;
  static constexpr auto is_optional_type = detail::is_optional_v<T>;
  static constexpr auto is_vector_type = detail::is_vector_v<T>;

public: // Structors
  setting_validator_range(plain_underlying_type min, plain_underlying_type max)
    requires(detail::setting_validator_range_type<plain_underlying_type>);

public: // Operators
  ///
  /// Validate value and return corrected value if necessary.
  /// \return corrected value if well defined validated value is available
  /// If well defined validated values is not available and the underlying
  /// setting type is optional, std::nullopt is returned.
  ///
  [[nodiscard]] auto validate(const T& value) const -> T
    requires(detail::setting_validator_range_type<T>);

  ///
  /// Validate overload for non plain underlying types (optional or vector). \see validate
  ///
  [[nodiscard]] auto validate(const plain_underlying_type& value) const -> plain_underlying_type
    requires(!is_plain_type && detail::setting_validator_range_type<T>);

public: // Operators
  ///
  /// Check if value is contained within the validator bounds.
  /// \return true if value is contained, false otherwise
  ///
  [[nodiscard]] auto contains(const T& value) const -> bool
    requires(detail::setting_validator_range_type<T>);

  ///
  /// Contains overload for non plain underlying types (optional or vector). \see contains
  ///
  [[nodiscard]] auto contains(const plain_underlying_type& value) const -> bool
    requires(!is_plain_type && detail::setting_validator_range_type<T>);

public: // Dummy implementation for unsupported types
  setting_validator_range(plain_underlying_type min, plain_underlying_type max)
    requires(!detail::setting_validator_range_type<plain_underlying_type>);
  [[nodiscard]] auto validate(const T& value) const -> T
    requires(!detail::setting_validator_range_type<T>);
  [[nodiscard]] auto validate(const plain_underlying_type& value) const -> plain_underlying_type
    requires(!is_plain_type && !detail::setting_validator_range_type<T>);
  [[nodiscard]] auto contains(const T& value) const -> bool
    requires(!detail::setting_validator_range_type<T>);
  [[nodiscard]] auto contains(const plain_underlying_type& value) const -> bool
    requires(!is_plain_type && !detail::setting_validator_range_type<T>);
};

///
/// Class for range type setting validators for type erased types.
/// \see setting_validator_range
///
template<underlying_setting_type_erased_type T>
class setting_validator_range_type_erased final
{
  // Typedefs
  using plain_underlying_type_ = detail::remove_optional_or_vector_t<T>;

  // Variables
  const std::function<T(const T&)> validate_;
  const std::function<plain_underlying_type_(const plain_underlying_type_&)> plain_validate_;
  const std::function<bool(const T&)> contains_;
  const std::function<bool(const plain_underlying_type_&)> plain_contains_;

public: // Typedefs
  using sptr_type = std::shared_ptr<setting_validator_range_type_erased<T>>;
  using underlying_type = T;
  using plain_underlying_type = plain_underlying_type_;

public: // Variables
  const std::shared_ptr<detail::setting_validator_connector> connector;

public: // Structors
  setting_validator_range_type_erased(const auto& validator)
    requires(detail::type_erased_validator_param_requirement<T, setting_validator_range, decltype(validator)>);

public: // Operators
  ///
  /// \see setting_validator_range::validate
  ///
  [[nodiscard]] auto validate(const T& value) const -> T;

  ///
  /// \see setting_validator_range::validate
  ///
  [[nodiscard]] auto validate(const plain_underlying_type& value) const -> plain_underlying_type
    requires(std::is_same_v<underlying_type, plain_underlying_type>);

public: // Operators
  ///
  /// \see setting_validator_range::contains
  ///
  [[nodiscard]] auto contains(const T& value) const -> bool;

  ///
  /// \see setting_validator_range::contains
  ///
  [[nodiscard]] auto contains(const plain_underlying_type& value) const -> bool
    requires(std::is_same_v<underlying_type, plain_underlying_type>);
};

///
/// Class for list type setting validators.
/// The provided getter and setter `available` accept plain types (no optional or vector types).
/// The contains checker accepts plain types (or optional types if the underlying setting type
/// is optional. If the underlying setting type is a vector type, every single value of the
/// vector must be checked.
///
template<underlying_setting_type T>
class setting_validator_list final : public detail::setting_validator_connector
{
  // Variables
  std::vector<detail::remove_optional_or_vector_t<T>> list_{};

public: // Typedefs
  using sptr_type = std::shared_ptr<setting_validator_list<T>>;
  using underlying_type = T;
  using plain_underlying_type = detail::remove_optional_or_vector_t<T>;

public: // Constants
  static constexpr auto is_plain_type = std::is_same_v<underlying_type, plain_underlying_type>;
  static constexpr auto is_optional_type = detail::is_optional_v<T>;
  static constexpr auto is_vector_type = detail::is_vector_v<T>;

public: // Structors
  setting_validator_list() = default;
  setting_validator_list(std::vector<plain_underlying_type> list);
  template<std::ranges::range R>
  setting_validator_list(const R& range)
    requires(std::is_same_v<std::ranges::range_value_t<R>, plain_underlying_type>);

public: // Accessors
  ///
  /// Check if the validator has any available values.
  /// \return true if no available values are set, false otherwise
  ///
  [[nodiscard]] auto empty() const -> bool;

  ///
  /// Get all available values.
  /// \return list of all available values
  ///
  [[nodiscard]] auto available() const -> std::vector<plain_underlying_type>;

public: // Modifiers
  ///
  /// Set the available values. This will replace all previously available values.
  /// \return true if list is valid and has been set, false otherwise
  ///
  template<std::ranges::range R>
  [[nodiscard]] auto available(const R& range) -> bool
    requires(std::is_same_v<std::ranges::range_value_t<R>, plain_underlying_type>);

public: // Operators
  ///
  /// Check if value is contained within the validator bounds.
  /// \return true if value is contained, false otherwise
  ///
  [[nodiscard]] auto contains(const T& value) const -> bool;

  ///
  /// Contains overload for non plain underlying types (optional or vector). \see contains
  ///
  [[nodiscard]] auto contains(const plain_underlying_type& value) const -> bool
    requires(!is_plain_type);
};

///
/// Class for list type setting validators for type erased types.
/// \see setting_validator_list
///
template<underlying_setting_type_erased_type T>
class setting_validator_list_type_erased final
{
  // Typedefs
  using plain_underlying_type_ = detail::remove_optional_or_vector_t<T>;

  // Variables
  const std::function<bool()> empty_;
  const std::function<std::vector<plain_underlying_type_>()> available_;
  const std::function<bool(const T&)> contains_;
  const std::function<bool(const plain_underlying_type_&)> plain_contains_;

public: // Typedefs
  using sptr_type = std::shared_ptr<setting_validator_list_type_erased<T>>;
  using underlying_type = T;
  using plain_underlying_type = plain_underlying_type_;

public: // Variables
  const std::shared_ptr<detail::setting_validator_connector> connector;

public: // Structors
  setting_validator_list_type_erased(const auto& validator)
    requires(detail::type_erased_validator_param_requirement<T, setting_validator_list, decltype(validator)>);

public: // Accessors
  ///
  /// \see setting_validator_list::empty
  ///
  [[nodiscard]] auto empty() const -> bool;

  ///
  /// \see setting_validator_list::available
  ///
  [[nodiscard]] auto available() const -> std::vector<plain_underlying_type>;

public: // Operators
  ///
  /// \see setting_validator_list::contains
  ///
  [[nodiscard]] auto contains(const T& value) const -> bool;

  ///
  /// \see setting_validator_list::contains
  ///
  [[nodiscard]] auto contains(const plain_underlying_type& value) const -> bool
    requires(std::is_same_v<underlying_type, plain_underlying_type>);
};

///
/// Setting validator variant type.
/// Optional underlying setting types are stripped away for validators.
///
template<underlying_setting_type T>
using setting_validator = std::variant<
  setting_validator_unbound::sptr_type,
  typename setting_validator_range<T>::sptr_type,
  typename setting_validator_list<T>::sptr_type>;

///
/// Setting validator type erased variant type.
/// Optional underlying setting types are stripped away for validators.
///
template<underlying_setting_type T>
using setting_validator_type_erased = std::variant<
  setting_validator_unbound::sptr_type,
  typename setting_validator_range_type_erased<T>::sptr_type,
  typename setting_validator_list_type_erased<T>::sptr_type>;

///
///
template<underlying_setting_type T>
setting_validator_range<T>::setting_validator_range(const plain_underlying_type min, const plain_underlying_type max)
  requires(detail::setting_validator_range_type<plain_underlying_type>)
  : range_{min, max}
{
  if constexpr(!is_optional_type)
  {
    if(math::empty(range_))
    {
      throw util::exception{"invalid empty range argument"};
    }
  }
}

///
///
template<underlying_setting_type T>
auto setting_validator_range<T>::validate(const T& value) const -> T
  requires(detail::setting_validator_range_type<T>)
{
  if constexpr(is_vector_type)
  {
    return value | std::views::transform([this](const plain_underlying_type v) { return validate(v); }) |
           std::ranges::to<std::vector>();
  }
  else if constexpr(is_optional_type)
  {
    if(math::empty(range_) || !value.has_value())
    {
      return std::nullopt;
    }
    return math::clamp(range_, *value);
  }
  else
  {
    // range must not be empty for non-optional setting types
    // \see setting_validator_range::setting_validator_range
    return math::clamp(range_, value);
  }
}

///
///
template<underlying_setting_type T>
auto setting_validator_range<T>::validate(const plain_underlying_type& value) const -> plain_underlying_type
  requires(!is_plain_type && detail::setting_validator_range_type<T>)
{
  if constexpr(is_optional_type)
  {
    if(math::empty(range_))
    {
      return std::nullopt;
    }
    return math::clamp(range_, value);
  }
  else
  {
    // range must not be empty for non-optional setting types
    // \see setting_validator_range::setting_validator_range
    return math::clamp(range_, value);
  }
}

///
///
template<underlying_setting_type T>
auto setting_validator_range<T>::contains(const T& value) const -> bool
  requires(detail::setting_validator_range_type<T>)
{
  if constexpr(is_vector_type)
  {
    return std::ranges::all_of(value, [this](const plain_underlying_type v) { return contains(v); });
  }
  else if constexpr(is_optional_type)
  {
    if(!value.has_value())
    {
      return true;
    }
    return math::contains(range_, *value);
  }
  else
  {
    return math::contains(range_, value);
  }
}

///
///
template<underlying_setting_type T>
auto setting_validator_range<T>::contains(const plain_underlying_type& value) const -> bool
  requires(!is_plain_type && detail::setting_validator_range_type<T>)
{
  return math::contains(range_, value);
}

///
///
template<underlying_setting_type T>
setting_validator_range<T>::setting_validator_range(
  [[maybe_unused]] plain_underlying_type /*min*/, [[maybe_unused]] plain_underlying_type
  /*max*/
)
  requires(!detail::setting_validator_range_type<plain_underlying_type>)
  : range_{0, 0}
{
  LOG_WARN("unsupported range validator construction: type=\"{}\"", typeid(T).name());
}

///
///
template<underlying_setting_type T>
auto setting_validator_range<T>::validate(const T& value) const -> T
  requires(!detail::setting_validator_range_type<T>)
{
  LOG_WARN("unsupported range validator validation: type=\"{}\"", typeid(T).name());
  return value;
}

///
///
template<underlying_setting_type T>
auto setting_validator_range<T>::validate(const plain_underlying_type& value) const -> plain_underlying_type
  requires(!is_plain_type && !detail::setting_validator_range_type<T>)
{
  LOG_WARN("unsupported range validator validation: type=\"{}\"", typeid(plain_underlying_type).name());
  return value;
}

///
///
template<underlying_setting_type T>
auto setting_validator_range<T>::contains([[maybe_unused]] const T& /*value*/) const -> bool
  requires(!detail::setting_validator_range_type<T>)
{
  LOG_WARN("unsupported range validator contains check: type=\"{}\"", typeid(T).name());
  return true;
}

///
///
template<underlying_setting_type T>
auto setting_validator_range<T>::contains([[maybe_unused]] const plain_underlying_type& /*value*/) const -> bool
  requires(!is_plain_type && !detail::setting_validator_range_type<T>)
{
  LOG_WARN("unsupported range validator contains check: type=\"{}\"", typeid(plain_underlying_type).name());
  return true;
}

///
///
template<underlying_setting_type_erased_type T>
setting_validator_range_type_erased<T>::setting_validator_range_type_erased(const auto& validator)
  requires(detail::type_erased_validator_param_requirement<T, setting_validator_range, decltype(validator)>)
  : validate_{[validator](const T& value)
              {
                using type = typename detail::validator_t<decltype(validator)>::underlying_type;
                static constexpr auto to_type_erased_value =
                  create_setting_value_converter<type, setting_type_erased_type_from<type>>();
                static constexpr auto from_type_erased_value =
                  create_setting_value_converter<setting_type_erased_type_from<type>, type>();
                return to_type_erased_value(validator->validate(from_type_erased_value(value)));
              }}
  , plain_validate_{[validator](const plain_underlying_type& value)
                    {
                      using type = typename detail::validator_t<decltype(validator)>::plain_underlying_type;
                      static constexpr auto to_type_erased_value =
                        create_setting_value_converter<type, setting_type_erased_type_from<type>>();
                      static constexpr auto from_type_erased_value =
                        create_setting_value_converter<setting_type_erased_type_from<type>, type>();
                      return to_type_erased_value(validator->validate(from_type_erased_value(value)));
                      return to_type_erased_value(validator->validate(from_type_erased_value(value)));
                    }}
  , contains_{[validator](const T& value)
              {
                using type = typename detail::validator_t<decltype(validator)>::underlying_type;
                static constexpr auto from_type_erased_value =
                  create_setting_value_converter<setting_type_erased_type_from<type>, type>();
                return validator->contains(from_type_erased_value(value));
              }}
  , plain_contains_{[validator](const plain_underlying_type& value)
                    {
                      using type = typename detail::validator_t<decltype(validator)>::plain_underlying_type;
                      static constexpr auto from_type_erased_value =
                        create_setting_value_converter<setting_type_erased_type_from<type>, type>();
                      return validator->contains(from_type_erased_value(value));
                    }}
  , connector{validator}
{
}

///
///
template<underlying_setting_type_erased_type T>
auto setting_validator_range_type_erased<T>::validate(const T& value) const -> T
{
  return validate_(value);
}

///
///
template<underlying_setting_type_erased_type T>
auto setting_validator_range_type_erased<T>::validate(const plain_underlying_type& value) const -> plain_underlying_type
  requires(std::is_same_v<underlying_type, plain_underlying_type>)
{
  return plain_validate_(value);
}

///
///
template<underlying_setting_type_erased_type T>
auto setting_validator_range_type_erased<T>::contains(const T& value) const -> bool
{
  return contains_(value);
}

///
///
template<underlying_setting_type_erased_type T>
auto setting_validator_range_type_erased<T>::contains(const plain_underlying_type& value) const -> bool
  requires(std::is_same_v<underlying_type, plain_underlying_type>)
{
  return plain_contains_(value);
}

///
///
template<underlying_setting_type T>
setting_validator_list<T>::setting_validator_list(std::vector<plain_underlying_type> list)
  : list_{[&]
          {
            std::sort(list.begin(), list.end());
            list.erase(std::unique(list.begin(), list.end()), list.end());
            return list;
          }()}
{
}

///
///
template<underlying_setting_type T>
template<std::ranges::range R>
setting_validator_list<T>::setting_validator_list(const R& range)
  requires(std::is_same_v<std::ranges::range_value_t<R>, plain_underlying_type>)
  : setting_validator_list<T>({std::ranges::begin(range), std::ranges::end(range)})
{
}

///
///
template<underlying_setting_type T>
auto setting_validator_list<T>::empty() const -> bool
{
  const auto lock = std::scoped_lock{mtx_};
  return list_.empty();
}

///
///
template<underlying_setting_type T>
auto setting_validator_list<T>::available() const -> std::vector<plain_underlying_type>
{
  const auto lock = std::scoped_lock{mtx_};
  return list_;
}

///
///
template<underlying_setting_type T>
template<std::ranges::range R>
auto setting_validator_list<T>::available(const R& range) -> bool
  requires(std::is_same_v<std::ranges::range_value_t<R>, plain_underlying_type>)
{
  auto new_list = [&range]
  {
    auto list = std::vector<plain_underlying_type>{std::ranges::begin(range), std::ranges::end(range)};
    std::sort(list.begin(), list.end());
    list.erase(std::unique(list.begin(), list.end()), list.end());
    return list;
  }();
  // A plain value needs one to hold, an optional or a vector may hold none
  const auto new_list_valid = !is_plain_type || !new_list.empty();
  if constexpr(!is_plain_type)
  {
    const auto lock = std::scoped_lock{mtx_};
    list_ = std::move(new_list);
  }
  else
  {
    if(new_list_valid)
    {
      const auto lock = std::scoped_lock{mtx_};
      list_ = std::move(new_list);
    }
  }
  notify_changed();
  return new_list_valid;
}

///
///
template<underlying_setting_type T>
auto setting_validator_list<T>::contains(const T& value) const -> bool
{
  const auto lock = std::scoped_lock{mtx_};
  if constexpr(is_vector_type)
  {
    return std::ranges::all_of(value, [this](const plain_underlying_type v) { return util::contains(list_, v); });
  }
  else if constexpr(is_optional_type)
  {
    if(!value.has_value())
    {
      return true;
    }
    return std::ranges::binary_search(list_, *value);
  }
  else
  {
    return std::ranges::binary_search(list_, value);
  }
}

///
///
template<underlying_setting_type T>
auto setting_validator_list<T>::contains(const plain_underlying_type& value) const -> bool
  requires(!is_plain_type)
{
  const auto lock = std::scoped_lock{mtx_};
  return std::ranges::binary_search(list_, value);
}

///
///
template<underlying_setting_type_erased_type T>
setting_validator_list_type_erased<T>::setting_validator_list_type_erased(const auto& validator)
  requires(detail::type_erased_validator_param_requirement<T, setting_validator_list, decltype(validator)>)
  : empty_{[validator]() { return validator->empty(); }}
  , available_{[validator]()
               {
                 using type = typename detail::validator_t<decltype(validator)>::plain_underlying_type;
                 static constexpr auto to_type_erased_value =
                   create_setting_value_converter<type, setting_type_erased_type_from<type>>();

                 const auto available_list = validator->available();
                 auto converted_list = std::vector<plain_underlying_type>{};
                 converted_list.reserve(available_list.size());
                 for(const auto& item : available_list)
                 {
                   auto converted_item = to_type_erased_value(item);
                   converted_list.emplace_back(std::move(converted_item));
                 }
                 return converted_list;
               }}
  , contains_{[validator](const T& value)
              {
                using type = typename detail::validator_t<decltype(validator)>::underlying_type;
                static constexpr auto from_type_erased_value =
                  create_setting_value_converter<setting_type_erased_type_from<type>, type>();
                return validator->contains(from_type_erased_value(value));
              }}
  , plain_contains_{[validator](const plain_underlying_type& value)
                    {
                      using type = typename detail::validator_t<decltype(validator)>::plain_underlying_type;
                      static constexpr auto from_type_erased_value =
                        create_setting_value_converter<setting_type_erased_type_from<type>, type>();
                      return validator->contains(from_type_erased_value(value));
                    }}
  , connector{validator}
{
}

///
///
template<underlying_setting_type_erased_type T>
auto setting_validator_list_type_erased<T>::empty() const -> bool
{
  return empty_();
}

///
///
template<underlying_setting_type_erased_type T>
auto setting_validator_list_type_erased<T>::available() const -> std::vector<plain_underlying_type>
{
  return available_();
}

///
///
template<underlying_setting_type_erased_type T>
auto setting_validator_list_type_erased<T>::contains(const T& value) const -> bool
{
  return contains_(value);
}

///
///
template<underlying_setting_type_erased_type T>
auto setting_validator_list_type_erased<T>::contains(const plain_underlying_type& value) const -> bool
  requires(std::is_same_v<underlying_type, plain_underlying_type>)
{
  return plain_contains_(value);
}

} // namespace bibstd::framework
