import QtQuick
import QtQuick.Controls
import BibQml

///
/// Button showing a single line of text, as wide as that text.
///
Button
{
  id: root

  // Properties
  implicitWidth: label.implicitWidth + root.leftPadding + root.rightPadding
  implicitHeight: Metrics.controlHeight
  leftPadding: Metrics.spacingMedium
  rightPadding: Metrics.spacingMedium
  topPadding: 0
  bottomPadding: 0
  opacity: root.enabled ? 1 : 0.5

  // Animations
  Behavior on opacity { NumberAnimation { duration: Metrics.durationShort } }

  // Components
  contentItem: TextSimple
  {
    id: label

    // Properties
    text: root.text
    horizontalAlignment: Text.AlignHCenter
  }

  // Style
  background: BackgroundSimple
  {
    // Properties
    color: root.pressed ? Colors.pressed : (root.hovered ? Colors.selection : Colors.backgroundSolidDarker)
  }
}
