import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtMultimedia

Item {
    id: root

    // 由外层 Window 传入的可见性，用于控制全屏/退出全屏
    property int windowVisibility: 0

    property bool userIsSeeking: false
    property bool psMode: false

    function formatMs(ms) {
        if (!ms || ms < 0)
            ms = 0
        var totalSeconds = Math.floor(ms / 1000)
        var s = totalSeconds % 60
        var m = Math.floor(totalSeconds / 60) % 60
        var h = Math.floor(totalSeconds / 3600)
        function pad(n) { return (n < 10 ? "0" : "") + n }
        return (h > 0 ? (h + ":" + pad(m) + ":" + pad(s)) : (m + ":" + pad(s)))
    }

    FileDialog {
        id: openFileDialog
        title: qsTr("选择视频文件")
        nameFilters: [
            qsTr("视频文件 (*.ps *.mp4 *.mkv *.mov *.avi *.wmv *.flv *.webm)"),
            qsTr("所有文件 (*)")
        ]
        onAccepted: {
            const path = selectedFile ? selectedFile.toString().toLowerCase() : ""
            psMode = path.endsWith(".ps")
            if (psMode) {
                player.stop()
                const ok = PlaybackManager.playPsFile(selectedFile)
                if (!ok) {
                    errorText.text = qsTr("PS 文件播放失败")
                    errorPopup.open()
                }
            } else {
                player.source = selectedFile
                player.play()
            }
        }
    }

    MediaPlayer {
        id: player
        videoOutput: videoOutput
        audioOutput: AudioOutput {
            id: audioOut
            volume: volumeSlider.value
            muted: muteButton.checked
        }

        onErrorOccurred: (error, errorString) => {
            errorText.text = errorString
            errorPopup.open()
        }
    }

    Popup {
        id: errorPopup
        modal: true
        focus: true
        x: Math.max(0, (root.width - width) / 2)
        y: Math.max(0, (root.height - height) / 2)
        width: Math.min(520, root.width - 40)
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            Label { text: qsTr("播放出错"); font.bold: true }
            Label { id: errorText; text: ""; wrapMode: Text.Wrap }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                Button { text: qsTr("关闭"); onClicked: errorPopup.close() }
            }
        }
    }

    Shortcut { sequence: "Space"; onActivated: playPauseButton.clicked() }
    Shortcut { sequence: "F"; onActivated: fullscreenButton.clicked() }
    Shortcut {
        sequence: "Esc"
        onActivated: if (windowVisibility === Window.FullScreen)
                         windowVisibility = Window.Windowed
    }

    Rectangle {
        anchors.fill: parent
        color: "#101216"
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ToolBar {
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                spacing: 8

                ToolButton {
                    text: qsTr("打开")
                    onClicked: openFileDialog.open()
                }

                Label {
                    Layout.fillWidth: true
                    elide: Label.ElideMiddle
                    text: player.source && player.source.toString().length > 0
                          ? player.source.toString()
                          : qsTr("未选择文件")
                    color: "#E6E8EE"
                }

                ToolButton {
                    id: fullscreenButton
                    text: windowVisibility === Window.FullScreen ? qsTr("退出全屏") : qsTr("全屏")
                    onClicked: windowVisibility =
                                   (windowVisibility === Window.FullScreen
                                        ? Window.Windowed
                                        : Window.FullScreen)
                }
            }
        }

        Item {
            id: videoStage
            Layout.fillWidth: true
            Layout.fillHeight: true

            VideoOutput {
                id: videoOutput
                anchors.fill: parent
                fillMode: VideoOutput.PreserveAspectFit
                visible: !psMode
            }

            Image {
                id: psVideo
                anchors.fill: parent
                visible: psMode
                fillMode: Image.PreserveAspectFit
                cache: false
                source: "image://svac/frame?serial=" + PlaybackManager.frameSerial
            }

            Label {
                anchors.centerIn: parent
                visible: !psMode
                         && player.playbackState !== MediaPlayer.PlayingState
                         && (!player.source || player.source.toString().length === 0)
                text: qsTr("点击“打开”选择本地视频")
                color: "#B9C0CC"
            }
        }

        Rectangle {
            Layout.fillWidth: true
            color: "#0D0F13"
            height: 112

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Label {
                        text: formatMs(userIsSeeking ? seekSlider.value : player.position)
                        color: "#D7DBE4"
                        font.pixelSize: 12
                    }

                    Slider {
                        id: seekSlider
                        Layout.fillWidth: true
                        from: 0
                        to: Math.max(0, player.duration)
                        enabled: player.duration > 0
                        value: player.position

                        onPressedChanged: {

                            userIsSeeking = pressed
                            if (!pressed) {
                                player.position = value
                            }
                        }
                    }

                    Label {
                        text: formatMs(player.duration)
                        color: "#D7DBE4"
                        font.pixelSize: 12
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    RoundButton {
                        id: playPauseButton
                        enabled: player.source && player.source.toString().length > 0
                        width: 44
                        height: 44

                        contentItem: Label {
                            text: player.playbackState === MediaPlayer.PlayingState ? "❚❚" : "▶"
                            color: playPauseButton.enabled ? "#EDEFF5" : "#7B8392"
                            font.pixelSize: 18
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            radius: width / 2
                            color: playPauseButton.down ? "#2B3140" : "#1C2230"
                            border.color: "#3A4254"
                            border.width: 1
                        }

                        onClicked: {
                            if (player.playbackState === MediaPlayer.PlayingState)
                                player.pause()
                            else
                                player.play()
                        }
                    }

                    RoundButton {
                        id: stopButton
                        enabled: player.source && player.source.toString().length > 0
                        width: 44
                        height: 44

                        contentItem: Label {
                            text: "■"
                            color: stopButton.enabled ? "#EDEFF5" : "#7B8392"
                            font.pixelSize: 18
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            radius: width / 2
                            color: stopButton.down ? "#2B3140" : "#1C2230"
                            border.color: "#3A4254"
                            border.width: 1
                        }

                        onClicked: player.stop()
                    }

                    Item { Layout.fillWidth: true }

                    ToolButton {
                        id: muteButton
                        checkable: true
                        text: checked ? qsTr("已静音") : qsTr("静音")
                    }

                    Slider {
                        id: volumeSlider
                        width: 140
                        from: 0
                        to: 1
                        value: 0.8
                    }
                }
            }
        }
    }
}

