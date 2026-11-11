import QtQuick
import ArcQml
import BibQml

///
/// Tab button of the notifications tab. The bell rings while a notification is unread.
///
TabButtonIconSimple
{
  id: root

  // Properties
  required property bool unread

  svgSource: root.unread ? BibIcons.bellRinging : BibIcons.bell
}
