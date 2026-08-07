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
                              : engineViewportWindow.diagnostic
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
                                    Label {
                                        anchors.left: parent.left
                                        anchors.leftMargin: 9
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: "LAYERS"
                                        color: root.textMuted
                                        font.pixelSize: 9
                                        font.bold: true
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
                                    required property string layerId
                                    required property string displayName
                                    required property var startFrame
                                    required property var durationFrames
                                    width: timelineBody.width - 2
                                    height: 24

                                    Rectangle {
                                        width: timelineBody.trackLabelWidth
                                        height: parent.height
                                        color: index % 2 === 0 ? "#191e28" : "#171b24"
                                        border.color: root.border
                                        Label {
                                            anchors.left: parent.left
                                            anchors.leftMargin: 9
                                            anchors.right: parent.right
                                            anchors.rightMargin: 8
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: displayName
                                            color: root.textMain
                                            font.pixelSize: 10
                                            elide: Text.ElideRight
                                        }
                                        ToolTip.visible: trackHover.hovered
                                        ToolTip.text: layerId
                                        HoverHandler { id: trackHover }
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
                                            height: 12
                                            radius: 3
                                            color: root.accent
                                            border.color: "#987fff"
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
                    text: "Submit typed command"
                    highlighted: true
                    onClicked: studioBridge.submitRename(nameField.text)
                }
                Label {
                    Layout.fillWidth: true
                    visible: studioBridge.diagnostic.length > 0
                    text: studioBridge.diagnostic
                    color: "#ff6f7d"
                    wrapMode: Text.Wrap
                }
                Rectangle { Layout.fillHeight: true; color: "transparent" }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 180
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
