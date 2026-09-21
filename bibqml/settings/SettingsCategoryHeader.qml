import QtQuick
import BibQml

///
/// Header of one settings category. It names the category and folds its settings away when it is
/// clicked. It is drawn as a heading with a rule running to the right of it.
///
Item
{
  id: root

  // Properties
  required property string category
  required property bool collapsed

  implicitHeight: Metrics.controlHeight + Metrics.spacingMedium

  // Signals
  signal toggled()

  // Components
  Item
  {
    id: heading

    // Properties
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.bottom: parent.bottom
    height: Metrics.controlHeight

    // Components
    Rectangle
    {
      // Properties
      anchors.fill: parent
      radius: Metrics.radiusSmall
      color:
      {
        if(mouseArea.pressed) { return Colors.pressed }
        if(mouseArea.containsMouse) { return Colors.selection }
        return Qt.alpha(Colors.selection, 0)
      }

      // Animations
      Behavior on color { ColorAnimation { duration: Metrics.durationShort } }
    }

    TriangleShape
    {
      id: indicator

      // Properties
      anchors.left: parent.left
      anchors.leftMargin: Metrics.spacingSmall
      anchors.verticalCenter: parent.verticalCenter
      width: Metrics.controlHeight / 2
      height: Metrics.controlHeight / 3
      color: Colors.borderDarker
      transformOrigin: Item.Center
      rotation: root.collapsed ? -90 : 0

      // Animations
      Behavior on rotation
      {
        NumberAnimation
        {
          duration: Metrics.durationShort
          easing.type: Easing.InOutQuad
        }
      }
    }

    TextSimple
    {
      id: title

      // Properties
      anchors.left: indicator.right
      anchors.leftMargin: Metrics.spacingSmall
      anchors.verticalCenter: parent.verticalCenter
      text: Translations.name(root.category, Translations.language)
      font.pointSize: Metrics.fontSizeHeading
      font.bold: true
      color: Colors.borderDarker
      elide: Text.ElideRight
    }

    Rectangle
    {
      // Properties
      anchors.left: title.right
      anchors.leftMargin: Metrics.spacingSmall
      anchors.right: parent.right
      anchors.rightMargin: Metrics.spacingSmall
      anchors.verticalCenter: parent.verticalCenter
      height: Metrics.border
      color: Colors.border
    }
  }

  MouseArea
  {
    id: mouseArea

    // Properties
    anchors.fill: heading
    hoverEnabled: true

    // Connections
    onClicked: { root.toggled() }
  }
}
