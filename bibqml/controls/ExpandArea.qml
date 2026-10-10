import QtQuick
import BibQml

///
/// Mouse area that lets the call site resize what it covers by dragging one of its four corners.
/// It only reports the edges a drag starts at, resizing the target is up to the call site.
/// Everything but the corners is left to the content, which is why this area never moves its
/// target. That is the job of a MoveArea.
///
Item
{
  id: root

  // Properties
  required property bool expandable
  required property int expandAreaWidth

  // Signals
  signal released(mouse: MouseEvent)
  signal expandRequested(edges: int)

  // Components
  ///
  /// Corners of the area, they resize the target.
  ///
  ExpandCorner
  {
    id: topLeft

    // Properties
    anchors.top: parent.top
    anchors.left: parent.left
    width: root.expandAreaWidth
    height: root.expandAreaWidth
    cursorShape: Qt.SizeFDiagCursor
    edges: Qt.TopEdge | Qt.LeftEdge

    // Connections
    onReleased: (mouse) => { root.released(mouse) }
    onResizeRequested: (edges) => { root.requestExpand(edges) }
  }

  ExpandCorner
  {
    id: topRight

    // Properties
    anchors.top: parent.top
    anchors.right: parent.right
    width: root.expandAreaWidth
    height: root.expandAreaWidth
    cursorShape: Qt.SizeBDiagCursor
    edges: Qt.TopEdge | Qt.RightEdge

    // Connections
    onReleased: (mouse) => { root.released(mouse) }
    onResizeRequested: (edges) => { root.requestExpand(edges) }
  }

  ExpandCorner
  {
    id: bottomLeft

    // Properties
    anchors.bottom: parent.bottom
    anchors.left: parent.left
    width: root.expandAreaWidth
    height: root.expandAreaWidth
    cursorShape: Qt.SizeBDiagCursor
    edges: Qt.BottomEdge | Qt.LeftEdge

    // Connections
    onReleased: (mouse) => { root.released(mouse) }
    onResizeRequested: (edges) => { root.requestExpand(edges) }
  }

  ExpandCorner
  {
    id: bottomRight

    // Properties
    anchors.bottom: parent.bottom
    anchors.right: parent.right
    width: root.expandAreaWidth
    height: root.expandAreaWidth
    cursorShape: Qt.SizeFDiagCursor
    edges: Qt.BottomEdge | Qt.RightEdge

    // Connections
    onReleased: (mouse) => { root.released(mouse) }
    onResizeRequested: (edges) => { root.requestExpand(edges) }
  }

  // Functions
  ///
  /// Reports the edges a drag of a corner starts at, if this area may be resized at all.
  ///
  function requestExpand(edges)
  {
    if(root.expandable)
    {
      root.expandRequested(edges)
    }
  }
}
