import QtQuick
import QtQuick.Controls
import BibQml

///
/// Popup fading in and out on the default background.
///
Popup
{
  // Properties
  padding: Metrics.spacingSmall

  // Animations
  enter: Transition
  {
    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Metrics.durationShort }
  }
  exit: Transition
  {
    NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Metrics.durationShort }
  }

  // Style
  background: BackgroundSimple {}
}
