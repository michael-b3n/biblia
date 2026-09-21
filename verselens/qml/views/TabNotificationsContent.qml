import QtQuick
import QtQuick.Layouts
import BibQml

///
/// Content of the notifications tab: the state of the updates and, if the application has an
/// updater, the buttons to check for an update and to install it. Both are listed in the titled
/// box the settings use as well, so that a tab of the window reads like the next one.
///
Item
{
  id: root

  // Properties
  required property BridgeApplication bridgeApplication

  // Store installs have no updater, the store delivers their updates
  readonly property bool hasUpdater: root.bridgeApplication.updateCheck !== BridgeApplication.UpdateCheckUnavailable
  readonly property bool checkRunning: root.bridgeApplication.updateCheck === BridgeApplication.UpdateCheckRunning

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
    /// State the updates are in, which is what there is to report.
    ///
    GroupBoxSimple
    {
      // Properties
      Layout.fillWidth: true
      // Note the language is passed to reevaluate this binding on a language change.
      title: Translations.name("notifications", Translations.language)

      // Components
      contentItem: TextSimple
      {
        // Properties
        text: Translations.name(root.statusKey(), Translations.language)
        wrapMode: Text.WordWrap
      }
    }

    ///
    /// What can be asked of the updater.
    ///
    GroupBoxSimple
    {
      // Properties
      Layout.fillWidth: true
      visible: root.hasUpdater
      title: Translations.name("updates", Translations.language)

      // Components
      contentItem: RowLayout
      {
        // Properties
        spacing: Metrics.spacingSmall

        // Components
        ///
        /// Installs the downloaded update, which restarts the application.
        ///
        ButtonTextSimple
        {
          // Properties
          visible: root.bridgeApplication.updateAvailable
          text: Translations.name("update_now", Translations.language)

          // Connections
          onClicked: { root.bridgeApplication.requestUpdate() }
        }

        ButtonTextSimple
        {
          // Properties
          enabled: !root.checkRunning
          text: Translations.name("update_check", Translations.language)

          // Connections
          onClicked: { root.bridgeApplication.requestUpdateCheck() }
        }

        Item { Layout.fillWidth: true }
      }
    }

    Item
    {
      // Properties
      Layout.fillWidth: true
      Layout.fillHeight: true
    }
  }

  // Functions
  ///
  /// Key of the text telling the state of the updates, a downloaded update comes first.
  ///
  function statusKey(): string
  {
    if(root.bridgeApplication.updateAvailable) { return "update_available" }
    switch(root.bridgeApplication.updateCheck)
    {
    case BridgeApplication.UpdateCheckRunning: return "update_check_running"
    case BridgeApplication.UpdateCheckFinished: return "update_check_finished"
    case BridgeApplication.UpdateCheckFailed: return "update_check_failed"
    default: return "notifications_none"
    }
  }
}
