import QtQuick
import BibQml

///
/// Area the main window covers, together with the math that puts it there.
///
/// Nothing but the functions below writes the area, so the window never moves on its own. It
/// moves exactly when its owner asks for it, which is what keeps it out of the user's way.
///
/// A pinned window stays where the user put it, an unpinned one follows the cursor of the search,
/// keeping the offset it was dragged to.
///
QtObject
{
  id: root

  // Properties
  // Cursor position an unpinned window is placed relative to, and the screen it is placed on
  required property point cursorPosition
  required property rect cursorScreenGeometry
  // Area an unpinned window steps aside for and the distance it keeps to it. An empty area
  // blocks nothing.
  required property rect blockedArea
  required property real blockedClearance
  // Length of the tail pointing at the window, it is kept free below a window never dragged yet
  required property int tailLength

  // Constants
  readonly property real goldenRatio: 1.618
  readonly property int minimalWidth: Metrics.controlHeight * root.goldenRatio * 3
  readonly property int minimalHeight: Metrics.controlHeight + 2 * Metrics.spacingSmall
  readonly property size defaultSize: Qt.size(root.minimalWidth * 2, root.minimalHeight * 10)

  // Settings, the internal path is not listed in the settings tab
  readonly property SettingBinding settingPinned: BridgeSettings.binding("internal.bubble.pinned", false)
  readonly property SettingBinding settingPinnedX: BridgeSettings.binding("internal.bubble.pinned_x", 0)
  readonly property SettingBinding settingPinnedY: BridgeSettings.binding("internal.bubble.pinned_y", 0)

  // Pinned state
  readonly property bool pinned: root.settingPinned.value

  // Area the window covers. It starts out at the stored position, where a pinned window is found
  // again after a restart.
  property rect area: Qt.rect(
    root.settingPinnedX.value, root.settingPinnedY.value, root.defaultSize.width, root.defaultSize.height
  )

  // Offset to the cursor the user dragged the window to. It starts out above the cursor, leaving
  // room for the tail below the window.
  property point offsetToCursor: Qt.point(
    -root.minimalWidth / root.goldenRatio, -(root.defaultSize.height + root.tailLength)
  )

  // Functions
  ///
  /// Places the window. A pinned one stays where it is, unless no screen shows it any more.
  ///
  function place()
  {
    if(root.pinned)
    {
      /*no binding*/ root.area = Placement.reachable(root.area)
      return
    }
    /*no binding*/ root.area = Placement.placedBeside(
      Qt.rect(
        root.cursorPosition.x + root.offsetToCursor.x,
        root.cursorPosition.y + root.offsetToCursor.y,
        root.area.width,
        root.area.height
      ),
      root.blockedArea,
      root.blockedClearance,
      root.cursorScreenGeometry
    )
  }

  ///
  /// Pins the window at its current position, or releases it back to the cursor.
  ///
  function setPinned(pinned)
  {
    if(pinned)
    {
      root.storePinnedPosition()
    }
    root.settingPinned.value = pinned
  }

  ///
  /// Stores the position the window is pinned at.
  ///
  function storePinnedPosition()
  {
    root.settingPinnedX.value = root.area.x
    root.settingPinnedY.value = root.area.y
  }

  ///
  /// Takes over the area the user dragged the window to and remembers its offset to the cursor,
  /// so that the window returns to it. The area is not kept on a screen, the window may cross
  /// from one screen to the next.
  ///
  function applyUserArea(target)
  {
    if(!root.pinned)
    {
      /*no binding*/ root.offsetToCursor = Qt.point(
        target.x - root.cursorPosition.x, target.y - root.cursorPosition.y
      )
    }
    /*no binding*/ root.area = target
  }
}
