import QtQuick
import QtQuick.VectorImage
import BibQml

///
/// Icon button showing a single svg icon.
///
ButtonBase
{
  id: root

  // Properties
  required property string svgSource
  property int iconSize: Math.min(root.width, root.height)

  // Faded as a whole, so the overlapping strokes of an icon do not darken
  layer.enabled: root.opacity < 1

  // Components
  VectorImage
  {
    // Properties
    anchors.centerIn: parent
    width: root.iconSize
    height: root.iconSize
    source: root.svgSource
    preferredRendererType: VectorImage.CurveRenderer
  }
}
