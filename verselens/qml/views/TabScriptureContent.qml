pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.VectorImage
import BibQml

///
/// Content of the scripture tab. A list view lists the verses of the listModelScripture, loading
/// further ones whenever an end of the loaded range is reached. Without a single scripture there
/// is nothing to list, the tab then asks for one instead.
///
Item
{
  id: root

  // Properties
  required property ScriptureListModel listModelScripture
  required property BridgeBibleRefLookup bridgeBibleRefLookup
  required property BridgeScripture bridgeScripture

  readonly property bool scripturesAvailable: root.bridgeScripture.available

  // The tab fades in when it is switched to, the layout takes the previous one off at once
  opacity: root.visible ? 1 : 0
  // Flattened while it fades, blending its children one by one would let the verses show
  // through the sticky header
  layer.enabled: root.opacity > 0 && root.opacity < 1

  // Animations
  Behavior on opacity
  {
    NumberAnimation
    {
      duration: Metrics.durationShort
      easing.type: Easing.InOutQuad
    }
  }

  // Components
  ///
  /// Sticky header showing current book and chapter
  ///
  Rectangle
  {
    id: stickyHeader

    // Properties
    anchors.top: parent.top
    anchors.left: parent.left
    anchors.right: parent.right
    height: stickyHeaderText.implicitHeight + Metrics.spacingMedium
    visible: root.scripturesAvailable
    color: Colors.backgroundSolid
    z: 1

    // Components
    Text
    {
      id: stickyHeaderText

      // Properties
      anchors.centerIn: parent
      text: listView.currentBook + (listView.currentChapter > 0 ? " " + listView.currentChapter : "")
      font.pointSize: Metrics.fontSizeBody
      font.bold: true
      color: Colors.text
      renderType: Text.CurveRendering
    }

    ///
    /// Opens the chapter currently shown in the header in the browser.
    ///
    ButtonIconSimple
    {
      id: openInBrowserButton

      // Properties
      anchors.right: parent.right
      anchors.rightMargin: Metrics.spacingSmall
      anchors.verticalCenter: stickyHeaderText.verticalCenter
      width: stickyHeaderText.implicitHeight
      height: stickyHeaderText.implicitHeight
      visible: listView.currentChapter > 0
      enabled: openInBrowserButton.visible && !root.bridgeBibleRefLookup.running
      svgSource: Icons.openInBrowser

      // Connections
      onClicked: { root.bridgeBibleRefLookup.lookupChapter(listView.currentBookId, listView.currentChapter) }
    }

    Rectangle
    {
      // Properties
      anchors.bottom: parent.bottom
      anchors.left: parent.left
      anchors.right: parent.right
      height: Metrics.border
      color: Colors.border
    }
  }

  ///
  /// List view with the scripture content.
  ///
  ListView
  {
    id: listView

    // Properties
    // Verses requested from the model whenever an end of the loaded range is reached
    readonly property int pageSize: 10
    // Pixels of delegates kept alive outside the visible area
    readonly property int cachedPixels: 600

    property string currentBook: ""
    property string currentBookId: ""
    property int currentChapter: 0

    anchors.top: stickyHeader.bottom
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.bottom: parent.bottom
    clip: true
    visible: root.scripturesAvailable
    model: root.listModelScripture
    spacing: Metrics.spacingTiny
    cacheBuffer: listView.cachedPixels

    // Connections
    // Everything that reaches the model is answered one event loop pass later: a view must not
    // call back into the model that is still reporting the change which asked for it. Each of
    // those functions tests its condition again, the pass may have made the work unnecessary.
    onContentYChanged: { listView.updateHeader() }
    onCountChanged: { listView.updateHeader() }
    onAtYBeginningChanged: { Qt.callLater(listView.loadPrevious) }
    onAtYEndChanged: { Qt.callLater(listView.loadNext) }
    Connections
    {
      target: root.listModelScripture

      function onRefreshed() { Qt.callLater(listView.showReference) }
      function onDataChanged() { listView.updateHeader() }
    }

    // Components
    delegate: Column
    {
      id: delegateRoot

      // Properties
      required property string verseText
      required property string bookName
      required property int chapterNumber
      required property int verseNumber
      required property bool isHeader
      width: listView.width - scrollBar.width

      // Components
      ///
      /// Book header
      ///
      Loader
      {
        // Properties
        active: delegateRoot.isHeader
        width: parent.width
        sourceComponent: Rectangle
        {
          // Properties
          width: parent.width
          height: bookTitle.implicitHeight + Metrics.spacingLarge
          color: Colors.bookHeader
          radius: Metrics.radiusMedium

          // Components
          Text
          {
            id: bookTitle

            // Properties
            anchors.centerIn: parent
            text: delegateRoot.bookName + " %1".arg(delegateRoot.chapterNumber)
            font.pointSize: Metrics.fontSizeHeading
            font.bold: true
            color: Colors.bookHeaderText
            renderType: Text.CurveRendering
          }
        }
      }

      ///
      /// Verse row: badge + text
      ///
      Row
      {
        // Properties
        width: parent.width
        leftPadding: Metrics.spacingSmall
        spacing: Metrics.spacingMedium

        // Components
        ///
        /// Badge with verse number
        ///
        Rectangle
        {
          id: verseBadge

          // Properties
          width: verseNum.implicitWidth + Metrics.spacingLarge
          height: verseNum.implicitHeight + Metrics.spacingSmall
          radius: Metrics.radiusMedium
          color: Colors.verseBox
          anchors.top: parent.top
          anchors.topMargin: Metrics.spacingTiny

          Text
          {
            id: verseNum

            // Properties
            anchors.centerIn: parent
            text: delegateRoot.verseNumber
            font.pointSize: Metrics.fontSizeSmall
            font.bold: true
            color: Colors.verseText
            renderType: Text.CurveRendering
          }
        }

        ///
        /// Scripture text
        ///
        Text
        {
          // Properties
          width: parent.width - verseBadge.width - parent.spacing - parent.leftPadding
          text: delegateRoot.verseText
          textFormat: Text.RichText
          wrapMode: Text.WordWrap
          color: Colors.text
          font.pointSize: Metrics.fontSizeBody
          renderType: Text.CurveRendering
        }
      }
    }

    ScrollBar.vertical: ScrollBarSimple { id: scrollBar }

    // Functions
    ///
    /// Tells which row is at a position of the content. A position falling into the gap between
    /// two rows belongs to the row above it, which is what the second lookup covers.
    ///
    function rowAt(y)
    {
      const position = Math.max(0, Math.min(y, listView.contentHeight - 1))
      const row = listView.indexAt(listView.contentX, position)
      return row >= 0 ? row : listView.indexAt(listView.contentX, position - listView.spacing)
    }

    ///
    /// Takes the book and chapter of the topmost row into the sticky header.
    ///
    function updateHeader()
    {
      if(!listView.model || listView.model.rowCount() === 0)
      {
        listView.currentBook = ""
        listView.currentBookId = ""
        listView.currentChapter = 0
        return
      }
      const row = listView.rowAt(listView.contentY)
      if(row >= 0)
      {
        const idx = listView.model.index(row, 0)
        listView.currentBook = listView.model.data(idx, ScriptureListModel.BookNameRole)
        listView.currentBookId = listView.model.data(idx, ScriptureListModel.BookIdRole)
        listView.currentChapter = Number(listView.model.data(idx, ScriptureListModel.ChapterNumberRole))
      }
    }

    ///
    /// Takes the view to the reference the model was reset with.
    ///
    function showReference()
    {
      if(root.listModelScripture.rowCount() === 0)
      {
        return
      }
      // Lays out the rows of the reset now instead of at the next frame, a view whose delegates
      // do not exist yet cannot be positioned
      listView.forceLayout()
      listView.positionViewAtIndex(root.listModelScripture.referenceRow, ListView.Beginning)
    }

    ///
    /// Loads verses before the first one and keeps the view on the verse it is on, prepending
    /// alone would push what the user reads out of sight.
    ///
    function loadPrevious()
    {
      if(!listView.atYBeginning || root.listModelScripture.rowCount() === 0)
      {
        return
      }
      const rowsBefore = root.listModelScripture.rowCount()
      root.listModelScripture.loadPrevious(listView.pageSize)
      const addedCount = root.listModelScripture.rowCount() - rowsBefore
      if(addedCount > 0)
      {
        listView.positionViewAtIndex(addedCount, ListView.Beginning)
      }
    }

    ///
    /// Loads verses after the last one.
    ///
    function loadNext()
    {
      if(listView.atYEnd && root.listModelScripture.rowCount() > 0)
      {
        root.listModelScripture.loadNext(listView.pageSize)
      }
    }
  }

  ///
  /// Copyright of the scripture the verses are taken from. Only the icon floats over the verses
  /// instead of taking room from them, the statement itself is told on hover. A scripture that
  /// states no copyright shows nothing.
  ///
  Item
  {
    id: copyrightNote

    // Properties
    readonly property string statement: root.listModelScripture.scriptureCopyright
    readonly property bool available: copyrightNote.statement !== ""

    anchors.bottom: parent.bottom
    anchors.right: parent.right
    anchors.margins: Metrics.spacingTiny
    width: Metrics.controlHeight - Metrics.spacingSmall
    height: copyrightNote.width
    visible: copyrightNote.available
    z: 1

    // Components
    VectorImage
    {
      id: copyrightIcon

      // Properties
      anchors.fill: parent
      opacity: copyrightArea.containsMouse ? 1 : 0.3
      source: Icons.copyright
      preferredRendererType: VectorImage.CurveRenderer

      // Animations
      Behavior on opacity
      {
        NumberAnimation
        {
          duration: Metrics.durationShort
          easing.type: Easing.InOutQuad
        }
      }

      // Components
      MouseArea
      {
        id: copyrightArea

        // Properties
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
      }
    }

    ///
    /// The statement, shown above the icon while it is hovered.
    ///
    Rectangle
    {
      id: copyrightStatement

      // Properties
      readonly property int maximumWidth: root.width - 2 * Metrics.spacingSmall

      anchors.right: parent.right
      anchors.bottom: parent.top
      anchors.bottomMargin: Metrics.spacingTiny
      width: copyrightStatementText.contentWidth + 2 * Metrics.spacingSmall
      height: copyrightStatementText.contentHeight + 2 * Metrics.spacingSmall
      visible: copyrightStatement.opacity > 0
      opacity: copyrightArea.containsMouse ? 1 : 0
      color: Colors.backgroundSolid
      border.color: Colors.border
      border.width: Metrics.border
      radius: Metrics.radiusMedium

      // Animations
      Behavior on opacity
      {
        NumberAnimation
        {
          duration: Metrics.durationShort
          easing.type: Easing.InOutQuad
        }
      }

      // Components
      Text
      {
        id: copyrightStatementText

        // Properties
        x: Metrics.spacingSmall
        y: Metrics.spacingSmall
        width: copyrightStatement.maximumWidth - 2 * Metrics.spacingSmall
        text: copyrightNote.statement
        wrapMode: Text.WordWrap
        font.pointSize: Metrics.fontSizeSmall
        color: Colors.text
        renderType: Text.CurveRendering
      }
    }
  }

  Item
  {
    id: emptyState

    // Properties
    property bool nothingImported: false

    anchors.fill: parent
    visible: !root.scripturesAvailable

    // Connections
    Connections
    {
      target: root.bridgeScripture

      function onImportEnded(count) { emptyState.nothingImported = count === 0 }
    }

    // Components
    ColumnLayout
    {
      // Properties
      anchors.centerIn: parent
      width: emptyState.width - 2 * Metrics.spacingLarge
      spacing: Metrics.spacingMedium

      // Components
      ButtonIconSimple
      {
        id: importButton

        // Properties
        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: 2 * Metrics.controlHeight
        Layout.preferredHeight: 2 * Metrics.controlHeight
        enabled: !root.bridgeScripture.importRunning
        opacity: importButton.enabled ? 1 : 0.5
        svgSource: Icons.folderOpen
        iconSize: Math.round(1.5 * Metrics.controlHeight)

        // Animations
        Behavior on opacity { NumberAnimation { duration: Metrics.durationShort } }

        // Connections
        onClicked: { importDialog.open() }
      }

      TextSimple
      {
        // Properties
        Layout.fillWidth: true
        text: Translations.name("scriptures_none", Translations.language)
        font.pointSize: Metrics.fontSizeHeading
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
      }

      TextSimple
      {
        // Properties
        Layout.fillWidth: true
        text: Translations.name(
          emptyState.nothingImported ? "scriptures_import_empty" : "scriptures_import_hint", Translations.language
        )
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
      }
    }

    FolderDialog
    {
      id: importDialog

      // Properties
      title: Translations.name("scriptures_import", Translations.language)

      // Connections
      onAccepted:
      {
        emptyState.nothingImported = false
        root.bridgeScripture.importFolder(importDialog.selectedFolder)
      }
    }
  }
}
