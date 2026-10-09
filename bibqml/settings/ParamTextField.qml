pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import BibQml

///
/// Input for string or double value types.
/// All input types must be convertible to string.
/// For boolean value types the ParamSwitch object shall be used.
///
ParamBase
{
  id: root

  // Components
  contentItem: TextField
  {
    id: input

    // Properties
    // Note optional params may hold no value at all.
    text: root.value === undefined ? "" : root.value.toString()
    font.pointSize: Metrics.fontSizeParam
    color: Colors.text
    selectionColor: Colors.borderDarker
    selectedTextColor: Colors.backgroundSolid
    renderType: Text.CurveRendering
    verticalAlignment: Text.AlignVCenter

    padding: Metrics.paddingParamContent
    topPadding: Metrics.spacingSmall
    bottomPadding: Metrics.spacingSmall
    // The edited text is kept clear of the postfix and the path button standing at the right edge
    rightPadding: postfixText.width + pathButton.width > 0
                  ? postfixText.width + pathButton.width + 2 * Metrics.paddingParamContent
                  : Metrics.paddingParamContent
    width: root.availableWidth
    implicitHeight: root.lineHeight + input.topPadding + input.bottomPadding

    inputMethodHints:
    {
      switch(root.valueType)
      {
      case SettingsListModel.IntValueType:
      case SettingsListModel.TimeValueType: return Qt.ImhDigitsOnly
      case SettingsListModel.DoubleValueType: return Qt.ImhFormattedNumbersOnly
      case SettingsListModel.StringValueType:
      case SettingsListModel.PathValueType:
      case SettingsListModel.BoolValueType:
      default: return Qt.ImhNone
      }
    }

    // Connections
    onTextEdited: { debounceTimer.restart() }

    Connections
    {
      target: root
      function onValueChanged()
      {
        if(root.value !== undefined)
        {
          input.text = root.value.toString()
        }
        else
        {
          input.text = ""
        }
      }
    }

    // Components
    ParamPostfix
    {
      id: postfixText

      // Properties
      anchors.right: parent.right
      anchors.rightMargin: Metrics.paddingParamContent
      anchors.verticalCenter: parent.verticalCenter
      text: root.postfix
    }

    Loader
    {
      id: pathButton

      // Properties
      anchors.right: postfixText.left
      anchors.verticalCenter: parent.verticalCenter
      active: root.valueType === SettingsListModel.PathValueType

      // Components
      sourceComponent: ButtonIconSimple
      {
        // Properties
        svgSource: Icons.folderOpen
        iconSize: Metrics.iconSize

        // Connections
        onClicked:
        {
          folderDialog.currentFolder = LocalPath.toUrl(input.text)
          folderDialog.open()
        }

        // Components
        HoverHandler
        {
          // Properties
          cursorShape: Qt.ArrowCursor
        }

        FolderDialog
        {
          id: folderDialog

          // Properties
          title: root.title

          // Connections
          onAccepted: { root.paramValueChanged(LocalPath.fromUrl(folderDialog.selectedFolder)) }
        }
      }
    }

    Timer
    {
      id: debounceTimer

      // Properties
      interval: Metrics.durationDebounce
      repeat: false

      // Connections
      onTriggered: { input.handleTextChange() }
    }

    // Style
    background: BackgroundSimple
    {
      color: input.activeFocus ? Colors.selection : Colors.backgroundSolidDarker
    }

    // Functions
    function handleTextChange()
    {
      switch(root.valueType)
      {
      case SettingsListModel.BoolValueType:
        BridgeLogger.error("unsupported SettingsListModel::BoolValueType used in ParamTextField")
        return
      case SettingsListModel.IntValueType: // [[fallthrough]]
      case SettingsListModel.TimeValueType:
      {
        const v = parseInt(input.text)
        if(isNaN(v))
        {
          input.text = root.value ? root.value.toString() : ""
        }
        else
        {
          root.paramValueChanged(v)
        }
        return
      }
      case SettingsListModel.DoubleValueType:
      {
        const v = parseFloat(input.text)
        if(isNaN(v))
        {
          input.text = root.value ? root.value.toString() : ""
        }
        else
        {
          root.paramValueChanged(v)
        }
        return
      }
      case SettingsListModel.StringValueType: // fallthrough
      case SettingsListModel.PathValueType:
        root.paramValueChanged(input.text)
        return
      default:
        BridgeLogger.error("unsupported SettingsListModel::ValueType value")
        return
      }
    }
  }
}
