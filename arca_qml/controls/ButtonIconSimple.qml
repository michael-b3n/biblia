import QtQuick
import QtQuick.VectorImage
import ArcaQml

///
/// Icon button showing a single svg icon, or a turning loading icon while it is busy.
///
ButtonBase
{
  id: root

  // Properties
  required property string svgSource
  property int iconSize: Math.min(root.width, root.height)
  property bool busy: false

  // Faded as a whole, so the overlapping strokes of an icon do not darken
  layer.enabled: root.opacity < 1

  // Components
  VectorImage
  {
    // Properties
    anchors.centerIn: parent
    width: root.iconSize
    height: root.iconSize
    visible: !root.busy
    source: root.svgSource
    preferredRendererType: VectorImage.CurveRenderer
  }

  VectorImage
  {
    id: spinner

    // Properties
    anchors.centerIn: parent
    width: root.iconSize
    height: root.iconSize
    visible: root.busy
    source: Icons.loading
    preferredRendererType: VectorImage.CurveRenderer

    // Animations
    RotationAnimator on rotation
    {
      running: spinner.visible
      loops: Animation.Infinite
      from: 0
      to: 360
      duration: Metrics.durationLong
    }
  }
}
