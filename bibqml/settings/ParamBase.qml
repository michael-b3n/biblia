import QtQuick
import BibQml

///
/// Titled box of a setting, the base of the params. A param nested in another one clears showTitle and is drawn
/// bare, the outer param carries the frame.
///
GroupBoxSimple
{
  id: root

  // Properties
  // Segments the setting is named by, the first one being the category it is grouped under
  required property var categories
  required property int valueType
  required property int wrapperType
  required property int validatorType
  required property var value
  required property var listValidatorData
  required property string postfix

  property bool showTitle: true
  readonly property int lineHeight: Math.ceil(paramLine.boundingRect.height)

  framed: root.showTitle
  // The category is already named by the section the setting is listed under, so only the
  // segments below it are left to name here. A setting without a category names itself.
  // Note the language is passed to reevaluate this binding on a language change.
  title: Translations.names(root.categories.length > 1 ? root.categories.slice(1) : root.categories,
                            Translations.language)

  // Signals
  ///
  /// Notifies the backend of a change, a value the setting corrects loops back to the UI.
  ///
  signal paramValueChanged(value: var)

  // Components
  TextMetrics
  {
    id: paramLine

    // Properties
    font.pointSize: Metrics.fontSizeParam
    text: "Ag"
  }
}
