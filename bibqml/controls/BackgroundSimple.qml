import QtQuick
import BibQml

///
/// Default background of fields, popups and their entries.
///
Rectangle
{
  // Properties
  color: Colors.backgroundSolidDarker
  border.color: Colors.border
  radius: Metrics.radiusSmall
  border.width: 0

  // Animations
  // Call sites recolor the background to mark focus or highlight, which shall not snap.
  Behavior on color { ColorAnimation { duration: Metrics.durationShort } }
}
