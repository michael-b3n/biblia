pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

///
/// Combobox parameter object.
/// For combobox params a valid options list (model)
/// is needed and values must be convertible to string.
///
ParamBase
{
  id: root

  // Components
  contentItem: ComboBox
  {
    id: control

    // Properties
    model: SimpleListModel{}

    // Note the language is passed to reevaluate this binding on a language change.
    displayText: control.currentIndex < 0 ? "" : Translations.name(control.currentText, Translations.language)

    // Size of the drop down indicator, the content is kept clear of it
    readonly property real indicatorWidth: control.availableHeight / 2
    readonly property real indicatorHeight: control.availableHeight / 3

    padding: Metrics.paddingParamContent
    topPadding: Metrics.spacingSmall
    bottomPadding: Metrics.spacingSmall
    spacing: Metrics.spacingTiny
    width: root.availableWidth
    implicitHeight: root.lineHeight + control.topPadding + control.bottomPadding

    // Connections
    onActivated: (index) =>
    {
      const idx = control.model.index(index, 0)
      const v = control.model.data(idx, SimpleListModel.ValueRole)
      if(v === undefined)
      {
        control.currentIndex = -1
      }
      else
      {
        root.paramValueChanged(v)
      }
    }
    Connections
    {
      target: root
      function onListValidatorDataChanged()
      {
        control.model.replace(root.listValidatorData)
        Qt.callLater(control.syncCurrentIndex)
      }
      function onValueChanged() { control.syncCurrentIndex() }
    }
    Component.onCompleted:
    {
      control.model.replace(root.listValidatorData)
      Qt.callLater(control.syncCurrentIndex)
    }

    // Components
    contentItem: TextSimple
    {
      id: contentText

      // Properties
      text: control.displayText
      elide: Text.ElideRight
      rightPadding: control.indicatorWidth + 2 * Metrics.paddingParamContent
      // The width and height of a combobox content item are ignored.
    }

    ///
    /// Drop down indicator. It points down while the list is closed and flips over while it is
    /// open, telling that clicking again closes it.
    ///
    indicator: TriangleShape
    {
      // Properties
      x: control.width - width - Metrics.paddingParamContent
      y: control.topPadding + (control.availableHeight - height) / 2
      width: control.indicatorWidth
      height: control.indicatorHeight
      color: control.pressed ? Colors.pressed : Colors.border
      transformOrigin: Item.Center
      // Note the popup is null until the control is built, the indicator is declared before it
      rotation: control.popup && control.popup.visible ? 180 : 0

      // Animations
      Behavior on rotation
      {
        NumberAnimation
        {
          duration: Metrics.durationShort
          easing.type: Easing.InOutQuad
        }
      }
      Behavior on color { ColorAnimation { duration: Metrics.durationShort } }
    }

    popup: PopupSimple
    {
      // Properties
      // Overlapping the border of the field joins the popup to it
      y: control.height - Metrics.border
      width: control.width
      // Capped, so that a list too long for it scrolls instead of running past the window
      height: Math.min(comboBoxListView.contentHeight + topPadding + bottomPadding,
                       Metrics.popupHeightMax)

      // Components
      contentItem: ListView
      {
        id: comboBoxListView

        // Properties
        clip: true
        implicitHeight: contentHeight
        model: control.popup.visible ? control.delegateModel : null
        currentIndex: control.highlightedIndex

        // Components
        ScrollBar.vertical: ScrollBarSimple {}
      }
    }

    delegate: ItemDelegate
    {
      id: itemDelegate

      // Properties
      required property var modelData
      required property int index
      highlighted: control.highlightedIndex === itemDelegate.index

      width: comboBoxListView.width
      padding: Metrics.spacingSmall

      // Components
      contentItem: TextSimple
      {
        id: textContent

        // Properties
        // Note the language is passed to reevaluate this binding on a language change.
        text:
        {
          const valid = itemDelegate.modelData !== undefined && itemDelegate.modelData !== null
          return valid ? Translations.name(String(itemDelegate.modelData), Translations.language) : ""
        }

        elide: Text.ElideRight
        width: itemDelegate.width
      }

      // Style
      background: BackgroundSimple
      {
        color: itemDelegate.highlighted ? Colors.selection : Colors.backgroundSolidDarker
        width: itemDelegate.width
      }
    }

    // Style
    background: BackgroundSimple { color: control.activeFocus ? Colors.selection : Colors.backgroundSolidDarker }

    // Functions
    ///
    /// Sync combobox index with the root value.
    /// This is needed since a value is provided and not an index.
    ///
    function syncCurrentIndex()
    {
      control.currentIndex = control.model ? control.model.indexOfValue(root.value) : -1
    }
  }
}

