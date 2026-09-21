import QtQuick
import BibQml

///
/// Content of the settings tab. It only fades the editor of the module in, like every other tab.
///
Item
{
  id: root

  // Properties
  required property SettingsListModel listModelSettings

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
  SettingsView
  {
    // Properties
    anchors.fill: parent
    listModelSettings: root.listModelSettings
  }
}
