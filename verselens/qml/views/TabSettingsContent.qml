pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import BibQml

///
/// Content of the settings tab. A list view lists the settings of the listModelSettings through
/// the Param controls, grouped by the first of the segments they are named by. Only this one
/// level is grouped, all further segments name the setting itself.
/// Every group can be folded away, the tab opens with all of them unfolded.
/// Groups and the settings inside them are listed in alphabetical order.
///
Item
{
  id: root

  // Properties
  required property SettingsListModel listModelSettings

  // Categories the user folded away, by their key. Remembered per category and not per row, so
  // that a setting created or reordered while a category is folded stays hidden with it.
  property var collapsedCategories: ({})

  // The tab fades in when it is switched to, the layout takes the previous one off at once
  opacity: root.visible ? 1 : 0

  // Animations
  Behavior on opacity
  {
    NumberAnimation
    {
      duration: Metrics.durationShort
      easing.type: Easing.InOutQuad
    }
  }

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

      readonly property bool collapsed: root.isCollapsed(delegateRoot.category)

      width: listView.width - scrollBar.width
      // A folded setting is kept with no height left instead of being filtered out of the model:
      // the section header belongs to the first setting of its section and would go with it.
      // The gap below a setting belongs to it, so that it is folded away together with it.
      height: delegateRoot.collapsed ? 0 : paramLoader.implicitHeight + Metrics.spacingSmall
      visible: delegateRoot.height > 0
      clip: true

      // Animations
      Behavior on height
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
        sourceComponent:
        {
          switch(delegateRoot.valueType)
          {
          case SettingsListModel.BoolValueType:
          {
            switch(delegateRoot.wrapperType)
            {
            case SettingsListModel.NoneWrapperType: // [[fallthrough]]
            case SettingsListModel.OptionalWrapperType: return paramSwitch
            case SettingsListModel.ListWrapperType: return paramError // unsupported
            default: return paramError
            }
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
              case SettingsListModel.RangeValidatorType: return paramTextField
              case SettingsListModel.ListValidatorType: return paramComboBox
              default: return paramError
              }
            case SettingsListModel.ListWrapperType: return paramListView
            default: return paramError
            }
          default: return paramError
          }
        }

        // Components
        Component
        {
          id: paramSwitch

          // Components
          ParamSwitch
          {
            // Properties
            categories: delegateRoot.categories
            valueType: delegateRoot.valueType
            wrapperType: delegateRoot.wrapperType
            validatorType: delegateRoot.validatorType
            value: delegateRoot.value
            listValidatorData: delegateRoot.listValidatorData

            // Connections
            onParamValueChanged: (value) => { delegateRoot.writeBack(value) }
          }
        }

        Component
        {
          id: paramTextField

          // Components
          ParamTextField
          {
            // Properties
            categories: delegateRoot.categories
            valueType: delegateRoot.valueType
            wrapperType: delegateRoot.wrapperType
            validatorType: delegateRoot.validatorType
            value: delegateRoot.value
            listValidatorData: delegateRoot.listValidatorData

            // Connections
            onParamValueChanged: (value) => { delegateRoot.writeBack(value) }
          }
        }

        Component
        {
          id: paramComboBox

          // Components
          ParamComboBox
          {
            // Properties
            categories: delegateRoot.categories
            valueType: delegateRoot.valueType
            wrapperType: delegateRoot.wrapperType
            validatorType: delegateRoot.validatorType
            value: delegateRoot.value
            listValidatorData: delegateRoot.listValidatorData

            // Connections
            onParamValueChanged: (value) => { delegateRoot.writeBack(value) }
          }
        }

        Component
        {
          id: paramListView

          // Components
          ParamListView
          {
            // Properties
            categories: delegateRoot.categories
            valueType: delegateRoot.valueType
            wrapperType: delegateRoot.wrapperType
            validatorType: delegateRoot.validatorType
            value: delegateRoot.value
            listValidatorData: delegateRoot.listValidatorData

            // Connections
            onParamValueChanged: (value) => { delegateRoot.writeBack(value) }
          }
        }

        Component
        {
          id: paramError

          // Components
          ParamError
          {
            // Properties
            categories: delegateRoot.categories
            valueType: delegateRoot.valueType
            wrapperType: delegateRoot.wrapperType
            validatorType: delegateRoot.validatorType
            value: delegateRoot.value
            listValidatorData: delegateRoot.listValidatorData
          }
        }
      }

      // Functions
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
