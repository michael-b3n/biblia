import QtQuick
import QtQuick.VectorImage
import BibQml

///
/// Tab button showing a single svg icon.
///
TabButtonBase
{
  id: root

  // Properties
  required property string svgSource

  // Components
  VectorImage
  {
    // Properties
    anchors.fill: parent
    source: root.svgSource
    preferredRendererType: VectorImage.CurveRenderer
  }
}
