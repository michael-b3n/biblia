#pragma once

#include "bibstd/framework/setting.hpp"

#include <functional>
#include <memory>

namespace bibstd::framework
{

///
/// Type erased setting class. This class wraps the general setting interface.
/// `This` must be destroyed before the underlying setting is destroyed.
///
template<underlying_setting_type_erased_type T>
class setting_type_erased final
{
  // Variables
  const std::function<T()> get_;
  const std::function<bool(const T&)> set_;

public: // Typedefs
  using value_type = T;

public: // Structors
  template<underlying_setting_type U>
  setting_type_erased(const std::shared_ptr<setting<U>>& setting);

public: // Accessors
  ///
  /// Access setting value.
  /// \return reference to setting value
  ///
  auto value() const -> value_type;

public: // Setters
  ///
  /// Set setting value.
  ///
  auto value(const value_type& v) -> bool;

public: // Variables
  const signal::adapter<setting_signals>& signal_adapter;
  const std::string path;
  const setting_validator_type_erased<value_type> validator;
  const std::string postfix;
};

///
/// Deduction guide for setting_type_erased.
///
template<underlying_setting_type U>
setting_type_erased(const std::shared_ptr<setting<U>>& setting) -> setting_type_erased<setting_type_erased_type_from<U>>;

namespace detail
{

///
/// Convert setting validator to type erased setting validator.
/// \return type erased setting validator
///
template<underlying_setting_type U>
auto validator_type_erased(const setting_validator<U>& validator)
  -> setting_validator_type_erased<setting_type_erased_type_from<U>>
{
  using erased_type = setting_type_erased_type_from<U>;
  using return_type = setting_validator_type_erased<erased_type>;
  return util::visit_lambdas(
    validator,
    [&](const setting_validator_unbound::sptr_type& validator_unbound) -> return_type { return validator_unbound; },
    [&](const setting_validator_range<U>::sptr_type& validator_range) -> return_type
    { return std::make_shared<setting_validator_range_type_erased<erased_type>>(validator_range); },
    [&](const setting_validator_list<U>::sptr_type& validator_list) -> return_type
    { return std::make_shared<setting_validator_list_type_erased<erased_type>>(validator_list); }
  );
}

} // namespace detail

///
///
template<underlying_setting_type_erased_type T>
template<underlying_setting_type U>
setting_type_erased<T>::setting_type_erased(const std::shared_ptr<setting<U>>& setting)
  : get_{[setting, converter = create_setting_value_converter<U, value_type>()]() { return converter(setting->value()); }}
  , set_{[setting, converter = create_setting_value_converter<value_type, U>()](const value_type& v)
         { return setting->value(converter(v)); }}
  , signal_adapter{*setting}
  , path{setting->path}
  , validator{detail::validator_type_erased<U>(setting->validator)}
  , postfix{setting->postfix}
{
}

///
///
template<underlying_setting_type_erased_type T>
auto setting_type_erased<T>::value() const -> value_type
{
  return get_();
}

///
///
template<underlying_setting_type_erased_type T>
auto setting_type_erased<T>::value(const value_type& v) -> bool
{
  return set_(v);
}

} // namespace bibstd::framework
