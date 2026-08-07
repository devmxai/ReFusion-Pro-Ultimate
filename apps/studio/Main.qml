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
    title: "ReFusion Studio — G0 Foundation"
    color: "#0b0d12"

    readonly property color panel: "#12151c"
    readonly property color panelRaised: "#181c25"
    readonly property color border: "#2a3040"
    readonly property color textMain: "#f2f4f8"
    readonly property color textMuted: "#8891a4"
    readonly property color accent: "#7c5cff"

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
                    anchors.centerIn: parent
                    width: Math.min(parent.width * 0.72, 820)
                    height: width * 0.5625
                    color: "#05060a"
                    border.color: "#343b4d"
                    border.width: 1

                    Column {
                        anchors.centerIn: parent
                        spacing: 10
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
                        Rectangle { Layout.fillWidth: true; color: "transparent" }
                        Label { text: "00:00:00:00"; color: root.textMuted }
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: root.panelRaised
                        border.color: root.border
                        Label {
                            anchors.centerIn: parent
                            text: "Accepted-revision snapshots only"
                            color: root.textMuted
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
                    Layout.preferredHeight: 90
                    color: root.panelRaised
                    border.color: root.border
                    Label {
                        anchors.fill: parent
                        anchors.margins: 10
                        text: "CONSOLE\nExternal Agent via files / CLI / MCP\nNo in-app Agent button by design"
                        color: root.textMuted
                        wrapMode: Text.Wrap
                    }
                }
            }
        }
    }
}

