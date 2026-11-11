import QtQuick
import QtQuick.Shapes
import ArcQml

///
/// Transparent button covering the reference on the screen. While the search runs it sits at the
/// cursor and reports itself by a segment traveling along its border, once the reference is found
/// it grows onto it. It times out, the content below it may have scrolled away by then. An icon
/// on its right offers the action a click does not run.
///
FadingWindow
{
  id: root

  // Properties
  required property rect overlayRect
  required property bool loading
  required property int timeout
  // Action of a click, one of the actions below
  required property string clickAction
  required property bool iconBusy

  // Constants
  // Actions on the reference, the stored values of the setting of a click
  readonly property string actionLookupBrowser: "LookupBrowser"
  readonly property string actionCopyClipboard: "CopyClipboard"
  readonly property string actionNone: "None"
  // Time the icon confirms an action
  readonly property int confirmationDuration: 1000

  // Area the window covers, it follows the requested one animated
  property rect currentRect: Qt.rect(0, 0, 0, 0)

  // Action of the icon, the one a click does not run
  readonly property string iconAction:
  {
    switch(root.clickAction)
    {
      case root.actionLookupBrowser: return root.actionCopyClipboard
      case root.actionCopyClipboard: return root.actionLookupBrowser
      default: return root.actionNone
    }
  }
  readonly property int iconSize: Metrics.controlHeight + Metrics.spacingSmall
  readonly property int glyphSize: Metrics.iconSize + Metrics.spacingSmall
  // Room the icon adds on the right of the reference, with a gap to its last char
  readonly property int iconSpace: root.iconAction !== root.actionNone ? root.iconSize + Metrics.spacingMedium : 0
  property bool confirmed: false
  readonly property bool hovered: mouseArea.containsMouse || iconButton.hovered

  // The overlay must not take the focus away from the window the user is reading.
  flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool | Qt.WindowDoesNotAcceptFocus
  x: root.currentRect.x
  y: root.currentRect.y
  width: root.currentRect.width
  height: root.currentRect.height

  // Signals
  signal actionTriggered(action: string)
  signal timedOut()

  // Connections
  Component.onCompleted: { root.applyOverlayRect() }
  // The requested area and the state it belongs to arrive as separate bindings, so the button
  // waits until both have settled before it decides to grow or to jump.
  onOverlayRectChanged: { Qt.callLater(root.applyOverlayRect) }
  onLoadingChanged: { Qt.callLater(root.applyOverlayRect) }
  onShownChanged: { Qt.callLater(root.applyOverlayRect) }

  // Animations
  PropertyAnimation
  {
    id: growAnimation

    // Properties
    target: root
    property: "currentRect"
    duration: Metrics.durationMedium
    easing.type: Easing.InOutQuad
  }

  // Components
  DashedBorder
  {
    id: dashedBorder

    // Properties
    borderWidth: root.width
    borderHeight: root.height
    radius: Metrics.radiusLarge
    strokeWidth: Metrics.borderThick
    segmentRatio: 0.25
  }

  Rectangle
  {
    // Properties
    // The reference below shall stay readable, so the area is only tinted while it is hovered.
    // Note the opacity never drops to zero, fully transparent pixels of a translucent window
    // are not hit by mouse events, which would make the overlay unclickable.
    anchors.fill: parent
    color: Colors.selection
    opacity: mouseArea.containsMouse ? Metrics.opacityLow : Metrics.opacityMinimal
    border.color: Colors.border
    border.width: mouseArea.containsMouse ? Metrics.border : 0
    radius: Metrics.radiusLarge

    // Animations
    Behavior on opacity
    {
      NumberAnimation
      {
        duration: Metrics.durationShort
        easing.type: Easing.InOutQuad
      }
    }
  }

  Shape
  {
    // Properties
    anchors.fill: parent
    opacity: root.loading ? 1 : 0

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
    ShapePath
    {
      // Properties
      strokeColor: Colors.border
      strokeWidth: Metrics.borderThick
      fillColor: "transparent"
      capStyle: ShapePath.RoundCap
      strokeStyle: ShapePath.DashLine
      // The pattern covers the whole border, so that exactly one segment travels along it.
      dashPattern: dashedBorder.dashPattern

      // Components
      PathRectangle
      {
        // Properties
        // The stroke is centered on the path, so it is inset to stay inside the button.
        x: Metrics.borderThick / 2
        y: Metrics.borderThick / 2
        width: root.width - Metrics.borderThick
        height: root.height - Metrics.borderThick
        radius: Metrics.radiusLarge
      }

      // Animations
      NumberAnimation on dashOffset
      {
        running: root.shown && root.loading
        loops: Animation.Infinite
        from: 0
        to: dashedBorder.period
        duration: Metrics.durationLong
      }
    }
  }

  MouseArea
  {
    id: mouseArea

    // Properties
    anchors.fill: parent
    enabled: !root.loading
    hoverEnabled: true
    cursorShape: Qt.PointingHandCursor

    // Connections
    onClicked: { root.actionTriggered(root.clickAction) }

    // Components
    Rectangle
    {
      id: iconItem

      // Properties
      // A segment at the right end of the button, its corners run along the ones of the button
      anchors.right: parent.right
      anchors.top: parent.top
      anchors.bottom: parent.bottom
      anchors.margins: Metrics.spacingTiny
      width: root.iconSize
      visible: root.iconSpace > 0 && !root.loading
      // The black icon must stay readable on a dark screen
      color: Qt.alpha(Colors.backgroundSolid, root.hovered ? Metrics.opacityHigh : Metrics.opacityLow)
      // Faint until the user turns to it or it reports something
      opacity: root.hovered || root.iconBusy || root.confirmed ? Metrics.opacityHigh : Metrics.opacityMedium
      // Faded as a whole, so neither the overlapping strokes of the icon nor the segment below it darken
      layer.enabled: true
      radius: Metrics.radiusLarge - Metrics.spacingTiny

      // Animations
      Behavior on color { ColorAnimation { duration: Metrics.durationShort } }
      Behavior on opacity { NumberAnimation { duration: Metrics.durationShort } }

      // Components
      ButtonIconSimple
      {
        id: iconButton

        // Properties
        anchors.fill: parent
        enabled: !root.iconBusy
        svgSource:
        {
          if(root.confirmed)
          {
            return Icons.checkMark
          }
          return root.iconAction === root.actionCopyClipboard ? Icons.copy : Icons.openInBrowser
        }
        iconSize: root.glyphSize
        busy: root.iconBusy

        // Connections
        onClicked: { root.actionTriggered(root.iconAction) }

        // Style
        // The segment shows the hover, a background of its own would not follow its corners
        background: null
      }
    }
  }

  Timer
  {
    id: confirmationTimer

    // Properties
    interval: root.confirmationDuration

    // Connections
    onTriggered: { root.confirmed = false }
  }

  Timer
  {
    // Properties
    interval: root.timeout
    running: root.shown && !root.loading && !root.hovered

    // Connections
    onTriggered: { root.timedOut() }
  }

  // Functions
  ///
  /// Shows for a moment that the action of the icon succeeded.
  ///
  function confirm()
  {
    root.confirmed = true
    confirmationTimer.restart()
  }

  ///
  /// Puts the button onto the area it shall cover. Only a shown button that knows the reference
  /// grows onto it, the others jump, so that none flies in from the previous search.
  ///
  function applyOverlayRect()
  {
    growAnimation.stop()
    if(root.shown && !root.loading)
    {
      growAnimation.to = root.overlayRect
      growAnimation.start()
    }
    else
    {
      /*no binding*/ root.currentRect = root.overlayRect
    }
  }
}
