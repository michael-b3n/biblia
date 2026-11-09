import QtQuick

///
/// Mouse area of a single resize corner. A press on it asks for a resize by the edges of the
/// target that meet in this corner.
///
MouseArea
{
  id: root

  // Properties
  // Edges of the target this corner drags, a combination of Qt.Edge values
  required property int edges

  hoverEnabled: true

  // Signals
  signal resizeRequested(edges: int)

  // Connections
  onPressed: { root.resizeRequested(root.edges) }
}
