import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 1440
    height: 900
    minimumWidth: 1100
    minimumHeight: 680
    visible: true
    title: "ReFusion Studio — Open Project"
    color: "#0b0d12"

    readonly property color panel: "#12151c"
    readonly property color panelRaised: "#181c25"
    readonly property color border: "#2a3040"
    readonly property color textMain: "#f2f4f8"
    readonly property color textMuted: "#8891a4"
    readonly property color accent: "#7c5cff"

    function playbackTime(milliseconds) {
        const totalSeconds = Math.floor(milliseconds / 1000)
        const minutes = Math.floor(totalSeconds / 60)
        const seconds = totalSeconds % 60
        return String(minutes).padStart(2, "0") + ":"
                + String(seconds).padStart(2, "0")
    }

    function syncTransformInspector() {
        if (!studioBridge.hasVisualSelection) {
            positionXField.text = ""
            positionYField.text = ""
            anchorXField.text = ""
            anchorYField.text = ""
            scaleXField.text = ""
            scaleYField.text = ""
            rotationField.text = ""
            opacityField.text = ""
            return
        }
        positionXField.text = Number(studioBridge.selectedPositionX).toString()
        positionYField.text = Number(studioBridge.selectedPositionY).toString()
        anchorXField.text = Number(studioBridge.selectedAnchorX).toString()
        anchorYField.text = Number(studioBridge.selectedAnchorY).toString()
        scaleXField.text = Number(studioBridge.selectedScaleX).toString()
        scaleYField.text = Number(studioBridge.selectedScaleY).toString()
        rotationField.text = Number(studioBridge.selectedRotation).toString()
        opacityField.text = Number(studioBridge.selectedOpacity).toString()
    }

    function measuredRectText(rect) {
        if (!rect) {
            return "Unavailable"
        }
        return "L " + Number(rect.left).toFixed(2)
                + "  T " + Number(rect.top).toFixed(2)
                + "  R " + Number(rect.right).toFixed(2)
                + "  B " + Number(rect.bottom).toFixed(2)
    }

    Component.onCompleted: syncTransformInspector()
    Connections {
        target: studioBridge
        function onSnapshotChanged() { root.syncTransformInspector() }
    }

    header: Rectangle {
        height: 52
        color: root.panel
        border.color: root.border

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 12

            Label {
                text: "ReFusion"
                color: root.textMain
                font.pixelSize: 18
                font.bold: true
            }
            Rectangle { Layout.fillWidth: true; color: "transparent" }
            Label {
                text: studioBridge.projectName + "  •  Revision " + studioBridge.revision
                color: root.textMuted
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Rectangle {
            Layout.preferredWidth: 74
            Layout.fillHeight: true
            color: root.panel
            border.color: root.border

            ColumnLayout {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 14
                spacing: 8

                Repeater {
                    model: ["VID", "IMG", "TXT", "BG", "SHP", "AUD", "SVG"]
                    delegate: ToolButton {
                        required property string modelData
                        text: modelData
                        Layout.preferredWidth: 54
                        Layout.preferredHeight: 44
                        ToolTip.visible: hovered
                        ToolTip.text: "Command surface: " + modelData
                        onClicked: studioBridge.addVisualLayer(modelData)
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 1

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#090b10"
                border.color: root.border

                Rectangle {
                    id: viewportFrame
                    anchors.centerIn: parent
                    readonly property real compositionAspect:
                        engineViewportWindow
                        ? engineViewportWindow.compositionWidth
                          / engineViewportWindow.compositionHeight
                        : 16 / 9
                    width: Math.min(parent.width * 0.78,
                                    parent.height * 0.80 * compositionAspect)
                    height: width / compositionAspect
                    color: "#05060a"
                    border.color: "#343b4d"
                    border.width: 1

                    WindowContainer {
                        anchors.fill: parent
                        anchors.margins: 1
                        visible: engineViewportAvailable
                        window: engineViewportWindow
                    }

                    Column {
                        anchors.centerIn: parent
                        spacing: 10
                        visible: !engineViewportAvailable
                        Label {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "Native GPU Viewport Boundary"
                            color: root.textMain
                            font.pixelSize: 22
                            font.bold: true
                        }
                        Label {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "Reserved for the engine presenter — no QML Canvas or CPU video path"
                            color: root.textMuted
                        }
                    }

                    Rectangle {
                        id: viewportStatusBadge
                        anchors.left: parent.left
                        anchors.top: parent.bottom
                        anchors.topMargin: 8
                        width: viewportStatus.implicitWidth + 18
                        height: 28
                        radius: 8
                        color: "#cc12151c"
                        border.color: root.border
                        visible: engineViewportAvailable

                        Label {
                            id: viewportStatus
                            anchors.centerIn: parent
                            text: engineViewportWindow
                                  ? engineViewportWindow.adapterName
                                    + "  •  GPU frames "
                                    + engineViewportWindow.presentedFrames
                                    + "  •  "
                                    + (engineViewportWindow.playbackRunning
                                       ? "PLAYING" : "PAUSED")
                                    + "  •  GPU "
                                    + engineViewportWindow.deviceStatus
                                  : "GPU viewport unavailable"
                            color: engineViewportWindow && engineViewportWindow.zeroCopy
                                   ? "#7ee787" : "#ff6f7d"
                            font.pixelSize: 11
                        }
                    }

                    Label {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: viewportStatusBadge.bottom
                        anchors.topMargin: 4
                        visible: engineViewportDiagnostic.length > 0
                                 || (engineViewportWindow
                                     && engineViewportWindow.diagnostic.length > 0)
                        text: engineViewportDiagnostic.length > 0
                              ? engineViewportDiagnostic
                              : (engineViewportWindow
                                 ? engineViewportWindow.diagnostic : "")
                        color: "#ff6f7d"
                        wrapMode: Text.Wrap
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 260
                color: root.panel
                border.color: root.border

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8
                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34

                        Row {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 8
                            Label {
                                text: "TIMELINE"
                                color: root.textMuted
                                font.bold: true
                            }
                            Label {
                                text: engineViewportWindow
                                      ? engineViewportWindow.compositionName
                                        + "  •  "
                                        + engineViewportWindow.compositionWidth
                                        + "×" + engineViewportWindow.compositionHeight
                                      : "No composition"
                                color: root.textMuted
                            }
                        }

                        ToolButton {
                            id: playbackToggle
                            anchors.centerIn: parent
                            width: 44
                            height: 30
                            enabled: transportBridge !== null
                            text: transportBridge && transportBridge.running
                                  ? "❚❚" : "▶"
                            font.pixelSize: 15
                            font.bold: true
                            onClicked: transportBridge.togglePlayback()
                            ToolTip.visible: hovered
                            ToolTip.text: transportBridge && transportBridge.running
                                          ? "Pause at current frame"
                                          : "Play from current frame"
                            background: Rectangle {
                                radius: 7
                                color: playbackToggle.down
                                       ? "#6548df"
                                       : playbackToggle.hovered
                                         ? "#2a3040" : root.panelRaised
                                border.color: playbackToggle.enabled
                                              ? root.accent : root.border
                            }
                            contentItem: Label {
                                text: playbackToggle.text
                                color: playbackToggle.enabled
                                       ? root.textMain : root.textMuted
                                font: playbackToggle.font
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }

                        Row {
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 10
                            Label {
                                text: transportBridge
                                      ? transportBridge.positionTimecode
                                        + " / "
                                        + transportBridge.durationTimecode
                                      : "00:00:00:00 / 00:00:00:00"
                                color: transportBridge && transportBridge.running
                                       ? "#7ee787" : root.textMuted
                                font.family: "Menlo"
                                font.pixelSize: 11
                            }
                            Label {
                                text: engineViewportWindow
                                      ? "Loop " + (engineViewportWindow.playbackLoop + 1)
                                      : "Loop 1"
                                color: root.textMuted
                                font.pixelSize: 10
                            }
                        }
                    }

                    Rectangle {
                        id: timelineBody
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: root.panelRaised
                        border.color: root.border

                        readonly property real trackLabelWidth: 150
                        readonly property int rulerTicks: 7

                        Column {
                            anchors.fill: parent
                            anchors.margins: 1
                            spacing: 0

                            Item {
                                width: parent.width
                                height: 25

                                Rectangle {
                                    width: timelineBody.trackLabelWidth
                                    height: parent.height
                                    color: "#171b24"
                                    border.color: root.border

                                    Row {
                                        anchors.fill: parent
                                        anchors.leftMargin: 4
                                        anchors.rightMargin: 6
                                        spacing: 3

                                        ToolButton {
                                            width: 24
                                            height: parent.height
                                            enabled: transportBridge
                                                     && transportBridge.canNavigateUp
                                            text: "‹"
                                            font.pixelSize: 16
                                            onClicked: transportBridge.navigateUp()
                                            ToolTip.visible: hovered
                                            ToolTip.text: "Up one group"
                                        }

                                        Label {
                                            width: parent.width - 27
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: transportBridge
                                                  ? transportBridge.timelinePath
                                                  : "LAYERS"
                                            color: root.textMuted
                                            font.pixelSize: 9
                                            font.bold: true
                                            elide: Text.ElideLeft
                                        }
                                    }
                                }

                                Item {
                                    id: rulerLane
                                    x: timelineBody.trackLabelWidth
                                    width: parent.width - x
                                    height: parent.height

                                    Repeater {
                                        model: timelineBody.rulerTicks
                                        delegate: Item {
                                            required property int index
                                            x: index * (rulerLane.width - 1)
                                               / (timelineBody.rulerTicks - 1)
                                            width: 1
                                            height: rulerLane.height
                                            Rectangle {
                                                anchors.bottom: parent.bottom
                                                width: 1
                                                height: index % 2 === 0 ? 9 : 6
                                                color: root.border
                                            }
                                            Label {
                                                anchors.top: parent.top
                                                x: index === 0 ? 4
                                                   : index === timelineBody.rulerTicks - 1
                                                     ? -implicitWidth - 4
                                                     : -implicitWidth / 2
                                                text: transportBridge
                                                      ? transportBridge.timecodeAtRatio(
                                                            index / (timelineBody.rulerTicks - 1))
                                                      : "00:00:00:00"
                                                color: root.textMuted
                                                font.family: "Menlo"
                                                font.pixelSize: 8
                                            }
                                        }
                                    }
                                }
                            }

                            Repeater {
                                model: transportBridge ? transportBridge.tracks : null
                                delegate: Item {
                                    required property int index
                                    required property string nodeId
                                    required property string displayName
                                    required property var startFrame
                                    required property var durationFrames
                                    required property string nodeKind
                                    required property bool isGroup
                                    required property var childCount
                                    required property string ownerNodeId
                                    required property bool ownerIsGroup
                                    required property int depth
                                    required property bool isPropertyRow
                                    readonly property bool selected:
                                        !isPropertyRow
                                        && studioBridge.selectedNodeId === nodeId
                                    readonly property bool ownerSelected:
                                        studioBridge.selectedNodeId === ownerNodeId
                                    width: timelineBody.width - 2
                                    height: 24

                                    Rectangle {
                                        width: timelineBody.trackLabelWidth
                                        height: parent.height
                                        color: selected ? "#30275a"
                                                       : isPropertyRow && ownerSelected
                                                         ? "#24203a"
                                                       : isGroup ? "#211d36"
                                                       : index % 2 === 0
                                                         ? "#191e28" : "#171b24"
                                        border.color: selected ? root.accent : root.border

                                        Label {
                                            anchors.left: parent.left
                                            anchors.leftMargin: 8 + depth * 12
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: isGroup ? "▸"
                                                  : nodeKind === "effect" ? "ƒ"
                                                  : nodeKind === "mask" ? "◩"
                                                  : nodeKind === "animation" ? "◆"
                                                  : ""
                                            color: root.accent
                                            font.pixelSize: 11
                                        }
                                        Label {
                                            anchors.left: parent.left
                                            anchors.leftMargin: 9 + depth * 12
                                                                + (isGroup
                                                                   || isPropertyRow
                                                                   ? 15 : 0)
                                            anchors.right: parent.right
                                            anchors.rightMargin: 8
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: isGroup
                                                  ? displayName + "  (" + childCount + ")"
                                                  : displayName
                                            color: isGroup ? "#c7baff"
                                                           : isPropertyRow
                                                             ? "#aeb6c7"
                                                             : root.textMain
                                            font.pixelSize: isPropertyRow ? 9 : 10
                                            font.bold: isGroup
                                            elide: Text.ElideRight
                                        }
                                        ToolTip.visible: trackHover.hovered
                                        ToolTip.text: isGroup
                                                      ? nodeId + " — double-click to open"
                                                      : isPropertyRow
                                                        ? nodeId + " — owned by "
                                                          + ownerNodeId
                                                        : nodeId
                                        HoverHandler { id: trackHover }
                                        MouseArea {
                                            anchors.fill: parent
                                            acceptedButtons: Qt.LeftButton
                                            onClicked: studioBridge.selectVisualNode(
                                                           ownerNodeId,
                                                           ownerIsGroup)
                                            onDoubleClicked: function(mouse) {
                                                if (isGroup && !isPropertyRow) {
                                                    transportBridge.enterGroup(nodeId)
                                                }
                                            }
                                        }
                                    }

                                    Item {
                                        id: trackLane
                                        x: timelineBody.trackLabelWidth
                                        width: parent.width - x
                                        height: parent.height

                                        Rectangle {
                                            anchors.fill: parent
                                            color: index % 2 === 0 ? "#151923" : "#131720"
                                            border.color: "#222837"
                                        }

                                        Rectangle {
                                            x: transportBridge && transportBridge.durationFrames > 0
                                               ? Number(startFrame)
                                                 / transportBridge.durationFrames
                                                 * trackLane.width : 0
                                            width: transportBridge
                                                   && transportBridge.durationFrames > 0
                                                   ? Math.max(2,
                                                       Number(durationFrames)
                                                       / transportBridge.durationFrames
                                                       * trackLane.width)
                                                   : 0
                                            anchors.verticalCenter: parent.verticalCenter
                                            height: isPropertyRow ? 6 : 12
                                            radius: 3
                                            color: selected ? "#9d87ff"
                                                            : isPropertyRow
                                                              ? nodeKind === "effect"
                                                                ? "#ff8a65"
                                                                : nodeKind === "mask"
                                                                  ? "#55d6be"
                                                                  : "#f4c95d"
                                                            : isGroup ? "#5f46bf"
                                                                      : root.accent
                                            border.color: selected ? "#ffffff"
                                                                   : isPropertyRow
                                                                     ? "#343c50"
                                                                   : isGroup ? "#b8a9ff"
                                                                             : "#987fff"
                                        }
                                    }
                                }
                            }
                        }

                        Item {
                            id: playheadLane
                            x: timelineBody.trackLabelWidth + 1
                            y: 1
                            width: timelineBody.width - x - 1
                            height: timelineBody.height - 2
                            z: 20

                            Rectangle {
                                id: playheadLine
                                x: transportBridge
                                   ? Math.round(transportBridge.positionRatio
                                                * (playheadLane.width - width))
                                   : 0
                                y: 18
                                width: 2
                                height: playheadLane.height - y
                                color: "#47d7ff"

                                Rectangle {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.bottom: parent.top
                                    width: 11
                                    height: 9
                                    radius: 2
                                    color: "#47d7ff"
                                    border.color: "#b7f2ff"
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                enabled: transportBridge !== null
                                hoverEnabled: true
                                cursorShape: pressed
                                             ? Qt.ClosedHandCursor
                                             : Qt.SplitHCursor
                                onPressed: function(mouse) {
                                    transportBridge.seekFromTimelinePosition(
                                                mouse.x, width)
                                }
                                onPositionChanged: function(mouse) {
                                    if (pressed) {
                                        transportBridge.seekFromTimelinePosition(
                                                    mouse.x, width)
                                    }
                                }
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        visible: transportBridge
                                 && transportBridge.diagnostic.length > 0
                        text: transportBridge ? transportBridge.diagnostic : ""
                        color: "#ff6f7d"
                        font.pixelSize: 10
                        elide: Text.ElideRight
                    }
                }
            }
        }

        Rectangle {
            Layout.preferredWidth: 320
            Layout.fillHeight: true
            color: root.panel
            border.color: root.border

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                Label {
                    text: "INSPECTOR"
                    color: root.textMuted
                    font.bold: true
                }

                ScrollView {
                    id: inspectorScroll
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    contentWidth: availableWidth

                    ColumnLayout {
                        width: inspectorScroll.availableWidth
                        spacing: 8

                        Label { text: "Project ID"; color: root.textMuted }
                        TextField {
                            Layout.fillWidth: true
                            text: studioBridge.projectId
                            readOnly: true
                        }
                        Label { text: "Project name command"; color: root.textMuted }
                        TextField {
                            id: nameField
                            Layout.fillWidth: true
                            text: studioBridge.projectName
                            selectByMouse: true
                        }
                        Button {
                            Layout.fillWidth: true
                            text: "Submit project command"
                            onClicked: studioBridge.submitRename(nameField.text)
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: root.border
                        }
                        Label {
                            text: studioBridge.hasVisualSelection
                                  ? studioBridge.selectedNodeKind.toUpperCase()
                                    + " TRANSFORM"
                                  : "LAYER TRANSFORM"
                            color: root.textMuted
                            font.bold: true
                        }
                        Label {
                            Layout.fillWidth: true
                            text: studioBridge.hasVisualSelection
                                  ? studioBridge.selectedDisplayName
                                    + "\n" + studioBridge.selectedNodeId
                                  : "Select a Layer or Group on the Timeline"
                            color: studioBridge.hasVisualSelection
                                   ? root.textMain : root.textMuted
                            wrapMode: Text.Wrap
                            elide: Text.ElideRight
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            columnSpacing: 8
                            rowSpacing: 6
                            enabled: studioBridge.hasVisualSelection

                            Label { text: "Position X"; color: root.textMuted }
                            TextField {
                                id: positionXField
                                Layout.fillWidth: true
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                            }
                            Label { text: "Position Y"; color: root.textMuted }
                            TextField {
                                id: positionYField
                                Layout.fillWidth: true
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                            }
                            Label { text: "Anchor X"; color: root.textMuted }
                            TextField {
                                id: anchorXField
                                Layout.fillWidth: true
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                            }
                            Label { text: "Anchor Y"; color: root.textMuted }
                            TextField {
                                id: anchorYField
                                Layout.fillWidth: true
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                            }
                            Label { text: "Scale X"; color: root.textMuted }
                            TextField {
                                id: scaleXField
                                Layout.fillWidth: true
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                            }
                            Label { text: "Scale Y"; color: root.textMuted }
                            TextField {
                                id: scaleYField
                                Layout.fillWidth: true
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                            }
                            Label { text: "Rotation °"; color: root.textMuted }
                            TextField {
                                id: rotationField
                                Layout.fillWidth: true
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                            }
                            Label { text: "Opacity 0–1"; color: root.textMuted }
                            TextField {
                                id: opacityField
                                Layout.fillWidth: true
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                            }
                        }

                        Button {
                            Layout.fillWidth: true
                            highlighted: true
                            enabled: studioBridge.hasVisualSelection
                            text: "Apply Transform Command"
                            onClicked: studioBridge.submitSelectedTransform(
                                           Number(positionXField.text),
                                           Number(positionYField.text),
                                           Number(anchorXField.text),
                                           Number(anchorYField.text),
                                           Number(scaleXField.text),
                                           Number(scaleYField.text),
                                           Number(rotationField.text),
                                           Number(opacityField.text))
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: root.border
                            visible: studioBridge.hasVisualSelection
                        }
                        Label {
                            text: "MEASURED ALIGNMENT"
                            color: root.textMuted
                            font.bold: true
                            visible: studioBridge.hasVisualSelection
                        }
                        Label {
                            Layout.fillWidth: true
                            visible: studioBridge.hasVisualSelection
                            text: studioBridge.selectedMeasuredBounds.available
                                  ? "Exact time: "
                                    + studioBridge.selectedMeasuredBounds.timeNs
                                    + " ns"
                                  : String(studioBridge.selectedMeasuredBounds.code
                                           || "Measurement unavailable")
                            color: studioBridge.selectedMeasuredBounds.available
                                   ? root.textMuted : "#ff8a96"
                            font.pixelSize: 9
                            wrapMode: Text.Wrap
                        }
                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            visible: studioBridge.hasVisualSelection

                            Label { text: "Geometry"; color: root.textMuted }
                            Label {
                                Layout.fillWidth: true
                                text: root.measuredRectText(
                                          studioBridge.selectedMeasuredBounds.geometry)
                                color: root.textMain
                                font.family: "Menlo"
                                font.pixelSize: 8
                            }
                            Label { text: "Logical"; color: root.textMuted }
                            Label {
                                Layout.fillWidth: true
                                text: studioBridge.selectedMeasuredBounds.logicalAvailable
                                      ? root.measuredRectText(
                                            studioBridge.selectedMeasuredBounds.logical)
                                      : "Unavailable"
                                color: root.textMain
                                font.family: "Menlo"
                                font.pixelSize: 8
                            }
                            Label { text: "Ink"; color: root.textMuted }
                            Label {
                                Layout.fillWidth: true
                                text: studioBridge.selectedMeasuredBounds.inkAvailable
                                      ? root.measuredRectText(
                                            studioBridge.selectedMeasuredBounds.ink)
                                      : "Unavailable"
                                color: root.textMain
                                font.family: "Menlo"
                                font.pixelSize: 8
                            }
                        }
                        ComboBox {
                            id: alignmentTarget
                            Layout.fillWidth: true
                            visible: studioBridge.hasVisualSelection
                            model: studioBridge.alignmentTargets
                            textRole: "displayName"
                            valueRole: "id"
                            displayText: currentIndex >= 0
                                         ? "Target · " + currentText
                                         : "No active target"
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            visible: studioBridge.hasVisualSelection
                            ComboBox {
                                id: horizontalAlignment
                                Layout.fillWidth: true
                                model: ["None", "Left", "Center", "Right"]
                                currentIndex: 2
                            }
                            ComboBox {
                                id: verticalAlignment
                                Layout.fillWidth: true
                                model: ["None", "Top", "Center", "Bottom"]
                                currentIndex: 2
                            }
                            ComboBox {
                                id: alignmentBasis
                                Layout.fillWidth: true
                                model: ["Geometry", "Logical", "Ink"]
                            }
                        }
                        Button {
                            Layout.fillWidth: true
                            visible: studioBridge.hasVisualSelection
                            enabled: alignmentTarget.currentIndex >= 0
                                     && (horizontalAlignment.currentIndex > 0
                                         || verticalAlignment.currentIndex > 0)
                            text: "Align at Current Project Time"
                            onClicked: {
                                const target = alignmentTarget.model[
                                                 alignmentTarget.currentIndex]
                                studioBridge.submitSelectedAlignment(
                                            String(target.id),
                                            Boolean(target.isGroup),
                                            horizontalAlignment.currentText,
                                            verticalAlignment.currentText,
                                            alignmentBasis.currentText)
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: root.border
                            visible: studioBridge.hasVisualSelection
                        }
                        Label {
                            text: "CONTENT / APPEARANCE"
                            color: root.textMuted
                            font.bold: true
                            visible: studioBridge.hasVisualSelection
                        }
                        Repeater {
                            model: studioBridge.selectedProperties
                            delegate: RowLayout {
                                required property var modelData
                                readonly property bool propertyVisible:
                                    !modelData.id.startsWith("transform.")
                                    && modelData.kind !== "paint"
                                Layout.fillWidth: true
                                Layout.preferredHeight: propertyVisible ? 38 : 0
                                spacing: 6
                                visible: propertyVisible

                                Label {
                                    Layout.preferredWidth: 92
                                    text: modelData.label
                                          + (modelData.unit === "text"
                                             || modelData.unit === "boolean"
                                             || modelData.unit === "rgba8"
                                             ? "" : " (" + modelData.unit + ")")
                                    color: root.textMuted
                                    font.pixelSize: 10
                                    elide: Text.ElideRight
                                }
                                TextField {
                                    id: propertyEditor
                                    Layout.fillWidth: true
                                    text: String(modelData.value)
                                    readOnly: !modelData.writable
                                    selectByMouse: true
                                    placeholderText: modelData.kind === "color"
                                                     ? "#RRGGBBAA"
                                                     : modelData.kind === "boolean"
                                                       ? "true / false" : ""
                                }
                                Button {
                                    text: "Apply"
                                    enabled: modelData.writable
                                    onClicked: studioBridge.submitSelectedProperty(
                                                   modelData.id,
                                                   propertyEditor.text)
                                }
                            }
                        }
                        Label {
                            text: "SHAPE FILL"
                            color: root.textMuted
                            font.bold: true
                            visible: studioBridge.selectedNodeKind === "Shape"
                        }
                        ComboBox {
                            id: shapeFillKind
                            Layout.fillWidth: true
                            visible: studioBridge.selectedNodeKind === "Shape"
                            model: ["Solid", "Linear Gradient", "Radial Gradient"]
                            currentIndex: studioBridge.selectedShapeFill.kind
                                          === "linear_gradient" ? 1
                                          : studioBridge.selectedShapeFill.kind
                                            === "radial_gradient" ? 2 : 0
                        }
                        GridLayout {
                            Layout.fillWidth: true
                            visible: studioBridge.selectedNodeKind === "Shape"
                            columns: 2
                            Label {
                                text: shapeFillKind.currentIndex === 0
                                      ? "Color" : "Color A"
                                color: root.textMuted
                            }
                            TextField {
                                id: shapeFillColorA
                                Layout.fillWidth: true
                                text: shapeFillKind.currentIndex === 0
                                      ? String(studioBridge.selectedShapeFill.color
                                               || "#7C5CFFFF")
                                      : String(studioBridge.selectedShapeFill.colorA
                                               || "#7C5CFFFF")
                                placeholderText: "#RRGGBBAA"
                            }
                            Label {
                                visible: shapeFillKind.currentIndex !== 0
                                text: "Color B"
                                color: root.textMuted
                            }
                            TextField {
                                id: shapeFillColorB
                                visible: shapeFillKind.currentIndex !== 0
                                Layout.fillWidth: true
                                text: String(studioBridge.selectedShapeFill.colorB
                                             || "#20D0FFFF")
                                placeholderText: "#RRGGBBAA"
                            }
                            Label {
                                visible: shapeFillKind.currentIndex === 1
                                text: "Start X / Y"
                                color: root.textMuted
                            }
                            RowLayout {
                                visible: shapeFillKind.currentIndex === 1
                                Layout.fillWidth: true
                                TextField {
                                    id: linearStartX
                                    Layout.fillWidth: true
                                    text: String(studioBridge.selectedShapeFill.startX
                                                 !== undefined
                                                 ? studioBridge.selectedShapeFill.startX
                                                 : -540)
                                }
                                TextField {
                                    id: linearStartY
                                    Layout.fillWidth: true
                                    text: String(studioBridge.selectedShapeFill.startY
                                                 !== undefined
                                                 ? studioBridge.selectedShapeFill.startY
                                                 : -960)
                                }
                            }
                            Label {
                                visible: shapeFillKind.currentIndex === 1
                                text: "End X / Y"
                                color: root.textMuted
                            }
                            RowLayout {
                                visible: shapeFillKind.currentIndex === 1
                                Layout.fillWidth: true
                                TextField {
                                    id: linearEndX
                                    Layout.fillWidth: true
                                    text: String(studioBridge.selectedShapeFill.endX
                                                 !== undefined
                                                 ? studioBridge.selectedShapeFill.endX
                                                 : 540)
                                }
                                TextField {
                                    id: linearEndY
                                    Layout.fillWidth: true
                                    text: String(studioBridge.selectedShapeFill.endY
                                                 !== undefined
                                                 ? studioBridge.selectedShapeFill.endY
                                                 : 960)
                                }
                            }
                            Label {
                                visible: shapeFillKind.currentIndex === 2
                                text: "Center X / Y"
                                color: root.textMuted
                            }
                            RowLayout {
                                visible: shapeFillKind.currentIndex === 2
                                Layout.fillWidth: true
                                TextField {
                                    id: radialCenterX
                                    Layout.fillWidth: true
                                    text: String(studioBridge.selectedShapeFill.centerX || 0)
                                }
                                TextField {
                                    id: radialCenterY
                                    Layout.fillWidth: true
                                    text: String(studioBridge.selectedShapeFill.centerY || 0)
                                }
                            }
                            Label {
                                visible: shapeFillKind.currentIndex === 2
                                text: "Radius"
                                color: root.textMuted
                            }
                            TextField {
                                id: radialRadius
                                visible: shapeFillKind.currentIndex === 2
                                Layout.fillWidth: true
                                text: String(studioBridge.selectedShapeFill.radius || 720)
                            }
                        }
                        Button {
                            Layout.fillWidth: true
                            highlighted: true
                            visible: studioBridge.selectedNodeKind === "Shape"
                            text: "Apply Fill Command"
                            onClicked: {
                                var parameters = {}
                                if (shapeFillKind.currentIndex === 0) {
                                    parameters.color = shapeFillColorA.text
                                    studioBridge.submitSelectedShapeFill(
                                                "solid", parameters)
                                } else if (shapeFillKind.currentIndex === 1) {
                                    parameters.colorA = shapeFillColorA.text
                                    parameters.colorB = shapeFillColorB.text
                                    parameters.startX = linearStartX.text
                                    parameters.startY = linearStartY.text
                                    parameters.endX = linearEndX.text
                                    parameters.endY = linearEndY.text
                                    studioBridge.submitSelectedShapeFill(
                                                "linear_gradient", parameters)
                                } else {
                                    parameters.colorA = shapeFillColorA.text
                                    parameters.colorB = shapeFillColorB.text
                                    parameters.centerX = radialCenterX.text
                                    parameters.centerY = radialCenterY.text
                                    parameters.radius = radialRadius.text
                                    studioBridge.submitSelectedShapeFill(
                                                "radial_gradient", parameters)
                                }
                            }
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: root.border
                            visible: studioBridge.hasVisualSelection
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            visible: studioBridge.hasVisualSelection
                                     && studioBridge.selectedNodeKind !== "Group"
                            Label {
                                text: "MASK STACK"
                                color: root.textMuted
                                font.bold: true
                            }
                            Item { Layout.fillWidth: true }
                            Repeater {
                                model: studioBridge.availableMasks
                                delegate: Button {
                                    required property var modelData
                                    text: "+ " + modelData.label
                                    onClicked: studioBridge.addSelectedMask(
                                                   modelData.kind)
                                }
                            }
                        }
                        Repeater {
                            model: studioBridge.selectedMasks
                            delegate: Rectangle {
                                required property var modelData
                                Layout.fillWidth: true
                                Layout.preferredHeight: 105
                                                        + modelData.parameters.length * 38
                                color: root.panelRaised
                                border.color: root.border
                                radius: 6
                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    RowLayout {
                                        Layout.fillWidth: true
                                        CheckBox {
                                            id: maskEnabled
                                            checked: modelData.enabled
                                            text: modelData.label
                                        }
                                        CheckBox {
                                            id: maskInverted
                                            checked: modelData.inverted
                                            text: "Invert"
                                        }
                                        Item { Layout.fillWidth: true }
                                        Button {
                                            text: "Remove"
                                            flat: true
                                            onClicked: studioBridge.removeSelectedMask(
                                                           modelData.id)
                                        }
                                    }
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 4
                                        Repeater {
                                            id: maskParameterEditors
                                            model: modelData.parameters
                                            delegate: RowLayout {
                                                required property var modelData
                                                property string parameterId: modelData.id
                                                property string parameterText: maskParameterValue.text
                                                Layout.fillWidth: true
                                                Label {
                                                    Layout.preferredWidth: 120
                                                    text: modelData.label
                                                          + (modelData.unit.length > 0
                                                             ? " (" + modelData.unit + ")" : "")
                                                    color: root.textMuted
                                                    font.pixelSize: 10
                                                }
                                                TextField {
                                                    id: maskParameterValue
                                                    Layout.fillWidth: true
                                                    text: String(modelData.value)
                                                    placeholderText: modelData.kind === "color"
                                                                     ? "#RRGGBBAA" : ""
                                                }
                                            }
                                        }
                                    }
                                    Button {
                                        Layout.fillWidth: true
                                        highlighted: true
                                        text: "Apply Mask Command"
                                        onClicked: {
                                            var parameters = {}
                                            for (var index = 0;
                                                 index < maskParameterEditors.count;
                                                 ++index) {
                                                var editor = maskParameterEditors.itemAt(index)
                                                parameters[editor.parameterId]
                                                        = editor.parameterText
                                            }
                                            studioBridge.updateSelectedMask(
                                                        modelData.id,
                                                        maskEnabled.checked,
                                                        maskInverted.checked,
                                                        parameters)
                                        }
                                    }
                                }
                            }
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: root.border
                            visible: studioBridge.hasVisualSelection
                        }
                        Label {
                            text: "LOCAL FX STACK"
                            color: root.textMuted
                            font.bold: true
                            visible: studioBridge.hasVisualSelection
                        }
                        Label {
                            Layout.fillWidth: true
                            visible: studioBridge.hasVisualSelection
                                     && studioBridge.selectedNodeKind === "Group"
                            text: "Group FX requires isolated compositing and is intentionally unavailable in this slice."
                            color: root.textMuted
                            font.pixelSize: 10
                            wrapMode: Text.Wrap
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            visible: studioBridge.hasVisualSelection
                                     && studioBridge.selectedNodeKind !== "Group"
                            Repeater {
                                model: studioBridge.availableEffects
                                delegate: Button {
                                    required property var modelData
                                    Layout.fillWidth: true
                                    text: "+ " + modelData.label
                                    onClicked: studioBridge.addSelectedEffect(
                                                   modelData.kind)
                                }
                            }
                        }
                        Repeater {
                            model: studioBridge.selectedEffects
                            delegate: Rectangle {
                                required property var modelData
                                Layout.fillWidth: true
                                Layout.preferredHeight: 86
                                                        + modelData.parameters.length * 38
                                color: root.panelRaised
                                border.color: root.border
                                radius: 6

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    spacing: 5
                                    RowLayout {
                                        Layout.fillWidth: true
                                        CheckBox {
                                            id: effectEnabled
                                            checked: modelData.enabled
                                            text: modelData.label
                                        }
                                        Item { Layout.fillWidth: true }
                                        Button {
                                            text: "Remove"
                                            flat: true
                                            onClicked: studioBridge.removeSelectedEffect(
                                                           modelData.id)
                                        }
                                    }
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 4
                                        Repeater {
                                            id: effectParameterEditors
                                            model: modelData.parameters
                                            delegate: RowLayout {
                                                required property var modelData
                                                property string parameterId: modelData.id
                                                property string parameterText: effectParameterValue.text
                                                Layout.fillWidth: true
                                                Label {
                                                    Layout.preferredWidth: 120
                                                    text: modelData.label
                                                          + (modelData.unit.length > 0
                                                             ? " (" + modelData.unit + ")" : "")
                                                    color: root.textMuted
                                                    font.pixelSize: 10
                                                }
                                                TextField {
                                                    id: effectParameterValue
                                                    Layout.fillWidth: true
                                                    text: String(modelData.value)
                                                    placeholderText: modelData.kind === "color"
                                                                     ? "#RRGGBBAA" : ""
                                                }
                                            }
                                        }
                                    }
                                    Button {
                                        Layout.fillWidth: true
                                        highlighted: true
                                        text: "Apply FX Command"
                                        onClicked: {
                                            var parameters = {}
                                            for (var index = 0;
                                                 index < effectParameterEditors.count;
                                                 ++index) {
                                                var editor = effectParameterEditors.itemAt(index)
                                                parameters[editor.parameterId]
                                                        = editor.parameterText
                                            }
                                            studioBridge.updateSelectedEffect(
                                                        modelData.id,
                                                        effectEnabled.checked,
                                                        parameters)
                                        }
                                    }
                                }
                            }
                        }
                        Label {
                            Layout.fillWidth: true
                            visible: studioBridge.diagnostic.length > 0
                            text: studioBridge.diagnostic
                            color: "#ff6f7d"
                            wrapMode: Text.Wrap
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 160
                    color: root.panelRaised
                    border.color: root.border
                    Label {
                        anchors.fill: parent
                        anchors.margins: 10
                        text: engineViewportWindow
                              ? "PROJECT OPEN\n"
                                + engineViewportWindow.projectPath
                                + "\n\nGPU LIFECYCLE: "
                                + engineViewportWindow.deviceStatus
                                + "  events="
                                + engineViewportWindow.deviceEventSequence
                                + "\nVisibility suspend/resume: "
                                + engineViewportWindow.visibilitySuspends
                                + "/"
                                + engineViewportWindow.visibilityResumes
                                + "\nOcclusion suspend/resume: "
                                + engineViewportWindow.occlusionSuspends
                                + "/"
                                + engineViewportWindow.occlusionResumes
                                + "\nDevice-loss rejections: "
                                + engineViewportWindow.deviceLossRejections
                                + "\n\nExternal Agent: files / CLI / MCP"
                              : "PROJECT NOT OPEN"
                        color: root.textMuted
                        font.pixelSize: 10
                        wrapMode: Text.Wrap
                    }
                }
            }
        }
    }
}
