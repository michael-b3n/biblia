import QtQuick
import QtQuick.Controls
import BibQml

///
/// Base of all icon buttons.
/// Implementations shall add their content, the size
/// is expected to be set by the call site.
///
Button
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
    color: root.pressed ? Colors.pressed : (root.hovered ? Colors.selection : Qt.alpha(Colors.selection, 0))
    radius: Metrics.radiusSmall

    // Animations
    Behavior on color { ColorAnimation { duration: Metrics.durationShort } }
  }
}
