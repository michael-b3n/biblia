import QtQuick
import BibQml

///
/// Window carrying the content. It has no decoration of its own, it is resized by the corners
/// of its expand area, moved by the free space of its header and framed by the speech bubble
/// behind it.
///
Window
{
  id: root

  // Properties
  required property SettingsListModel listModelSettings
  required property ScriptureListModel listModelScripture
  required property BridgeBibleRefOcr bridgeBibleRefOcr
  required property BridgeBibleRefLookup bridgeBibleRefLookup
  required property BridgeApplication bridgeApplication
  required property BridgeScripture bridgeScripture
  required property rect mainRect
  required property bool pinned
  required property bool shown

  color: "transparent"
  flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
  visible: root.shown
  x: root.mainRect.x
  y: root.mainRect.y
  width: root.mainRect.width
  height: root.mainRect.height

  // Signals
  signal released()
  signal moveRequested(deltaX: int, deltaY: int)
  signal expandRequested(deltaX: int, deltaY: int, deltaWidth: int, deltaHeight: int)
  signal closeClicked()
  signal pinClicked()

  // Components
  Item
  {
    id: content

    // Properties
    anchors.fill: parent
    opacity: 0
    // Holds the keyboard focus of the window, which is what lets it answer the escape key
    focus: true

    // Connections
    ///
    /// Escape hides the window, like its close button does. Answered here and not by a shortcut
    /// of the application, so that an open popup is closed first and the window only last.
    ///
    Keys.onEscapePressed: (event) =>
    {
      root.closeClicked()
      event.accepted = true
    }

    // Animations
    // The window appears animated and disappears at once: what it shows belongs to the search
    // that asked for it, so it must not linger over what the user turns to next.
    states: State
    {
      name: "shown"
      when: root.shown

      PropertyChanges { content.opacity: 1 }
    }
    transitions: Transition
    {
      to: "shown"

      NumberAnimation
      {
        property: "opacity"
        duration: Metrics.durationShort
        easing.type: Easing.InOutQuad
      }
    }

    // Components
    ExpandArea
    {
      // Properties
      anchors.fill: parent
      expandable: true
      expandAreaWidth: Metrics.spacingLarge

      // Connections
      onReleased: { root.released() }
      onExpandRequested: (deltaX, deltaY, deltaWidth, deltaHeight) =>
      {
        root.expandRequested(deltaX, deltaY, deltaWidth, deltaHeight)
      }

      // Components
      MainTabLayout
      {
        // Properties
        listModelSettings: root.listModelSettings
        listModelScripture: root.listModelScripture
        bridgeBibleRefOcr: root.bridgeBibleRefOcr
        bridgeBibleRefLookup: root.bridgeBibleRefLookup
        bridgeApplication: root.bridgeApplication
        bridgeScripture: root.bridgeScripture
        pinned: root.pinned
        movable: true

        anchors.fill: parent

        // Connections
        onCloseClicked: { root.closeClicked() }
        onPinClicked: { root.pinClicked() }
        onReleased: { root.released() }
        onMoveRequested: (deltaX, deltaY) => { root.moveRequested(deltaX, deltaY) }
      }
    }
  }
}
