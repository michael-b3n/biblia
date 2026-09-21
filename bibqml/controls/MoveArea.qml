import QtQuick

///
/// Mouse area that lets the call site move the window it belongs to. It only reports the deltas
/// of the drag, where the window ends up is up to the call site. The cursor marks it as the
/// handle of the window, everything else the window shows must not take it along.
///
MouseArea
{
  id: root

  // Properties
  required property bool movable

  // Position the drag started at
  property int clickX: 0
  property int clickY: 0

  cursorShape: root.movable ? Qt.SizeAllCursor : Qt.ArrowCursor
  hoverEnabled: true

  // Signals
  signal moveRequested(deltaX: int, deltaY: int)

  // Connections
  onPressed: (mouse) =>
  {
    root.clickX = mouse.x
    root.clickY = mouse.y
  }
  onPositionChanged: (mouse) =>
  {
    if(root.movable && root.pressed)
    {
      root.moveRequested(mouse.x - root.clickX, mouse.y - root.clickY)
    }
  }
}
