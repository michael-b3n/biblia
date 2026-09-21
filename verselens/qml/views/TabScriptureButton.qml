import QtQuick
import QtQuick.VectorImage
import BibQml

///
/// Tab button of the scripture tab. It doubles as the indicator of the reference search:
/// it spins while a search runs and rests on a check mark once it is over.
///
TabButtonBase
{
  id: root

  // Properties
  required property bool searchRunning

  // Components
  // The two icons are crossfaded, so that a search that is over for good is told apart from the
  // flicker of one that only paused between two frames.
  VectorImage
  {
    id: loading

    // Properties
    anchors.fill: parent
    opacity: root.searchRunning ? 1 : 0
    visible: loading.opacity > 0
    source: Icons.loading
    preferredRendererType: VectorImage.CurveRenderer

    // Animations
    Behavior on opacity { NumberAnimation { duration: Metrics.durationShort } }
    RotationAnimator on rotation
    {
      // Kept turning while it fades out, a spinner frozen mid-fade would read as a hang.
      running: root.visible && loading.visible
      loops: Animation.Infinite
      from: 0
      to: 360
      duration: Metrics.durationLong
    }
  }

  VectorImage
  {
    id: done

    // Properties
    anchors.fill: parent
    opacity: root.searchRunning ? 0 : 1
    visible: done.opacity > 0
    source: Icons.checkMark
    preferredRendererType: VectorImage.CurveRenderer

    // Animations
    Behavior on opacity { NumberAnimation { duration: Metrics.durationShort } }
  }
}
