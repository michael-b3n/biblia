pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import BibQml

///
/// Scroll bar of all scrollable views. It only shows itself while the view it belongs to is
/// scrolled, so that it takes no attention away from the content.
///
ScrollBar
{
  id: root

  // Properties
  policy: ScrollBar.AsNeeded

  // Style
  background: Rectangle
  {
    // Properties
    width: Metrics.controlHeight / 3
    implicitWidth: Metrics.controlHeight / 3
    color: Colors.backgroundSolid
    radius: Metrics.radiusMedium
    opacity: root.active ? 1.0 : 0.0

    // Animations
    Behavior on opacity { NumberAnimation { duration: Metrics.durationShort } }
  }

  contentItem: Rectangle
  {
    // Properties
    width: Metrics.controlHeight / 4
    implicitWidth: Metrics.controlHeight / 4
    radius: Metrics.radiusMedium
    color: root.pressed ? Colors.pressed : Colors.border
    opacity: root.active ? 1.0 : 0.0

    // Animations
    Behavior on opacity { NumberAnimation { duration: Metrics.durationShort } }
  }
}
