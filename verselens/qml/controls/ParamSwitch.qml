pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

///
/// Switch control for boolean value types.
///
ParamBase
{
  id: root

  // Components
  contentItem: Switch
  {
    id: control

    // Properties
    // Note optional params may hold no value at all.
    checked: root.value === undefined ? false : root.value

    padding: 0
    implicitWidth: root.availableWidth
    implicitHeight: root.lineHeight

    // Connections
    onClicked: { root.paramValueChanged(control.checked) }

    Connections
    {
      target: root
      function onValueChanged()
      {
        if(root.value !== undefined)
        {
          control.checked = root.value
        }
        else
        {
          // TODO display a warning that the optional setting holds no value
        }
      }
    }

    // Components
    indicator: Rectangle
    {
      // Properties
      x: control.width - width
      y: (control.height - height) / 2
      width: 2 * height
      height: control.implicitHeight
      radius: height / 2
      color: control.checked ? Colors.selection : Colors.backgroundSolidDarker
      border.color: control.checked ? Colors.borderDarker : Colors.border

      // Animations
      Behavior on color { ColorAnimation { duration: Metrics.durationShort } }
      Behavior on border.color { ColorAnimation { duration: Metrics.durationShort } }

      // Components
      ///
      /// Handle, it slides to the side the switch was toggled to.
      ///
      Rectangle
      {
        // Properties
        x: control.checked ? parent.width - width : 0
        width: parent.height
        height: parent.height
        radius: parent.height / 2
        color: control.down ? Colors.borderDarker : Colors.border
        border.color: control.checked ? (control.down ? Colors.greenDarker : Colors.green) : Colors.borderDarker
        border.width: Metrics.borderThick

        // Animations
        Behavior on x
        {
          NumberAnimation
          {
            duration: Metrics.durationShort
            easing.type: Easing.InOutQuad
          }
        }
        Behavior on color { ColorAnimation { duration: Metrics.durationShort } }
        Behavior on border.color { ColorAnimation { duration: Metrics.durationShort } }
      }
    }
  }
}
