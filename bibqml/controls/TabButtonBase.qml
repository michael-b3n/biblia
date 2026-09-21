import QtQuick
import QtQuick.Controls
import BibQml

///
/// Base of all tab buttons.
/// Implementations shall add their content, every tab button is a square of the size all
/// controls share, so that the bar they are listed in can size itself by them.
///
TabButton
{
  id: root

  // Properties
  implicitWidth: Metrics.controlHeight
  implicitHeight: Metrics.controlHeight
  padding: 0

  // Style
  background: Rectangle
  {
    // Properties
    // Note the pressed state takes precedence over the checked state,
    // otherwise pressing the current tab gives no feedback at all.
    color:
    {
      if(root.pressed) { return Colors.pressed }
      if(root.hovered) { return Colors.selection }
      if(root.checked) { return Colors.backgroundSolidDarker }
      return Qt.alpha(Colors.selection, 0)
    }
    radius: Metrics.radiusSmall

    // Animations
    Behavior on color { ColorAnimation { duration: Metrics.durationShort } }
  }
}
