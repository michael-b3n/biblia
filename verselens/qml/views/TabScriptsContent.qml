import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import BibQml

///
/// Content of the scripts tab: the loaded scripts with their description and
/// their functions, and the buttons to import scripts and to load them anew.
///
Item
{
  id: root

  // Properties
  required property BridgeScript bridgeScript

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
  ColumnLayout
  {
    // Properties
    anchors.fill: parent
    anchors.margins: Metrics.spacingSmall
    spacing: Metrics.spacingSmall

    // Components
    ///
    /// The loaded scripts, each with its description and the functions it offers.
    ///
    GroupBoxSimple
    {
      // Properties
      Layout.fillWidth: true
      Layout.fillHeight: true
      // Note the language is passed to reevaluate this binding on a language change.
      title: Translations.name("scripts_loaded", Translations.language)

      // Components
      contentItem: Item
      {
        ListView
        {
          id: scriptList

          // Properties
          anchors.fill: parent
          clip: true
          spacing: Metrics.spacingMedium
          model: root.bridgeScript.scripts

          // Components
          ScrollBar.vertical: ScrollBarSimple {}

          delegate: ColumnLayout
          {
            id: scriptRow

            // Properties
            required property var modelData

            width: scriptList.width
            spacing: Metrics.spacingTiny

            // Components
            TextSimple
            {
              // Properties
              Layout.fillWidth: true
              text: scriptRow.modelData.name
              font.bold: true
              elide: Text.ElideRight
            }

            TextSimple
            {
              // Properties
              Layout.fillWidth: true
              text: scriptRow.modelData.description
              wrapMode: Text.WordWrap
              visible: text !== ""
            }

            TextSimple
            {
              // Properties
              Layout.fillWidth: true
              text: scriptRow.modelData.functions.join(", ")
              font.pointSize: Metrics.fontSizeSmall
              color: Colors.borderDarker
              wrapMode: Text.WordWrap
            }
          }
        }
      }
    }

    ///
    /// Copies script files into the script folder and loads them.
    ///
    ButtonIconTextSimple
    {
      // Properties
      Layout.fillWidth: true
      svgSource: Icons.fileImport
      text: Translations.name("scripts_import", Translations.language)
      enabled: !root.bridgeScript.loading

      // Connections
      onClicked: { importDialog.open() }
    }

    ///
    /// Takes changes of the scripts and of their settings without a restart.
    ///
    ButtonIconTextSimple
    {
      // Properties
      Layout.fillWidth: true
      svgSource: Icons.folderOpen
      text: Translations.name("scripts_load", Translations.language)
      enabled: !root.bridgeScript.loading

      // Connections
      onClicked: { root.bridgeScript.loadScripts() }
    }
  }

  FileDialog
  {
    id: importDialog

    // Properties
    title: Translations.name("scripts_import", Translations.language)
    fileMode: FileDialog.OpenFiles
    nameFilters: [Translations.name("scripts_import_filter", Translations.language) + " (*.lua)"]

    // Connections
    onAccepted: { root.bridgeScript.importFiles(importDialog.selectedFiles) }
  }
}
