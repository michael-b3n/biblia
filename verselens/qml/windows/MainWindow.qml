import QtQuick
import BibQml

///
/// Window carrying the content. It has no decoration of its own, it is resized by the corners
/// of its expand area, moved by the free space of its header and framed by the speech bubble
/// behind it. Both drags are handed to the system, only it takes the window across screens of
/// different scaling.
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
  required property BridgeScript bridgeScript
  required property rect mainRect
  required property bool pinned
  required property bool shown

  // Screen the window is on
  readonly property rect screenGeometry: Qt.rect(Screen.virtualX, Screen.virtualY, Screen.width, Screen.height)
  // Whether the system moves or resizes the window at the moment, only then it reports its area
  property bool dragging: false

  color: "transparent"
  flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
  visible: root.shown
  x: root.mainRect.x
  y: root.mainRect.y
  width: root.mainRect.width
  height: root.mainRect.height

  // Signals
  signal released()
  signal dragged(area: rect)
  signal closeClicked()
  signal pinClicked()

  // Connections
  onXChanged: { root.reportDrag() }
  onYChanged: { root.reportDrag() }
  onWidthChanged: { root.reportDrag() }
  onHeightChanged: { root.reportDrag() }

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
      onReleased: { root.endDrag() }
      onExpandRequested: (edges) => { root.dragging = root.startSystemResize(edges) }

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
        bridgeScript: root.bridgeScript
        pinned: root.pinned
        movable: true

        anchors.fill: parent

        // Connections
        onCloseClicked: { root.closeClicked() }
        onPinClicked: { root.pinClicked() }
        onReleased: { root.endDrag() }
        onMoveRequested: { root.dragging = root.startSystemMove() }
      }
    }
  }

  // Functions
  ///
  /// Reports the area the system dragged the window to.
  ///
  function reportDrag()
  {
    if(root.dragging)
    {
      root.dragged(Qt.rect(root.x, root.y, root.width, root.height))
    }
  }

  ///
  /// Ends the drag of the system.
  ///
  function endDrag()
  {
    root.dragging = false
    root.released()
  }
}
