import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: root
    width: 1180
    height: 760
    minimumWidth: 940
    minimumHeight: 650
    visible: true
    title: "ReFusion — Project Launcher"
    color: "#0b0d12"

    readonly property color panel: "#12151c"
    readonly property color panelRaised: "#181c25"
    readonly property color border: "#2a3040"
    readonly property color textMain: "#f2f4f8"
    readonly property color textMuted: "#8891a4"
    readonly property color accent: "#7c5cff"

    property int selectedPresetIndex: 0
    property int selectedResolutionIndex: 0
    property int selectedFrameRate: 30
    readonly property var presetOptions: projectLauncher.presets
    readonly property var currentPreset:
        presetOptions.length > selectedPresetIndex
        ? presetOptions[selectedPresetIndex] : null
    readonly property var currentResolution:
        currentPreset && currentPreset.resolutions.length > selectedResolutionIndex
        ? currentPreset.resolutions[selectedResolutionIndex] : null

    FolderDialog {
        id: projectFolderDialog
        title: "Select or create an empty project folder"
        acceptLabel: "Use This Folder"
        onAccepted: {
            projectLauncher.createProject(
                        projectName.text,
                        root.currentPreset.id,
                        root.currentResolution.id,
                        root.selectedFrameRate,
                        durationSeconds.value,
                        selectedFolder)
        }
    }

    FolderDialog {
        id: openProjectDialog
        title: "Select a ReFusion project folder"
        acceptLabel: "Open Project"
        onAccepted: projectLauncher.openProjectWorkspace(selectedFolder)
    }

    header: Rectangle {
        height: 64
        color: root.panel
        border.color: root.border

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 22
            anchors.rightMargin: 22

            Label {
                text: "ReFusion"
                color: root.textMain
                font.pixelSize: 21
                font.bold: true
            }
            Label {
                text: "PROJECT LAUNCHER"
                color: root.textMuted
                font.pixelSize: 11
                font.letterSpacing: 1.2
            }
            Item { Layout.fillWidth: true }
            Button {
                text: "Open Existing Project"
                enabled: !projectLauncher.busy
                onClicked: openProjectDialog.open()
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 28
        spacing: 24

        Rectangle {
            Layout.preferredWidth: 280
            Layout.fillHeight: true
            color: root.panel
            radius: 12
            border.color: root.border

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 22
                spacing: 15

                Label {
                    text: "Create a real project"
                    color: root.textMain
                    font.pixelSize: 22
                    font.bold: true
                }
                Label {
                    Layout.fillWidth: true
                    text: "The engine creates Project.rfx, stable IDs, Agent instructions and a locked workspace."
                    color: root.textMuted
                    wrapMode: Text.Wrap
                    lineHeight: 1.2
                }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: root.border
                }
                Label {
                    text: "Project name"
                    color: root.textMuted
                }
                TextField {
                    id: projectName
                    Layout.fillWidth: true
                    placeholderText: "My ReFusion Project"
                    selectByMouse: true
                    maximumLength: 120
                }
                Label {
                    text: "Duration"
                    color: root.textMuted
                }
                RowLayout {
                    Layout.fillWidth: true
                    SpinBox {
                        id: durationSeconds
                        from: 1
                        to: 3600
                        value: 30
                        editable: true
                        Layout.fillWidth: true
                    }
                    Label {
                        text: "seconds"
                        color: root.textMuted
                    }
                }
                Item { Layout.fillHeight: true }
                Label {
                    Layout.fillWidth: true
                    text: "After Create, select or make an empty folder in the native system dialog. Project.rfx is committed last and reopened through Core."
                    color: root.textMuted
                    wrapMode: Text.Wrap
                    font.pixelSize: 11
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: root.panel
            radius: 12
            border.color: root.border

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 14

                Label {
                    text: "Composition"
                    color: root.textMain
                    font.pixelSize: 18
                    font.bold: true
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 4
                    columnSpacing: 10
                    rowSpacing: 10

                    Repeater {
                        model: root.presetOptions
                        delegate: Rectangle {
                            id: presetCard
                            required property int index
                            required property var modelData
                            Layout.fillWidth: true
                            Layout.preferredHeight: 150
                            radius: 10
                            color: root.selectedPresetIndex === index
                                   ? "#221d3f" : root.panelRaised
                            border.width: root.selectedPresetIndex === index ? 2 : 1
                            border.color: root.selectedPresetIndex === index
                                          ? root.accent : root.border

                            Column {
                                anchors.centerIn: parent
                                spacing: 10
                                Rectangle {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    width: modelData.id === "reels-9x16" ? 34
                                           : modelData.id === "portrait-4x5" ? 45
                                           : modelData.id === "cinematic-239x100" ? 82
                                           : 72
                                    height: modelData.id === "reels-9x16" ? 61
                                            : modelData.id === "portrait-4x5" ? 56
                                            : modelData.id === "cinematic-239x100" ? 34
                                            : 41
                                    radius: 3
                                    color: "#090b10"
                                    border.color: root.selectedPresetIndex === index
                                                  ? "#aa98ff" : "#596175"
                                }
                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: modelData.name
                                    color: root.textMain
                                    font.bold: true
                                }
                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: modelData.aspect
                                    color: root.textMuted
                                    font.pixelSize: 11
                                }
                            }

                            TapHandler {
                                onTapped: {
                                    root.selectedPresetIndex = index
                                    root.selectedResolutionIndex = 0
                                }
                            }
                        }
                    }
                }

                Label {
                    text: "Resolution"
                    color: root.textMuted
                    font.bold: true
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Repeater {
                        model: root.currentPreset ? root.currentPreset.resolutions : []
                        delegate: Button {
                            required property int index
                            required property var modelData
                            Layout.fillWidth: true
                            checkable: true
                            checked: root.selectedResolutionIndex === index
                            text: modelData.name + "\n" + modelData.width + "×" + modelData.height
                            onClicked: root.selectedResolutionIndex = index
                        }
                    }
                }

                Label {
                    text: "Frame rate"
                    color: root.textMuted
                    font.bold: true
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Repeater {
                        model: projectLauncher.frameRates
                        delegate: Button {
                            required property var modelData
                            Layout.fillWidth: true
                            checkable: true
                            checked: root.selectedFrameRate === Number(modelData)
                            text: modelData + " fps"
                            onClicked: root.selectedFrameRate = Number(modelData)
                        }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    visible: projectLauncher.diagnostic.length > 0
                    text: projectLauncher.diagnostic
                    color: "#ff6f7d"
                    wrapMode: Text.Wrap
                }

                Item { Layout.fillHeight: true }

                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 48
                    highlighted: true
                    enabled: projectName.text.trim().length > 0
                             && root.currentPreset !== null
                             && root.currentResolution !== null
                             && !projectLauncher.busy
                    text: projectLauncher.busy
                          ? "Creating project…"
                          : "Create Project and Choose Folder"
                    onClicked: projectFolderDialog.open()
                }
            }
        }
    }
}
