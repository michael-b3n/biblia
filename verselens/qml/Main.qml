import QtQuick
import BibQml

///
/// Application root object. It owns the main window carrying the content, the bubble window
/// framing it and the overlay button covering the reference a search found.
///
QtObject
{
  id: root

  // Typedefs
  ///
  /// Phases the application goes through, they tell what is on the screen. Events move it from one
  /// phase to the next, see handle() for which event leads where.
  ///
  enum Phase
  {
    // Nothing is on the screen
    Hidden,
    // A search runs and reports itself by the overlay at the cursor
    Searching,
    // A search found nothing, the overlay keeps reporting it for a moment
    NotFound,
    // A reference was found and only its overlay is on the screen
    Found,
    // The main window is on the screen
    Shown
  }

  ///
  /// Events the phases react to.
  ///
  enum Event
  {
    // A manual search started
    SearchStarted,
    // A manual search finished, a reference it found has been reported already
    SearchFinished,
    // A manual or automatic search found a reference
    ReferenceFound,
    // The search that found nothing was reported long enough
    NotFoundElapsed,
    // The overlay of the found reference timed out
    OverlayTimedOut,
    // The window was asked for from the tray
    ShowRequested,
    // The window was closed
    CloseRequested
  }

  // Properties
  required property SettingsListModel listModelSettings
  required property ScriptureListModel listModelScripture
  required property BridgeBibleRefOcr bridgeBibleRefOcr
  required property BridgeBibleRefLookup bridgeBibleRefLookup
  required property BridgeApplication bridgeApplication
  required property BridgeScripture bridgeScripture

  // Constants
  readonly property int referenceOverlayTimeout: 10000
  // Time the overlay keeps reporting a search that found nothing. A search is over in an instant,
  // without it the user only gets a flicker at the cursor.
  readonly property int searchWithoutResultDuration: 2500
  // Size the overlay has while the area of the reference is still unknown
  readonly property int overlayDefaultSize: Metrics.controlHeight + 2 * Metrics.spacingSmall
  // Length of the bubble tail
  readonly property int tailLength: Metrics.spacingLarge * 2
  // Gap between the tail tip and the reference overlay, so the tail does not cover it
  readonly property int referenceTailGap: Metrics.spacingSmall

  // Settings
  readonly property SettingBinding settingHideWindow: BridgeSettings.binding("ui.hide_window", false)

  // Phase the application is in
  property int phase: Main.Phase.Hidden
  readonly property bool windowShown: root.phase === Main.Phase.Shown

  // Cursor position of the running or last search
  property point cursorPosition: Qt.point(0, 0)
  property rect cursorScreenGeometry: Placement.screenGeometryAt(Qt.point(0, 0))

  // Reference of the last search
  property QtObject reference: QtObject
  {
    id: reference

    // Properties
    property string bookId: ""
    property int chapterBegin: 0
    property int verseBegin: 0
    property int chapterEnd: 0
    property int verseEnd: 0
    // Area the reference covers on the screen, it is empty if the search could not tell where it is
    property rect area: Qt.rect(0, 0, 0, 0)

    readonly property bool areaKnown: reference.area.width > 0 && reference.area.height > 0
    // Area the overlay covers, the margin around the reference text makes it easier to hit
    readonly property rect overlayArea: Placement.grown(reference.area, Metrics.spacingMedium)

    // Functions
    ///
    /// Forgets where the reference is on the screen, which hides its overlay.
    ///
    function forgetArea()
    {
      /*no binding*/ reference.area = Qt.rect(0, 0, 0, 0)
    }
  }

  // Overlay button state. It sits at the cursor in its default size for as long as the area of the
  // reference is unknown.
  readonly property bool overlayLoading:
  {
    switch(root.phase)
    {
      // A search that found nothing keeps reporting itself for a moment, see NotFound
      case Main.Phase.Searching: // [[fallthrough]]
      case Main.Phase.NotFound: return !reference.areaKnown
      default: return false
    }
  }
  readonly property bool overlayShown:
  {
    switch(root.phase)
    {
      case Main.Phase.Searching: // [[fallthrough]]
      case Main.Phase.NotFound: return true
      case Main.Phase.Found: // [[fallthrough]]
      case Main.Phase.Shown: return reference.areaKnown
      default: return false
    }
  }
  readonly property rect overlayRect: reference.areaKnown
    ? reference.overlayArea
    : Placement.centeredSquare(root.cursorPosition, root.overlayDefaultSize, root.cursorScreenGeometry)

  // Point the tail points at: the border of the reference overlay, or the cursor
  readonly property point tailPosition: reference.areaKnown
    ? Placement.borderPointTowards(reference.overlayArea, root.referenceTailGap, mainPlacement.area)
    : root.cursorPosition

  // Placement
  property MainWindowPlacement mainPlacement: MainWindowPlacement
  {
    id: mainPlacement

    // Properties
    cursorPosition: root.cursorPosition
    cursorScreenGeometry: root.cursorScreenGeometry
    // The main window steps aside for the reference overlay, leaving room for the tail
    blockedArea: reference.overlayArea
    blockedClearance: root.tailLength + root.referenceTailGap
    tailLength: root.tailLength
  }

  // Connections
  property Connections bridgeConnections: Connections
  {
    target: root.bridgeBibleRefOcr

    function onCursorPositionChanged(cursorPosition)
    {
      root.cursorPosition = cursorPosition
      root.cursorScreenGeometry = Placement.screenGeometryAt(cursorPosition)
    }

    function onReferenceRangeFound(bookId, chapterBegin, verseBegin, chapterEnd, verseEnd, boundingBox)
    {
      reference.bookId = bookId
      reference.chapterBegin = chapterBegin
      reference.verseBegin = verseBegin
      reference.chapterEnd = chapterEnd
      reference.verseEnd = verseEnd
      reference.area = boundingBox
      root.handle(Main.Event.ReferenceFound)
    }

    function onManualSearchRunningChanged(manualSearchRunning)
    {
      if(manualSearchRunning)
      {
        root.handle(Main.Event.SearchStarted)
      }
      else
      {
        // The result follows this notification, so the search is only finished afterwards.
        Qt.callLater(root.finishSearch)
      }
    }
  }

  property Connections applicationConnections: Connections
  {
    target: root.bridgeApplication

    function onShowWindowRequested() { root.handle(Main.Event.ShowRequested) }
  }

  // Timer keeping the overlay of a search that found nothing on the screen for a moment
  property Timer searchWithoutResultTimer: Timer
  {
    // Properties
    interval: root.searchWithoutResultDuration
    running: root.phase === Main.Phase.NotFound

    // Connections
    onTriggered: { root.handle(Main.Event.NotFoundElapsed) }
  }

  // Windows
  property Window bubble: SpeechBubbleWindow
  {
    id: bubble

    // Properties
    screenGeometry: mainPlacement.screenGeometry
    bubbleRect: mainPlacement.area
    tailPosition: root.tailPosition
    // A pinned main window does not belong to a position on the screen, so it shows no tail.
    tailVisible: !mainPlacement.pinned
    shown: root.windowShown

    // Connections
    onVisibleChanged: { Qt.callLater(root.raiseWindows) }
  }

  property Window referenceOverlay: ReferenceOverlayWindow
  {
    // Properties
    overlayRect: root.overlayRect
    loading: root.overlayLoading
    shown: root.overlayShown
    timeout: root.referenceOverlayTimeout

    // Connections
    onClicked: { root.triggerReferenceClickAction() }
    onTimedOut: { root.handle(Main.Event.OverlayTimedOut) }
  }

  property Window main: MainWindow
  {
    id: main

    // Properties
    listModelSettings: root.listModelSettings
    listModelScripture: root.listModelScripture
    bridgeBibleRefOcr: root.bridgeBibleRefOcr
    bridgeBibleRefLookup: root.bridgeBibleRefLookup
    bridgeApplication: root.bridgeApplication
    bridgeScripture: root.bridgeScripture
    mainRect: mainPlacement.area
    pinned: mainPlacement.pinned
    shown: root.windowShown

    // Connections
    onVisibleChanged: { Qt.callLater(root.raiseWindows) }
    onReleased:
    {
      root.raiseWindows()
      if(mainPlacement.pinned)
      {
        mainPlacement.storePinnedPosition()
      }
    }
    onMoveRequested: (deltaX, deltaY) => { mainPlacement.moveBy(deltaX, deltaY) }
    onExpandRequested: (deltaX, deltaY, deltaWidth, deltaHeight) =>
    {
      mainPlacement.resizeBy(deltaX, deltaY, deltaWidth, deltaHeight)
    }
    onCloseClicked: { Qt.callLater(root.closeWindow) }
    onPinClicked: { mainPlacement.setPinned(!mainPlacement.pinned) }
  }

  // Functions
  ///
  /// Moves the application to the phase an event leads to. Every phase lists the events it reacts
  /// to, an event a phase does not list leaves it as it is.
  ///
  function handle(event: int)
  {
    switch(root.phase)
    {
      case Main.Phase.Hidden:
      {
        switch(event)
        {
          case Main.Event.SearchStarted: root.enterSearching(); break
          case Main.Event.ReferenceFound: root.enterReference(); break
          case Main.Event.ShowRequested: root.enterShown(); break
        }
        break
      }
      case Main.Phase.Searching:
      {
        switch(event)
        {
          case Main.Event.SearchStarted: root.enterSearching(); break
          case Main.Event.SearchFinished: root.enterNotFound(); break
          case Main.Event.ReferenceFound: root.enterReference(); break
          case Main.Event.ShowRequested: root.enterShown(); break
        }
        break
      }
      case Main.Phase.NotFound:
      {
        switch(event)
        {
          case Main.Event.SearchStarted: root.enterSearching(); break
          case Main.Event.ReferenceFound: root.enterReference(); break
          case Main.Event.NotFoundElapsed: root.enterHidden(); break
          case Main.Event.ShowRequested: root.enterShown(); break
        }
        break
      }
      case Main.Phase.Found:
      {
        switch(event)
        {
          case Main.Event.SearchStarted: root.enterSearching(); break
          case Main.Event.ReferenceFound: root.enterReference(); break
          case Main.Event.OverlayTimedOut: root.enterHidden(); break
          case Main.Event.ShowRequested: root.enterShown(); break
        }
        break
      }
      case Main.Phase.Shown:
      {
        switch(event)
        {
          case Main.Event.SearchStarted: root.enterSearching(); break
          case Main.Event.ReferenceFound: root.enterShown(); break
          case Main.Event.OverlayTimedOut: reference.forgetArea(); break
          case Main.Event.CloseRequested: root.enterHidden(); break
        }
        break
      }
    }
  }

  ///
  /// Finishes the manual search, called once its result has been reported.
  /// \note Qt.callLater merges calls of the same function, so every deferred event has its own.
  ///
  function finishSearch()
  {
    root.handle(Main.Event.SearchFinished)
  }

  ///
  /// Closes the main window.
  ///
  function closeWindow()
  {
    root.handle(Main.Event.CloseRequested)
  }

  ///
  /// Takes everything off the screen.
  ///
  function enterHidden()
  {
    root.phase = Main.Phase.Hidden
    reference.forgetArea()
  }

  ///
  /// Begins a search. The passage the main window shows belongs to the previous one, so the window
  /// disappears until this search has a result.
  ///
  function enterSearching()
  {
    root.phase = Main.Phase.Searching
    reference.forgetArea()
  }

  ///
  /// Keeps reporting a search that found nothing for a moment, so that the user sees it happened.
  ///
  function enterNotFound()
  {
    root.phase = Main.Phase.NotFound
  }

  ///
  /// Presents a found reference: by the main window, or by its overlay alone if the user hid the
  /// window. A reference without a known area has no overlay, nothing is on the screen then.
  ///
  function enterReference()
  {
    if(!root.settingHideWindow.value)
    {
      root.enterShown()
    }
    else if(reference.areaKnown)
    {
      root.phase = Main.Phase.Found
    }
    else
    {
      root.enterHidden()
    }
  }

  ///
  /// Places the main window and shows it there. It is placed before it is shown, so it never
  /// appears at the area of the previous search first. A window already on the screen only moves,
  /// taking it off for the placement would make it blink.
  ///
  function enterShown()
  {
    mainPlacement.place()
    root.phase = Main.Phase.Shown
  }

  ///
  /// Restores the window order: the bubble is the frame, so it stays below the content.
  ///
  function raiseWindows()
  {
    bubble.raise()
    main.raise()
  }

  ///
  /// Executes the action the user configured for a click on a found reference.
  ///
  function triggerReferenceClickAction()
  {
    switch(root.bridgeBibleRefOcr.clickAction())
    {
      case BridgeBibleRefOcr.LookupBrowser:
      {
        root.bridgeBibleRefLookup.lookup(
          reference.bookId, reference.chapterBegin, reference.verseBegin, reference.chapterEnd, reference.verseEnd
        )
        break
      }
      case BridgeBibleRefOcr.None: break
    }
  }
}
