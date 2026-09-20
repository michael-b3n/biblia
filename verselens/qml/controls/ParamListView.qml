pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.VectorImage
import BibQml

///
/// Editable list parameter object.
/// For editable param lists a list wrapper type
/// is required for values that are convertible to string.
/// The validator type must be none.
///
ParamBase
{
  id: root

  // Components
  contentItem: Column
  {
    // Properties
    width: root.availableWidth
    spacing: Metrics.spacingTiny

    // Components
    ListView
    {
      id: listView

      // Properties
      property int dragIndex: -1
      property int dropIndex: -1
      readonly property real rowHeight:
        listView.count > 0 ? (listView.contentHeight + listView.spacing) / listView.count : 0

      model: SimpleListModel{}
      width: parent.width
      height: listView.contentHeight
      spacing: Metrics.spacingSmall
      interactive: false

      // Animations
      add: Transition
      {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Metrics.durationShort }
      }

      remove: Transition
      {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Metrics.durationShort }
      }

      displaced: Transition
      {
        NumberAnimation { property: "y"; duration: Metrics.durationShort; easing.type: Easing.InOutQuad }
      }

      // Connections
      Connections
      {
        target: root

        function onValueChanged() { listView.model.replace(root.value) }
      }

      Component.onCompleted: { listView.model.replace(root.value) }

      // Components
      delegate: Item
      {
        id: delegateRoot

        // Properties
        required property int index
        required property var value

        // What the control of the entry is left once the handle and the remove button took theirs
        readonly property real controlWidth:
          delegateRoot.width - 2 * (Metrics.iconSize + entry.spacing)

        // How far the row steps aside to open the gap the dragged one will drop into. Only the
        // rows between the place it was picked up from and the place it is headed for move, and
        // they move by one row towards the gap the dragged one left behind.
        readonly property real dragOffset:
        {
          if(listView.dragIndex < 0 || delegateRoot.index === listView.dragIndex)
          {
            return 0
          }
          if(delegateRoot.index > listView.dragIndex && delegateRoot.index <= listView.dropIndex)
          {
            return -listView.rowHeight
          }
          if(delegateRoot.index < listView.dragIndex && delegateRoot.index >= listView.dropIndex)
          {
            return listView.rowHeight
          }
          return 0
        }

        // Whether this is the entry the user picked up
        readonly property bool dragged: listView.dragIndex === delegateRoot.index

        width: listView.width
        height: entry.height
        // The dragged entry is drawn over the ones it is pulled across
        z: delegateRoot.dragged ? 1 : 0
        // Stepping aside is a transform and not a position, so that it stays out of the way of
        // the view, which is the one placing the rows.
        transform: Translate
        {
          y: delegateRoot.dragOffset

          Behavior on y
          {
            NumberAnimation { duration: Metrics.durationShort; easing.type: Easing.InOutQuad }
          }
        }

        // Components
        Rectangle
        {
          // Properties
          anchors.fill: parent
          visible: delegateRoot.dragged
          radius: Metrics.radiusSmall
          color: Colors.selection
        }

        Row
        {
          id: entry

          // Properties
          width: parent.width
          spacing: Metrics.spacingTiny

          // Components
          ///
          /// Handle the entry is dragged at. It is the only part of the entry that starts a drag,
          /// which leaves the control next to it the gestures it needs itself.
          ///
          Item
          {
            id: handle

            // Properties
            // The column is kept even with nothing to order, so that the rows stay aligned
            anchors.verticalCenter: parent.verticalCenter
            width: Metrics.iconSize
            height: Metrics.controlHeight

            // Components
            VectorImage
            {
              // Properties
              anchors.centerIn: parent
              width: Metrics.iconSize
              height: Metrics.iconSize
              // There is nothing to order while the list holds a single entry
              visible: listView.count > 1
              source: Icons.dragHandle
              preferredRendererType: VectorImage.CurveRenderer
            }

            HoverHandler
            {
              // Properties
              enabled: listView.count > 1
              // The hand cursors are drawn by Qt itself and ignore the cursor the user picked,
              // the resize ones are the native cursors and tell the drag axis as well.
              cursorShape: Qt.SizeVerCursor
            }

            DragHandler
            {
              id: dragHandler

              // Properties
              target: null
              enabled: listView.count > 1
              xAxis.enabled: false
              cursorShape: Qt.SizeVerCursor

              property real startY: 0

              // Connections
              onActiveChanged:
              {
                if(dragHandler.active)
                {
                  dragHandler.startY = delegateRoot.y
                  listView.dragIndex = delegateRoot.index
                  listView.dropIndex = delegateRoot.index
                  return
                }

                const from = listView.dragIndex
                const to = listView.dropIndex
                delegateRoot.y = dragHandler.startY
                listView.dragIndex = -1
                listView.dropIndex = -1
                if(to !== from)
                {
                  listView.moveItem(from, to)
                }
              }
              onTranslationChanged:
              {
                if(!dragHandler.active)
                {
                  return
                }

                // The entry is held inside the list, one pulled past its end would land nowhere
                const limit = listView.contentHeight - delegateRoot.height
                delegateRoot.y =
                  Math.max(0, Math.min(limit, dragHandler.startY + dragHandler.translation.y))

                const offset = Math.round((delegateRoot.y - dragHandler.startY) / listView.rowHeight)
                listView.dropIndex =
                  Math.max(0, Math.min(listView.count - 1, listView.dragIndex + offset))
              }
            }
          }

          Loader
          {
            // Properties
            // The loader carries the width, it resizes whichever control it holds to it
            width: delegateRoot.controlWidth
            sourceComponent:
            {
              // param has list wrapper type
              switch(root.valueType)
              {
              case SettingsListModel.BoolValueType: return paramSwitch
              case SettingsListModel.IntValueType: // [[fallthrough]]
              case SettingsListModel.DoubleValueType: // [[fallthrough]]
              case SettingsListModel.TimeValueType: // [[fallthrough]]
              case SettingsListModel.StringValueType: // [[fallthrough]]
              case SettingsListModel.PathValueType:
                switch(root.validatorType)
                {
                case SettingsListModel.UnboundValidatorType: // [[fallthrough]]
                case SettingsListModel.RangeValidatorType: return paramTextField
                case SettingsListModel.ListValidatorType: return paramComboBox
                default: return paramError
                }
              default: return paramError
              }
            }

            // Components
            Component
            {
              id: paramSwitch

              // Components
              ParamSwitch
              {
                // Properties
                categories: root.categories
                valueType: root.valueType
                wrapperType: root.wrapperType
                validatorType: root.validatorType
                value: delegateRoot.value
                listValidatorData: root.listValidatorData

                showTitle: false

                // Connections
                onParamValueChanged: (value) => { listView.updateItem(delegateRoot.index, value) }
              }
            }
            Component
            {
              id: paramTextField

              // Components
              ParamTextField
              {
                // Properties
                categories: root.categories
                valueType: root.valueType
                wrapperType: root.wrapperType
                validatorType: root.validatorType
                value: delegateRoot.value
                listValidatorData: root.listValidatorData

                showTitle: false

                // Connections
                onParamValueChanged: (value) => { listView.updateItem(delegateRoot.index, value) }
              }
            }
            Component
            {
              id: paramComboBox

              // Components
              ParamComboBox
              {
                // Properties
                categories: root.categories
                valueType: root.valueType
                wrapperType: root.wrapperType
                validatorType: root.validatorType
                value: delegateRoot.value
                listValidatorData: root.listValidatorData

                showTitle: false

                // Connections
                onParamValueChanged: (value) => { listView.updateItem(delegateRoot.index, value) }
              }
            }
            Component
            {
              id: paramError

              // Components
              ParamError
              {
                // Properties
                categories: root.categories
                valueType: root.valueType
                wrapperType: root.wrapperType
                validatorType: root.validatorType
                value: delegateRoot.value
                listValidatorData: root.listValidatorData

                showTitle: false
              }
            }
          }

          ///
          /// Removes the entry it belongs to. It is listed in a column of its own, so that it is
          /// told apart from whatever the control next to it draws at its own right edge.
          ///
          ButtonIconSimple
          {
            // Properties
            anchors.verticalCenter: parent.verticalCenter
            svgSource: Icons.remove
            iconSize: Metrics.iconSize
            width: Metrics.iconSize
            height: Metrics.controlHeight

            // Connections
            onClicked: { listView.removeVar(delegateRoot.index) }
          }
        }
      }

      // Functions
      function addVar()
      {
        switch(root.valueType)
        {
        case SettingsListModel.BoolValueType:
          listView.model.append(false)
          break
        case SettingsListModel.IntValueType:
          listView.model.append(0)
          break
        case SettingsListModel.DoubleValueType:
        case SettingsListModel.TimeValueType:
          listView.model.append(0.0)
          break
        case SettingsListModel.StringValueType:
        case SettingsListModel.PathValueType:
          listView.model.append("")
          break
        default:
          BridgeLogger.error("unsupported SettingsListModel::ValueType value")
          return
        }
        root.paramValueChanged(listView.model.entries())
      }
      function removeVar(index)
      {
        const idx = listView.model.index(index, 0)
        if(listView.model.remove(idx))
        {
          root.paramValueChanged(listView.model.entries())
        }
      }
      function updateItem(index, value)
      {
        const idx = listView.model.index(index, 0)
        if(listView.model.setData(idx, value, SimpleListModel.ValueRole))
        {
          root.paramValueChanged(listView.model.entries())
        }
      }
      function moveItem(fromIndex, toIndex)
      {
        const fromIdx = listView.model.index(fromIndex, 0)
        const toIdx = listView.model.index(toIndex, 0)
        if(listView.model.move(fromIdx, toIdx))
        {
          root.paramValueChanged(listView.model.entries())
        }
      }
    }

    ///
    /// Adds an entry below the ones already listed.
    ///
    ButtonBase
    {
      // Properties
      width: parent.width

      // Connections
      onClicked: { listView.addVar() }

      // Components
      VectorImage
      {
        // Properties
        anchors.left: parent.left
        // Lined up with the grips of the entries above it
        anchors.leftMargin: (Metrics.controlHeight - Metrics.iconSize) / 2
        anchors.verticalCenter: parent.verticalCenter
        width: Metrics.iconSize
        height: Metrics.iconSize
        source: Icons.add
        preferredRendererType: VectorImage.CurveRenderer
      }
    }
  }
}
