import QtQuick

///
/// Mouse area that lets the call site move the window it belongs to. It only reports the press a
/// drag starts with, moving the window is up to the call site. The cursor marks it as the handle
/// of the window, everything else the window shows must not take it along.
///
MouseArea
{
  id: root

  // Properties
  required property bool movable

  cursorShape: root.movable ? Qt.SizeAllCursor : Qt.ArrowCursor
  hoverEnabled: true

  // Signals
  signal moveRequested()

  // Connections
  onPressed:
  {
    if(root.movable)
    {
      root.moveRequested()
    }
  }
}
