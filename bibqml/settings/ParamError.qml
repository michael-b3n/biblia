import QtQuick
import BibQml

///
/// Error marker for unsupported param types.
/// This is for debugging only and should not be visible
///
ParamBase
{
  id: root

  // Components
  contentItem: BackgroundSimple
  {
    // Properties
    width: root.availableWidth
    height: errorText.implicitHeight

    // Components
    TextSimple
    {
      id: errorText

      // Properties
      padding: Metrics.paddingParamContent
      text: "Error"
    }
  }
}
