import QtQuick
import QtQuick.Layouts
import BibQml

///
/// Window a second instance shows to tell the user that the application is already running. It
/// is the only thing that instance ever puts on the screen, closing it ends the instance.
///
Window
{
  id: root

  // Properties
  required property string applicationName

  // Constants
  // Width the message is wrapped at. It is fixed, the window takes its size from the content.
  readonly property int messageWidth: 240

  // Whether the notice is on the screen, it fades in once the window is up
  property bool shown: false

  color: "transparent"
  flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
  visible: true
  width: frame.implicitWidth
  height: frame.implicitHeight
  x: Screen.virtualX + Math.round((Screen.width - root.width) / 2)
  y: Screen.virtualY + Math.round((Screen.height - root.height) / 2)

  // Connections
  // The running instance may hold the foreground window, the notice has to be asked for in front of it.
  Component.onCompleted:
  {
    root.shown = true
    root.requestActivate()
  }

  // Components
  Rectangle
  {
    id: frame

    // Properties
    anchors.fill: parent
    implicitWidth: layout.implicitWidth + 2 * Metrics.spacingSmall
    implicitHeight: layout.implicitHeight + 2 * Metrics.spacingSmall
    color: Colors.backgroundSolid
    border.color: Colors.border
    border.width: Metrics.border
    radius: Metrics.radiusLarge
    opacity: 0
    // Holds the keyboard focus of the window, which is what lets it answer the escape key
    focus: true

    // Connections
    ///
    /// Escape closes the notice, like its close button does.
    ///
    Keys.onEscapePressed: (event) =>
    {
      root.close()
      event.accepted = true
    }

    // Animations
    // The notice fades in like the main window does, it must not snap onto the screen.
    states: State
    {
      name: "shown"
      when: root.shown

      PropertyChanges { frame.opacity: 1 }
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

    // Components
    ColumnLayout
    {
      id: layout

      // Properties
      anchors.fill: parent
      anchors.margins: Metrics.spacingSmall
      spacing: Metrics.spacingSmall

      // Components
      ///
      /// Header: the name of the application and the button taking the notice off the screen.
      ///
      RowLayout
      {
        // Properties
        Layout.fillWidth: true
        Layout.fillHeight: false
        Layout.preferredHeight: Metrics.controlHeight
        spacing: Metrics.spacingSmall

        // Components
        ///
        /// Names the notice, and is the one place it is moved by.
        ///
        MoveArea
        {
          // Properties
          Layout.fillWidth: true
          Layout.fillHeight: true
          movable: true

          // Connections
          onMoveRequested: (deltaX, deltaY) => { root.moveBy(deltaX, deltaY) }

          // Components
          TextSimple
          {
            // Properties
            anchors.fill: parent
            anchors.leftMargin: Metrics.spacingSmall
            text: root.applicationName
            font.bold: true
            elide: Text.ElideRight
          }
        }

        ButtonIconSimple
        {
          // Properties
          Layout.fillHeight: true
          Layout.fillWidth: false
          Layout.preferredWidth: Metrics.controlHeight
          Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
          svgSource: Icons.close

          // Connections
          onClicked: { root.close() }
        }
      }

      ///
      /// The notice itself.
      ///
      TextSimple
      {
        // Properties
        Layout.fillWidth: false
        Layout.preferredWidth: root.messageWidth
        Layout.leftMargin: Metrics.spacingMedium
        Layout.rightMargin: Metrics.spacingMedium
        Layout.bottomMargin: Metrics.spacingMedium
        Layout.alignment: Qt.AlignHCenter
        // Note the language is passed to reevaluate this binding on a language change.
        text: Translations.name("already_running", Translations.language)
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignLeft
      }
    }
  }

  // Functions
  ///
  /// Moves the notice by the given delta. It belongs to no position on the screen, so it only
  /// ends up where the user drags it.
  ///
  function moveBy(deltaX: int, deltaY: int)
  {
    /*no binding*/ root.x += deltaX
    /*no binding*/ root.y += deltaY
  }
}
