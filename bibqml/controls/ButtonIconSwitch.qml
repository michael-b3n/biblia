import QtQuick
import QtQuick.VectorImage
import BibQml

///
/// Icon button showing one of two svg icons.
/// The toggled state is owned by the call site, the button only reports the click.
///
ButtonBase
{
  id: root

  // Properties
  required property string svgSourceFirst
  required property string svgSourceSecond
  property bool toggled: false

  // Components
  // The two icons are crossfaded, so that the toggle reads as one icon changing and not as two
  // icons swapping.
  VectorImage
  {
    id: first

    // Properties
    anchors.fill: parent
    opacity: root.toggled ? 0 : 1
    visible: first.opacity > 0
    source: root.svgSourceFirst
    preferredRendererType: VectorImage.CurveRenderer

    // Animations
    Behavior on opacity { NumberAnimation { duration: Metrics.durationShort } }
  }

  VectorImage
  {
    id: second

    // Properties
    anchors.fill: parent
    opacity: root.toggled ? 1 : 0
    visible: second.opacity > 0
    source: root.svgSourceSecond
    preferredRendererType: VectorImage.CurveRenderer

    // Animations
    Behavior on opacity { NumberAnimation { duration: Metrics.durationShort } }
  }
}
