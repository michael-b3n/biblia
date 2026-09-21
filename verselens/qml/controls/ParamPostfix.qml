import QtQuick

///
/// Postfix a param draws behind its value, e.g. the unit it is measured in. It belongs to the
/// setting and is never edited, so it stands apart from the value instead of being part of it.
///
TextSimple
{
  id: root

  // Properties
  color: Colors.border
  width: root.implicitWidth
  visible: root.text.length > 0
}
