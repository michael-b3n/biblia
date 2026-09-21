#include "bibqml/model/SettingsListModel.hpp"
#include "bibqml/util/SettingConversion.hpp"

#include <bibstd/framework/setting.hpp>
#include <bibstd/framework/setting_validator.hpp>
#include <bibstd/math/arithmetic.hpp>
#include <bibstd/util/contains.hpp>
#include <bibstd/util/exception.hpp>
#include <bibstd/util/log.hpp>
#include <bibstd/util/visit_helper.hpp>

#include <algorithm>
#include <chrono>
#include <optional>
#include <ranges>
#include <string_view>
#include <type_traits>

namespace bibqml
{
namespace
{

// Typedefs
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
/// Check if the setting of the specified path is an internal one. Internal settings hold state
/// the application manages on its own, e.g. the position of a window. They are persisted like
/// any other setting, but they are not meant to be edited by the user.
/// \return true if the setting is internal, false otherwise
///
auto isInternalSetting(const std::string_view path) -> bool
{
  static constexpr auto internalPrefix = std::string_view{"internal."};
  return path.starts_with(internalPrefix);
}

///
/// Read the segments a setting path is made of. Splitting is left to the backend, which is
/// where how a path is written is known.
/// \return segments of the path as the views read them
///
auto toCategories(const std::string& path) -> QStringList
{
  const auto segments = bibstd::workflow::workflow_settings::split_path(path);
  auto categories = QStringList{};
  categories.reserve(static_cast<qsizetype>(segments.size()));
  std::ranges::transform(
    segments, std::back_inserter(categories), [](const auto& segment) { return QString::fromStdString(segment); }
  );
  return categories;
}

///
/// Deduce the value type enum value from the settings pointer.
/// \return enum value describing the value type
///
constexpr auto getValueType(const auto& setting) -> SettingsListModel::ValueType
{
  using value_type = setting_raw_value_type<std::remove_cvref_t<decltype(setting)>>;
  // clang-format off
  if constexpr(std::is_same_v<value_type, bool>) { return SettingsListModel::ValueType::BoolValueType; }
  else if constexpr(std::is_same_v<value_type, std::int32_t>) { return SettingsListModel::ValueType::IntValueType; }
  else if constexpr(std::is_same_v<value_type, std::int64_t>) { return SettingsListModel::ValueType::IntValueType; }
  else if constexpr(std::is_same_v<value_type, std::uint32_t>) { return SettingsListModel::ValueType::IntValueType; }
  else if constexpr(std::is_same_v<value_type, std::uint64_t>) { return SettingsListModel::ValueType::IntValueType; }
  else if constexpr(std::is_same_v<value_type, double>) { return SettingsListModel::ValueType::DoubleValueType; }
  else if constexpr(std::is_same_v<value_type, std::string>) { return SettingsListModel::ValueType::StringValueType; }
  else if constexpr(std::is_same_v<value_type, std::chrono::milliseconds>) { return SettingsListModel::ValueType::TimeValueType; }
  else if constexpr(std::is_same_v<value_type, std::chrono::seconds>) { return SettingsListModel::ValueType::TimeValueType; }
  else if constexpr(std::is_same_v<value_type, std::chrono::minutes>) { return SettingsListModel::ValueType::TimeValueType; }
  else if constexpr(std::is_same_v<value_type, std::filesystem::path>) { return SettingsListModel::ValueType::PathValueType; }
  else { static_assert(always_false_v<value_type>, "Unsupported setting value type"); }
  // clang-format on
}

///
/// Deduce the wrapper type enum value from the settings pointer.
/// \return enum value describing the wrapper type
///
constexpr auto getWrapperType(const auto& setting) -> SettingsListModel::WrapperType
{
  using value_type = std::remove_pointer_t<std::remove_cvref_t<decltype(setting)>>::value_type;
  using raw_value_type = setting_raw_value_type<std::remove_cvref_t<decltype(setting)>>;
  static_assert(std::is_same_v<std::remove_cvref_t<value_type>, value_type>);
  static_assert(std::is_same_v<std::remove_cvref_t<raw_value_type>, raw_value_type>);

  // clang-format off
  if constexpr(std::is_same_v<value_type, raw_value_type>) { return SettingsListModel::WrapperType::NoneWrapperType; }
  else if constexpr(std::is_same_v<value_type, std::optional<raw_value_type>>) { return SettingsListModel::WrapperType::OptionalWrapperType; }
  else if constexpr(std::is_same_v<value_type, std::vector<raw_value_type>>) { return SettingsListModel::WrapperType::ListWrapperType; }
  else { static_assert(always_false_v<value_type>, "Unsupported setting wrapper type"); }
  // clang-format on
}

///
/// Deduce the validator type enum value from the settings validator variant.
/// \return enum value describing the validator type
///
auto getValidatorType(const auto& setting) -> SettingsListModel::ValidatorType
{
  using value_type = std::remove_pointer_t<std::remove_cvref_t<decltype(setting)>>::value_type;
  return bibstd::util::visit_lambdas(
    setting->validator,
    []([[maybe_unused]] const validator_unbound_sptr&) { return SettingsListModel::ValidatorType::UnboundValidatorType; },
    []([[maybe_unused]] const validator_range_sptr<value_type>&)
    { return SettingsListModel::ValidatorType::RangeValidatorType; },
    []([[maybe_unused]] const validator_list_sptr<value_type>&) { return SettingsListModel::ValidatorType::ListValidatorType; }
  );
}

} // namespace

///
///
SettingsListModel::SettingsListModel(
  std::shared_ptr<bibstd::workflow::workflow_settings> workflowSettings, const bibstd::util::non_owning_ptr<QObject> parent
)
  : QAbstractListModel{parent}
  , workflowSettings_{std::move(workflowSettings)}
{
  const auto settings = workflowSettings_->type_erased_settings();
  std::ranges::for_each(
    settings | std::views::filter([](const auto& settingData) { return !isInternalSetting(settingData.path); }),
    [&](const auto& settingData)
    { std::visit([&](const auto setting) { addSetting(settingData.path, setting); }, settingData.setting); }
  );

  // Settings may also be created after this model was constructed, e.g. by the QML layer.
  workflowSettings_->connect_queued(
    &bibstd::workflow::workflow_settings_signals::setting_created,
    [this](const std::string& path)
    {
      LOG_DEBUG("notify setting created: path=\"{}\"", path)
      QMetaObject::invokeMethod(this, [this, path]() { appendSetting(path); }, Qt::QueuedConnection);
    },
    executor_
  );
}

///
///
SettingsListModel::~SettingsListModel() noexcept = default;

///
///
int SettingsListModel::rowCount(const QModelIndex& parent) const
{
  if(parent.isValid())
  {
    return 0;
  }
  return static_cast<int>(entries_.size());
}

///
///
QVariant SettingsListModel::data(const QModelIndex& index, const int role) const
{
  if(!index.isValid() || index.row() < 0 || static_cast<decltype(entries_.size())>(index.row()) >= entries_.size())
  {
    return {};
  }
  [[maybe_unused]] const auto& entry = entries_.at(static_cast<std::size_t>(index.row()));
  switch(role)
  {
  case PathRole: return QString::fromStdString(entry.path);
  case CategoriesRole: return entry.categories;
  case ValueTypeRole: return static_cast<int>(entry.valueType);
  case WrapperTypeRole: return static_cast<int>(entry.wrapperType);
  case ValidatorTypeRole: return static_cast<int>(entry.validatorType);
  case ValueRole: return toQmlValue(entry.setting);
  case ListValidatorDataRole: return toQmlValidatorValues(entry.setting);
  case PostfixRole: return entry.postfix;
  default: return {};
  }
}

///
///
QHash<int, QByteArray> SettingsListModel::roleNames() const
{
  return {
    {             PathRole,              "path"},
    {       CategoriesRole,        "categories"},
    {        ValueTypeRole,         "valueType"},
    {      WrapperTypeRole,       "wrapperType"},
    {    ValidatorTypeRole,     "validatorType"},
    {            ValueRole,             "value"},
    {ListValidatorDataRole, "listValidatorData"},
    {          PostfixRole,           "postfix"},
  };
}

///
///
bool SettingsListModel::setData(const QModelIndex& index, const QVariant& value, const int role)
{
  if(role != ValueRole)
  {
    return false; // all other roles are immutable and cannot be set
  }
  if(!index.isValid() || index.row() < 0 || static_cast<decltype(entries_.size())>(index.row()) >= entries_.size())
  {
    return false;
  }
  try
  {
    const auto& entry = entries_.at(static_cast<std::size_t>(index.row()));
    const auto isValueSet = setQmlValue(entry.setting, value);
    if(!isValueSet)
    {
      // Emit dataChanged signal to notify that the value
      // could not be set such that the UI can be reset.
      // The setting itself will also emit a dataChanged signal
      // if the value of the setting changed successfully.
      // If settings fails but the value still changed,
      // the signal is emitted twice.
      emit dataChanged(index, index, {role});
    }
    LOG_DEBUG("write setting: path=\"{}\", row={}", entry.path, index.row());
    return isValueSet;
  }
  catch(...)
  {
    LOG_ERROR("exception writing setting value: {}", bibstd::util::exception_report());
    emit dataChanged(index, index, {role});
    return false;
  }
}

///
///
Qt::ItemFlags SettingsListModel::flags([[maybe_unused]] const QModelIndex& /*index*/) const
{
  return Qt::NoItemFlags;
}

///
///
void SettingsListModel::disconnect()
{
  executor_.disconnect();
}

///
///
void SettingsListModel::appendSetting(const std::string& path)
{
  if(isInternalSetting(path) || bibstd::util::contains(entries_, [&path](const auto& entry) { return entry.path == path; }))
  {
    return;
  }
  const auto settingData = workflowSettings_->type_erased_setting(path);
  if(!settingData)
  {
    LOG_ERROR("append setting failed: setting not found: path=\"{}\"", path);
    return;
  }
  if(entries_.size() >= static_cast<std::size_t>(std::numeric_limits<int>::max()))
  {
    LOG_ERROR("max entries count exceeded: path=\"{}\" not added", path);
    return;
  }
  const auto row = static_cast<int>(entries_.size());
  beginInsertRows(QModelIndex{}, row, row);
  std::visit([&](const auto setting) { addSetting(path, setting); }, *settingData);
  endInsertRows();
}

///
///
void SettingsListModel::addSetting(const std::string& path, const auto& setting)
{
  addEntry(path, setting);
  const auto rowIndex = bibstd::math::arithmetic::subtract(entries_.size(), decltype(entries_.size()){1}).value();
  setting->signal_adapter.connect_queued(
    &bibstd::framework::setting_signals::value_changed,
    [this, path, rowIndex]()
    {
      LOG_DEBUG("notify setting value changed: path=\"{}\", row={}", path, rowIndex)
      QMetaObject::invokeMethod(
        this,
        [this, rowIndex]() { emit dataChanged(index(rowIndex), index(rowIndex), {Role::ValueRole}); },
        Qt::QueuedConnection
      );
    },
    executor_
  );
  setting->signal_adapter.connect_queued(
    &bibstd::framework::setting_signals::validator_changed,
    [this, setting, path, rowIndex]()
    {
      using value_type = std::remove_pointer_t<std::remove_cvref_t<decltype(setting)>>::value_type;
      bibstd::util::visit_lambdas(
        setting->validator,
        []([[maybe_unused]] const validator_unbound_sptr&) { /*noop*/ },
        []([[maybe_unused]] const validator_range_sptr<value_type>&) { /*noop*/ },
        [this, path, rowIndex]([[maybe_unused]] const validator_list_sptr<value_type>&)
        {
          LOG_DEBUG("notify setting validator changed: path=\"{}\", row={}", path, rowIndex)
          QMetaObject::invokeMethod(
            this,
            [this, rowIndex]() { emit dataChanged(index(rowIndex), index(rowIndex), {Role::ListValidatorDataRole}); },
            Qt::QueuedConnection
          );
        }
      );
    },
    executor_
  );
}

///
///
void SettingsListModel::addEntry(std::string path, const auto& setting)
{
  if(entries_.size() >= static_cast<std::size_t>(std::numeric_limits<int>::max()))
  {
    LOG_ERROR("max entries count exceeded: path=\"{}\" not added", path);
    return;
  }
  auto categories = toCategories(path);
  entries_.emplace_back(
    Entry{
      .path = std::move(path),
      .categories = std::move(categories),
      .valueType = getValueType(setting),
      .wrapperType = getWrapperType(setting),
      .validatorType = getValidatorType(setting),
      .postfix = toQmlPostfix(setting),
      .setting = setting
    }
  );
}

} // namespace bibqml
