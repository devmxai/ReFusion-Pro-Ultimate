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
                                       ? "PLAYING" : "STOPPED")
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
                Layout.preferredHeight: 230
                color: root.panel
                border.color: root.border

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8
                    RowLayout {
                        Label { text: "TIMELINE"; color: root.textMuted; font.bold: true }
                        Label {
                            text: engineViewportWindow
                                  ? engineViewportWindow.compositionName
                                    + "  •  "
                                    + engineViewportWindow.compositionWidth
                                    + "×" + engineViewportWindow.compositionHeight
                                  : "No composition"
                            color: root.textMuted
                        }
                        Rectangle { Layout.fillWidth: true; color: "transparent" }
                        Label {
                            text: engineViewportWindow
                                  ? root.playbackTime(engineViewportWindow.playbackPositionMs)
                                    + " / "
                                    + root.playbackTime(engineViewportWindow.playbackDurationMs)
                                    + "  •  Loop "
                                    + (engineViewportWindow.playbackLoop + 1)
                                  : "00:00 / 00:00"
                            color: engineViewportWindow
                                   && engineViewportWindow.playbackRunning
                                   ? "#7ee787" : root.textMuted
                        }
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: root.panelRaised
                        border.color: root.border

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 3

                            Repeater {
                                model: engineViewportWindow
                                       ? engineViewportWindow.layerNames : []
                                delegate: Rectangle {
                                    required property string modelData
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 20
                                    color: "#202532"
                                    border.color: root.border
                                    radius: 3

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 8
                                        anchors.rightMargin: 8
                                        Label {
                                            text: modelData
                                            color: root.textMain
                                            font.pixelSize: 10
                                        }
                                        Rectangle {
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 4
                                            radius: 2
                                            color: root.accent
                                        }
                                        Label {
                                            text: "30s"
                                            color: root.textMuted
                                            font.pixelSize: 9
                                        }
                                    }
                                }
                            }
                        }
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
                    Layout.preferredHeight: 128
                    color: root.panelRaised
                    border.color: root.border
                    Label {
                        anchors.fill: parent
                        anchors.margins: 10
                        text: engineViewportWindow
                              ? "PROJECT OPEN\n"
                                + engineViewportWindow.projectPath
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
