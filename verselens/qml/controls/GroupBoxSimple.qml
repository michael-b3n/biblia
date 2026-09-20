import QtQuick
import QtQuick.Controls

///
/// Box drawing a frame around its content, named by a title that is set into the top line
/// of that frame. Call sites add what belongs together through the content item.
///
Control
{
  id: root

  // Properties
  property string title: ""
  property bool framed: true

  readonly property int frameTop: root.framed ? Math.round(titleText.implicitHeight / 2) : 0
  readonly property int contentInset: root.framed ? root.frameTop + Metrics.spacingTiny : 0

  topPadding: root.frameTop + root.contentInset
  bottomPadding: root.contentInset
  leftPadding: root.contentInset
  rightPadding: root.contentInset

  // Style
  background: FrameShape
  {
    // Properties
    y: root.frameTop
    width: root.width
    height: root.height - root.frameTop
    visible: root.framed
    strokeColor: Colors.border
    strokeWidth: Metrics.border
    radius: Metrics.radiusMedium
    gapStart: titleArea.x
    gapEnd: titleArea.x + titleArea.width
  }

  // Components
  Item
  {
    id: titleArea

    // Properties
    visible: root.framed
    x: Metrics.spacingMedium
    width: Math.min(titleText.implicitWidth, root.width - 2 * Metrics.spacingMedium) + 2 * Metrics.spacingTiny
    height: titleText.implicitHeight

    // Components
    TextSimple
    {
      id: titleText

      // Properties
      text: root.title
      anchors.centerIn: titleArea
      width: titleArea.width - 2 * Metrics.spacingTiny
      font.pointSize: Metrics.fontSizeSmall
      color: Colors.border
      elide: Text.ElideRight
    }
  }
}
