pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import BibQml

///
/// Editor of the settings of a SettingsListModel, grouped by their first segment, both in alphabetical order.
/// Groups can be folded, all start unfolded.
///
Item
{
  id: root

  // Properties
  required property SettingsListModel listModelSettings

  // Categories the user folded away, by their key. Remembered per category and not per row, so
  // that a setting created or reordered while a category is folded stays hidden with it.
  property var collapsedCategories: ({})

  // Components
  ///
  /// Order the settings are listed in. The backend has no order to offer, so they are sorted by
  /// the names they are displayed under.
  ///
  SettingsSortModel
  {
    id: sortModel

    // Properties
    sourceModel: root.listModelSettings
  }

  ListView
  {
    id: listView

    // Properties
    anchors.fill: parent
    anchors.margins: Metrics.spacingSmall
    clip: true
    // The gap between two settings belongs to the setting above it, so that it is folded away
    // together with it. Spacing of the view itself would stay behind as an empty strip.
    spacing: 0
    model: sortModel

    section.property: "category"
    section.criteria: ViewSection.FullString

    // Components
    ///
    /// Header the settings of one category are listed under.
    ///
    section.delegate: SettingsCategoryHeader
    {
      // The view names the section it creates this header for
      required property string section

      category: section
      collapsed: root.isCollapsed(section)
      width: listView.width - scrollBar.width

      // Connections
      onToggled: { root.toggleCategory(section) }
    }

    delegate: Item
    {
      id: delegateRoot

      // Properties
      required property int index
      required property var categories
      required property string category
      required property int valueType
      required property int wrapperType
      required property int validatorType
      required property var value
      required property var listValidatorData
      required property string postfix

      readonly property bool collapsed: root.isCollapsed(delegateRoot.category)
      property real unfolded: delegateRoot.collapsed ? 0 : 1

      width: listView.width - scrollBar.width
      // A folded setting is kept with no height left instead of being filtered out of the model:
      // the section header belongs to the first setting of its section and would go with it.
      // The gap below a setting belongs to it, so that it is folded away together with it.
      height: delegateRoot.unfolded * (paramLoader.implicitHeight + Metrics.spacingSmall)
      visible: delegateRoot.height > 0
      clip: true

      // Animations
      Behavior on unfolded
      {
        NumberAnimation
        {
          duration: Metrics.durationShort
          easing.type: Easing.InOutQuad
        }
      }

      // Components
      Loader
      {
        id: paramLoader

        // Properties
        x: Metrics.spacingSmall
        width: delegateRoot.width - 2 * Metrics.spacingSmall

        // Connections
        // Bindings, so the param follows the model
        Component.onCompleted:
        {
          paramLoader.setSource(delegateRoot.paramFile(), {
            categories: Qt.binding(() => delegateRoot.categories),
            valueType: Qt.binding(() => delegateRoot.valueType),
            wrapperType: Qt.binding(() => delegateRoot.wrapperType),
            validatorType: Qt.binding(() => delegateRoot.validatorType),
            value: Qt.binding(() => delegateRoot.value),
            listValidatorData: Qt.binding(() => delegateRoot.listValidatorData),
            postfix: Qt.binding(() => delegateRoot.postfix)
          })
        }
      }

      Connections
      {
        target: paramLoader.item
        function onParamValueChanged(value: var) { delegateRoot.writeBack(value) }
      }

      // Functions
      ///
      /// \return the file of the param editing the setting
      ///
      function paramFile(): string
      {
        switch(delegateRoot.valueType)
        {
        case SettingsListModel.BoolValueType:
          switch(delegateRoot.wrapperType)
          {
          case SettingsListModel.NoneWrapperType: // [[fallthrough]]
          case SettingsListModel.OptionalWrapperType: return "ParamSwitch.qml"
          default: return "ParamError.qml"
          }
        case SettingsListModel.IntValueType: // [[fallthrough]]
        case SettingsListModel.DoubleValueType: // [[fallthrough]]
        case SettingsListModel.TimeValueType: // [[fallthrough]]
        case SettingsListModel.StringValueType: // [[fallthrough]]
        case SettingsListModel.PathValueType:
          switch(delegateRoot.wrapperType)
          {
          case SettingsListModel.NoneWrapperType: // [[fallthrough]]
          case SettingsListModel.OptionalWrapperType:
            switch(delegateRoot.validatorType)
            {
            case SettingsListModel.UnboundValidatorType: // [[fallthrough]]
            case SettingsListModel.RangeValidatorType: return "ParamTextField.qml"
            case SettingsListModel.ListValidatorType: return "ParamComboBox.qml"
            default: return "ParamError.qml"
            }
          case SettingsListModel.ListWrapperType: return "ParamListView.qml"
          default: return "ParamError.qml"
          }
        default: return "ParamError.qml"
        }
      }

      ///
      /// Reports an edited value back to the backend, which validates it and answers with the
      /// value it accepted, so that the control shows what was stored and not what was typed.
      ///
      function writeBack(value: var)
      {
        const modelIndex = listView.model.index(delegateRoot.index, 0)
        listView.model.setData(modelIndex, value, SettingsListModel.ValueRole)
      }
    }

    ScrollBar.vertical: ScrollBarSimple { id: scrollBar }
  }

  // Functions
  ///
  /// Tells whether the settings of a category are folded away.
  ///
  function isCollapsed(category: string): bool
  {
    return root.collapsedCategories[category] === true
  }

  ///
  /// Folds the settings of a category away, or reads them back onto the screen.
  ///
  function toggleCategory(category: string)
  {
    // The bindings on the folded state follow the property, not what is written into the object
    // it holds, so the change is reported by handing out a new object.
    const collapsed = Object.assign({}, root.collapsedCategories)
    collapsed[category] = !root.isCollapsed(category)
    root.collapsedCategories = collapsed
  }
}
