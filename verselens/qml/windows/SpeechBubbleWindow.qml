import QtQuick
import BibQml

///
/// Transparent window covering the screen of the window whose rect it is given, drawing the
/// speech bubble shape around that window. It never takes any input.
///
FadingWindow
{
  id: root

  // Properties
  required property rect screenGeometry
  required property rect bubbleRect
  required property point tailPosition
  required property bool tailVisible

  // Geometry the bubble is drawn with, it only follows the requested one while the bubble is shown
  property rect currentBubbleRect: Qt.rect(0, 0, 0, 0)
  property point currentTailPosition: Qt.point(0, 0)
  // Area the window covers: the screen, and the bubble where it reaches beyond that screen
  readonly property rect coveredArea: Placement.united(
    root.screenGeometry, Placement.grown(root.bubbleRect, Metrics.spacingSmall)
  )

  flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.WindowTransparentForInput
  x: root.coveredArea.x
  y: root.coveredArea.y
  width: root.coveredArea.width
  height: root.coveredArea.height

  // Connections
  onBubbleRectChanged: { root.takeGeometry() }
  onTailPositionChanged: { root.takeGeometry() }
  onShownChanged: { root.takeGeometry() }

  // Components
  SpeechBubbleShape
  {
    // Properties
    radius: Metrics.radiusLarge
    tailVisible: root.tailVisible
    tailPositionX: root.currentTailPosition.x - root.x
    tailPositionY: root.currentTailPosition.y - root.y
    offsetToTailX: root.currentBubbleRect.x - root.currentTailPosition.x
    offsetToTailY: root.currentBubbleRect.y - root.currentTailPosition.y
    bubbleWidth: root.currentBubbleRect.width
    bubbleHeight: root.currentBubbleRect.height
    strokeColor: Colors.border
    fillColor: Colors.backgroundTransparent
  }

  // Functions
  ///
  /// Takes over the geometry the bubble shall be drawn with. A bubble that is not shown keeps the
  /// geometry it has, so that it is drawn where the window it frames is and nowhere else.
  ///
  function takeGeometry()
  {
    if(!root.shown)
    {
      return
    }
    /*no binding*/ root.currentBubbleRect = root.bubbleRect
    /*no binding*/ root.currentTailPosition = root.tailPosition
  }
}
