import QtQuick
import BibQml

///
/// Transparent window whose content fades in when it is shown and disappears at once when it is
/// hidden: what it shows belongs to the search that asked for it, so it must not linger.
///
Window
{
  id: root

  // Properties
  required property bool shown
  // Children of the window go into the item that fades
  default property alias fadingData: content.data

  // Whether the last frame on the screen shows content
  property bool contentPresented: false

  color: "transparent"
  // Stays until its content is off the screen, Windows shows the last frame of a window that comes back
  visible: root.shown || root.contentPresented

  // Connections
  onFrameSwapped: { root.contentPresented = content.opacity > 0 }

  // Components
  Item
  {
    id: content

    // Properties
    anchors.fill: parent
    opacity: 0

    // Animations
    states: State
    {
      name: "shown"
      when: root.shown

      PropertyChanges { content.opacity: 1 }
    }
    transitions: Transition
    {
      to: "shown"

      NumberAnimation
      {
        property: "opacity"
        duration: Metrics.durationShort
        easing.type: Easing.InOutQuad
      }
    }
  }
}
