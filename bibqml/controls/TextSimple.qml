import QtQuick
import BibQml

///
/// Text in the default font size and color of the application.
/// Height shall be left untouched to be determined
/// by the content. Width has to be set.
///
Text
{
  id: root

  // Properties
  font.pointSize: Metrics.fontSizeParam
  color: Colors.text
  renderType: Text.CurveRendering
  verticalAlignment: Text.AlignVCenter
}
