import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Window {
    id: home
    width: 1200
    height: 720
    visible: true
    title: ""
    flags: Qt.FramelessWindowHint

    // 登录页传入的服务器信息，用来拼接资源树接口地址
    property string serverIp: ""
    property string serverPort: ""

    function resourceEndpoint() {
        if (!serverIp || !serverPort)
            return ""
        // TODO: 按你的真实接口路径修改这里
        return "http://" + serverIp + ":" + serverPort + "/api/resource/tree"
    }

    property var cameraSearchResults: []
    // 预览分屏模式：1/4/9
    property int previewSplit: 1
    property string playbackHint: ""
    property bool psPlaying: false

    function windowButtonSymbol(role) {
        if (role === "min") return "—"
        if (role === "max") return home.visibility === Window.Maximized ? "❐" : "▢"
        return "✕"
    }

    FileDialog {
        id: psFileDialog
        title: qsTr("选择 PS 文件")
        nameFilters: [qsTr("PS 文件 (*.ps)"), qsTr("所有文件 (*)")]
        onAccepted: {
            psPlaying = PlaybackManager.playPsFile(selectedFile)
        }
    }

    Connections {
        target: PlaybackManager
        function onPlayStarted(filePath) {
            psPlaying = true
            playbackHint = qsTr("开始播放: ") + filePath
        }
        function onPlayFinished(decodedFrameCount) {
            psPlaying = false
            playbackHint = qsTr("播放完成，解码帧数: ") + decodedFrameCount
        }
        function onPlayFailed(reason) {
            psPlaying = false
            playbackHint = qsTr("播放失败: ") + reason
        }
    }

    // 顶部蓝色工具栏 + 标签
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: topBar
            Layout.fillWidth: true
            height: 48
            color: "#1C8CE6"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 10
                spacing: 10

                Label {
                    text: qsTr("视频图像综合应用平台")
                    color: "white"
                    font.pixelSize: 17
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                ToolButton {
                    width: 28
                    height: 24
                    text: "?"
                    background: Rectangle {
                        radius: 4
                        color: "transparent"
                    }
                }

                ToolButton {
                    width: 28
                    height: 24
                    text: windowButtonSymbol("min")
                    onClicked: home.showMinimized()
                    background: Rectangle {
                        radius: 4
                        color: parent.down ? "#4AA3F0" : parent.hovered ? "#3D97E8" : "transparent"
                        border.color: "#7EC0F7"
                        border.width: 1
                    }
                    contentItem: Label {
                        text: parent.text
                        color: "white"
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                ToolButton {
                    width: 28
                    height: 24
                    text: windowButtonSymbol("max")
                    onClicked: home.visibility = (home.visibility === Window.Maximized
                                                  ? Window.Windowed
                                                  : Window.Maximized)
                    background: Rectangle {
                        radius: 4
                        color: parent.down ? "#4AA3F0" : parent.hovered ? "#3D97E8" : "transparent"
                        border.color: "#7EC0F7"
                        border.width: 1
                    }
                    contentItem: Label {
                        text: parent.text
                        color: "white"
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                ToolButton {
                    width: 28
                    height: 24
                    text: windowButtonSymbol("close")
                    onClicked: home.close()
                    background: Rectangle {
                        radius: 4
                        color: parent.down ? "#E35D5D" : parent.hovered ? "#D94A4A" : "transparent"
                        border.color: "#F0A0A0"
                        border.width: 1
                    }
                    contentItem: Label {
                        text: parent.text
                        color: "white"
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }

        // 顶部二级导航（首页 / 视频监控 ...）
        Rectangle {
            Layout.fillWidth: true
            height: 40
            color: "#ffffff"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 12

                TabBar {
                    id: tabBar
                    Layout.alignment: Qt.AlignVCenter
                    TabButton { text: qsTr("首页") }
                    TabButton { text: qsTr("视频监控") }
                }

                Item { Layout.fillWidth: true }
            }
        }

        // 主内容区：左侧资源树 + 右侧视频预览
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#f0f3f8"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                // 左侧功能图标栏
                Rectangle {
                    Layout.preferredWidth: 52
                    Layout.fillHeight: true
                    radius: 4
                    color: "#ffffff"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 6

                        Repeater {
                            model: [
                                { icon: "⌂", tip: qsTr("首页") },
                                { icon: "◫", tip: qsTr("视频监控") },
                                { icon: "◉", tip: qsTr("告警") },
                                { icon: "▣", tip: qsTr("回放") },
                                { icon: "☰", tip: qsTr("资源") }
                            ]

                            delegate: ToolButton {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 44
                                text: modelData.icon
                                ToolTip.visible: hovered
                                ToolTip.text: modelData.tip

                                background: Rectangle {
                                    radius: 6
                                    color: parent.down ? "#D8EAFE" : parent.hovered ? "#EEF5FF" : "transparent"
                                    border.color: parent.down ? "#7DB5F2" : "transparent"
                                    border.width: 1
                                }

                                contentItem: Label {
                                    text: parent.text
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    color: "#1C8CE6"
                                    font.pixelSize: 24
                                }
                            }
                        }
                    }
                }

                // 左侧资源监控面板
                Rectangle {
                    Layout.preferredWidth: 280
                    Layout.fillHeight: true
                    radius: 6
                    color: "#ffffff"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Label {
                                text: qsTr("资源监控")
                                font.pixelSize: 18
                                font.bold: true
                                color: "#333A4D"
                            }

                            Item { Layout.fillWidth: true }

                            ToolButton {
                                text: "↻"
                                onClicked: {
                                    var url = home.resourceEndpoint()
                                    if (url && url.length > 0) {
                                        CameraTreeService.endpoint = url
                                        CameraTreeService.refresh()
                                    } else {
                                        console.log("endpoint empty")
                                    }
                                }
                            }       
                        }

                        TabBar {
                            id: searchModeBar
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            TabButton { text: qsTr("按组织搜") }
                            TabButton { text: qsTr("按标签搜") }
                        }

                        CheckBox {
                            text: qsTr("只看在线资源")
                        }

                        ComboBox {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 34
                            model: [qsTr("基础目录"), qsTr("视频目录"), qsTr("组织目录")]
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            TextField {
                                id: resourceSearch
                                Layout.fillWidth: true
                                Layout.preferredHeight: 34
                                placeholderText: qsTr("请输入关键字/国标编码/...")
                                font.pixelSize: 14
                            }

                            ToolButton {
                                text: "🔍"
                                Layout.preferredWidth: 34
                                Layout.preferredHeight: 34
                                onClicked: {
                                    var t = resourceSearch.text.trim()
                                    if (!t || t.length === 0) {
                                        cameraSearchResults = []
                                        return
                                    }
                                    cameraSearchResults = CameraTreeModel.findCamerasByLabel(t)
                                }
                            }
                        }

                        ListView {
                            id: cameraResultList
                            Layout.fillWidth: true
                            Layout.fillHeight: cameraSearchResults.length > 0
                            Layout.preferredHeight: cameraSearchResults.length > 0 ? 0 : 0
                            visible: cameraSearchResults.length > 0
                            clip: true
                            model: cameraSearchResults

                            delegate: ItemDelegate {
                                width: ListView.view.width
                                contentItem: RowLayout {
                                    spacing: 8

                                    Label {
                                        // 左侧图标（与树里相机节点一致）
                                        text: "📷"
                                        color: "#4C84D8"
                                        font.pixelSize: 13
                                        horizontalAlignment: Text.AlignHCenter
                                        Layout.preferredWidth: 18
                                    }

                                    Label {
                                        text: (modelData.label || "") + "  [" + (modelData.id || "") + "]"
                                        color: "#1F2A37"
                                        font.pixelSize: 13
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                }
                                onClicked: {
                                    var idx = CameraTreeModel.findFirstByLabel(modelData.label || "")
                                    if (idx && idx.isValid) {
                                        resourceTree.expandToIndex(idx)
                                        resourceTree.currentIndex = idx
                                    }
                                }
                            }
                        }

                        TreeView {
                            id: resourceTree
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            visible: cameraSearchResults.length === 0
                            clip: true
                            model: CameraTreeModel

                            delegate: TreeViewDelegate {
                                width: resourceTree.width
                                height: 34
                                spacing: 6

                                background: Rectangle {
                                    anchors.fill: parent
                                    radius: 3
                                    color: selected ? "#2F88FF" : (hovered ? "#EAF3FF" : "transparent")
                                }

                                // 不要 anchors.fill/padding，避免破坏 TreeViewDelegate 自带缩进
                                contentItem: RowLayout {
                                    spacing: 6
                                    anchors.verticalCenter: parent.verticalCenter

                                    Label {
                                        // 目录/相机图标
                                        text: model.type === "camera" ? "📷" : "🏢"
                                        color: selected ? "white" : "#4C84D8"
                                        font.pixelSize: 13
                                        horizontalAlignment: Text.AlignHCenter
                                        Layout.preferredWidth: 18
                                    }

                                    Label {
                                        text: model.label
                                        color: selected ? "white" : "#1F2A37"
                                        font.pixelSize: 13
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }

                                    Label {
                                        // 统计：按你截图的 (0/2) 形式，暂用 0/相机总数
                                        visible: model.type === "folder"
                                        text: "(" + 0 + "/" + model.cameraCount + ")"
                                        color: selected ? "white" : "#667085"
                                        font.pixelSize: 12
                                    }
                                }

                                onClicked: {
                                    // 支持单击整行展开/收起目录
                                    if (hasChildren) {
                                        resourceTree.toggleExpanded(row)
                                        return
                                    }
                                    // camera：点击后拉取 RTSP SVAC 密流到右侧预览区
                                    var testRtsp = "rtsp://172.17.42.63:554/rtp/13100000002001000111_11010600001310000001"
                                    var ok = PlaybackManager.playRtspUrl(testRtsp)
                                    if (!ok) {
                                        playbackHint = qsTr("RTSP 播放启动失败")
                                    }
                                }
                            }
                        }
                    }
                }

                // 右侧视频预览区域
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 4
                    color: "#ffffff"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Label {
                                text: qsTr("公共预案分组")
                                font.pixelSize: 15
                                font.bold: true
                                color: "#333A4D"
                            }

                            Item { Layout.fillWidth: true }

                            Button {
                                text: qsTr("预览")
                                Layout.preferredWidth: 72
                            }
                            Button {
                                text: qsTr("回放")
                                Layout.preferredWidth: 72
                            }
                            Button {
                                text: qsTr("播放PS")
                                Layout.preferredWidth: 84
                                onClicked: psFileDialog.open()
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: playbackHint.length > 0
                            text: playbackHint
                            color: "#667085"
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }

                        Rectangle {
                            id: previewArea
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: "#101418"
                            radius: 3

                            GridLayout {
                                id: previewGrid
                                anchors.fill: parent
                                anchors.margins: 4
                                rowSpacing: 2
                                columnSpacing: 2
                                rows: home.previewSplit === 1 ? 1 : (home.previewSplit === 4 ? 2 : 3)
                                columns: home.previewSplit === 1 ? 1 : (home.previewSplit === 4 ? 2 : 3)

                                Repeater {
                                    model: home.previewSplit

                                    delegate: Rectangle {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        color: "#000000"
                                        border.color: "#2A2F36"
                                        border.width: 1

                                        Image {
                                            anchors.fill: parent
                                            visible: home.psPlaying && index === 0
                                            fillMode: Image.PreserveAspectFit
                                            cache: false
                                            source: "image://svac/frame?serial=" + PlaybackManager.frameSerial
                                        }

                                        Label {
                                            anchors.centerIn: parent
                                            visible: !(home.psPlaying && index === 0)
                                            text: qsTr("画面 ") + (index + 1)
                                            color: "#8FA0B5"
                                            font.pixelSize: 12
                                        }
                                    }
                                }
                            }

                            // 右下角分屏切换按钮
                            Row {
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                anchors.rightMargin: 8
                                anchors.bottomMargin: 8
                                spacing: 6

                                ToolButton {
                                    text: "◻"
                                    checkable: true
                                    checked: home.previewSplit === 1
                                    onClicked: home.previewSplit = 1
                                    width: 24
                                    height: 24
                                    background: Rectangle {
                                        radius: 3
                                        color: parent.checked ? "#DDE3EA" : (parent.hovered ? "#CFD6DE" : "#BCC5CF")
                                        border.color: "#E7ECF1"
                                        border.width: 1
                                    }
                                    contentItem: Label {
                                        text: parent.text
                                        color: "#4F5A67"
                                        font.pixelSize: 12
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                }

                                ToolButton {
                                    text: "▦"
                                    checkable: true
                                    checked: home.previewSplit === 4
                                    onClicked: home.previewSplit = 4
                                    width: 24
                                    height: 24
                                    background: Rectangle {
                                        radius: 3
                                        color: parent.checked ? "#DDE3EA" : (parent.hovered ? "#CFD6DE" : "#BCC5CF")
                                        border.color: "#E7ECF1"
                                        border.width: 1
                                    }
                                    contentItem: Label {
                                        text: parent.text
                                        color: "#4F5A67"
                                        font.pixelSize: 12
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                }

                                ToolButton {
                                    text: "▩"
                                    checkable: true
                                    checked: home.previewSplit === 9
                                    onClicked: home.previewSplit = 9
                                    width: 24
                                    height: 24
                                    background: Rectangle {
                                        radius: 3
                                        color: parent.checked ? "#DDE3EA" : (parent.hovered ? "#CFD6DE" : "#BCC5CF")
                                        border.color: "#E7ECF1"
                                        border.width: 1
                                    }
                                    contentItem: Label {
                                        text: parent.text
                                        color: "#4F5A67"
                                        font.pixelSize: 12
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

