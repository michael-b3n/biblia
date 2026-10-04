import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.VectorImage
import BibQml

///
/// Button showing a svg icon followed by a single line of text, both centered.
///
Button
{
  id: root

  // Properties
  required property string svgSource

  implicitWidth: content.implicitWidth + root.leftPadding + root.rightPadding
  implicitHeight: Metrics.controlHeight
  leftPadding: Metrics.spacingMedium
  rightPadding: Metrics.spacingMedium
  topPadding: 0
  bottomPadding: 0
  opacity: root.enabled ? 1 : 0.5

  // Animations
  Behavior on opacity { NumberAnimation { duration: Metrics.durationShort } }

  // Components
  contentItem: Item
  {
    // Properties
    implicitWidth: content.implicitWidth
    implicitHeight: content.implicitHeight

    // Components
    RowLayout
    {
      id: content

      // Properties
      anchors.centerIn: parent
      spacing: Metrics.spacingSmall

      // Components
      VectorImage
      {
        // Properties
        Layout.preferredWidth: Math.round(0.7 * Metrics.controlHeight)
        Layout.preferredHeight: Math.round(0.7 * Metrics.controlHeight)
        source: root.svgSource
        preferredRendererType: VectorImage.CurveRenderer
      }

      TextSimple
      {
        // Properties
        text: root.text
      }
    }
  }

  // Style
  background: BackgroundSimple
  {
    // Properties
    color: root.pressed ? Colors.pressed : (root.hovered ? Colors.selection : Colors.backgroundSolidDarker)
  }
}
