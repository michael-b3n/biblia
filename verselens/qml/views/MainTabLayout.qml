import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import BibQml

///
/// Content of the main window. All functionality is reachable through the tabs, the buttons
/// beside them belong to the window itself and are only reported to its owner.
///
Item
{
  id: root

  // Properties
  required property SettingsListModel listModelSettings
  required property ScriptureListModel listModelScripture
  required property BridgeBibleRefOcr bridgeBibleRefOcr
  required property BridgeBibleRefLookup bridgeBibleRefLookup
  required property BridgeApplication bridgeApplication
  required property BridgeScripture bridgeScripture
  required property bool pinned
  required property bool movable

  // Whether the bell rings. Starts with an update found before this layout, a click on the tab clears it.
  property bool notificationsUnread: root.bridgeApplication.updateAvailable

  // Constants
  // Index of the tab a found reference is shown in
  readonly property int scriptureTabIndex: 0

  implicitWidth: 640
  implicitHeight: 480

  // Signals
  signal closeClicked()
  signal pinClicked()
  signal released()
  signal moveRequested(deltaX: int, deltaY: int)

  // Connections
  ///
  /// Switches to the scripture tab, a reference found while another tab is open would stay
  /// hidden otherwise.
  ///
  Connections
  {
    target: root.bridgeBibleRefOcr

    function onReferenceFound(bookId, chapter, verse)
    {
      bar.setCurrentIndex(root.scriptureTabIndex)
    }
  }

  ///
  /// Rings the bell for every new notification.
  ///
  Connections
  {
    target: root.bridgeApplication

    function onUpdateAvailableChanged()
    {
      if(root.bridgeApplication.updateAvailable) { root.notificationsUnread = true }
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
    /// Header: the tabs and the buttons of the window.
    ///
    RowLayout
    {
      // Properties
      Layout.fillWidth: true
      Layout.fillHeight: false
      Layout.preferredHeight: Metrics.controlHeight
      spacing: Metrics.spacingSmall

      // Components
      TabBar
      {
        id: bar

        // Properties
        Layout.fillHeight: true
        Layout.fillWidth: false
        // Only as wide as the tabs it lays out, so that everything it does not need is left to
        // the move area beside it
        Layout.preferredWidth: bar.implicitWidth
        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
        spacing: Metrics.spacingSmall

        // Style
        background: Rectangle { color: "transparent" }

        // Components
        TabScriptureButton
        {
          // Properties
          searchRunning: root.bridgeBibleRefOcr.manualSearchRunning
        }

        TabSettingsButton {}

        TabNotificationsButton
        {
          // Properties
          unread: root.notificationsUnread

          // Connections
          onClicked: { root.notificationsUnread = false }
        }
      }

      ///
      /// Free space of the header beside the tabs, the one place the window is moved by.
      /// Dragging anywhere else would take it along while the user works in it.
      ///
      MoveArea
      {
        // Properties
        Layout.fillHeight: true
        Layout.fillWidth: true
        movable: root.movable

        // Connections
        onReleased: { root.released() }
        onMoveRequested: (deltaX, deltaY) => { root.moveRequested(deltaX, deltaY) }
      }

      ///
      /// Starts and stops the automatic reference search. The button follows the search
      /// instead of the click: the search reports when it really started and stopped.
      ///
      ButtonIconSwitch
      {
        // Properties
        Layout.fillHeight: true
        Layout.fillWidth: false
        Layout.preferredWidth: Metrics.controlHeight
        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
        svgSourceFirst: Icons.play
        svgSourceSecond: Icons.stop
        toggled: root.bridgeBibleRefOcr.autoSearchRunning

        // Connections
        onClicked: { root.bridgeBibleRefOcr.setAutoSearch(!root.bridgeBibleRefOcr.autoSearchRunning) }
      }

      ///
      /// Pins the window at its current position, or releases it back to the cursor.
      ///
      ButtonIconSwitch
      {
        // Properties
        Layout.fillHeight: true
        Layout.fillWidth: false
        Layout.preferredWidth: Metrics.controlHeight
        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
        svgSourceFirst: Icons.pin
        svgSourceSecond: Icons.pinFilled
        toggled: root.pinned

        // Connections
        onClicked: { root.pinClicked() }
      }

      ///
      /// Takes the window off the screen.
      ///
      ButtonIconSimple
      {
        // Properties
        Layout.fillHeight: true
        Layout.fillWidth: false
        Layout.preferredWidth: Metrics.controlHeight
        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
        svgSource: Icons.close

        // Connections
        onClicked: { root.closeClicked() }
      }
    }

    ///
    /// Content of the tab the user selected.
    ///
    StackLayout
    {
      // Properties
      Layout.fillWidth: true
      Layout.fillHeight: true
      currentIndex: bar.currentIndex

      // Components
      TabScriptureContent
      {
        // Properties
        listModelScripture: root.listModelScripture
        bridgeBibleRefLookup: root.bridgeBibleRefLookup
        bridgeScripture: root.bridgeScripture
      }

      TabSettingsContent
      {
        // Properties
        listModelSettings: root.listModelSettings
      }

      TabNotificationsContent
      {
        // Properties
        bridgeApplication: root.bridgeApplication
      }
    }
  }
}
