import QtQuick

///
/// Parameter base object.
/// It is the titled box of the application, named by the setting it stands for. Param
/// implementations shall add the according design using the required properties. A param nested
/// in another one clears showTitle and is then drawn bare, the param it belongs to carries the
/// frame for it.
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
  /// The signal will notify the backend about the change,
  /// the value will be validated and loop back to UI
  /// if there was an error or the desired value was not set.
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
