import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Window {
    id: root
    width: 960
    height: 520
    visible: true
    title: ""
    flags: Qt.FramelessWindowHint

    minimumWidth: 900
    minimumHeight: 520
    // 持有 HomePage 引用，避免 createObject(null) 创建的窗口被 QML GC 回收
    property var homeWindowRef: null

    // 登录失败提示
    MessageDialog {
        id: loginErrorDialog
        title: qsTr("登录失败")
        text: qsTr("登录验证失败，请检查密码、国标ID或服务器地址。")
        buttons: MessageDialog.Ok
    }

    // 顶层自绘标题栏按钮（最小化/关闭），位于窗口最右上角
    Row {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 4
        anchors.rightMargin: 4
        spacing: 4
        z: 20

        ToolButton {
            text: "-"
            onClicked: root.showMinimized()
        }

        ToolButton {
            text: "✕"
            onClicked: Qt.quit()
        }
    }

    // 左右布局：左图右登录 / 播放器
    RowLayout {
        anchors.fill: parent
        spacing: 0

        // 左侧蓝色插画区（复用前面示例的感觉）
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.55
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#0E7BD7" }
                GradientStop { position: 1.0; color: "#12B3FF" }
            }

            Image {
                anchors.centerIn: parent
                // 请将 logo 图片拷贝到工程下的 images/logo.png
                source: "images/logo.png"
                fillMode: Image.PreserveAspectFit
                sourceSize.width: 200
                sourceSize.height: 200
                smooth: true
            }

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 32
                anchors.top: parent.top
                anchors.topMargin: 32
                text: qsTr("视频图像综合应用平台")
                color: "white"
                font.pixelSize: 22
                font.bold: true
            }
        }

        // 右侧：登录或播放器
        // 右侧：登录界面
        Pane {
            padding: 32
            Layout.fillHeight: true
            Layout.fillWidth: true
            background: Rectangle {
                color: "#F9FBFF"
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 32
                spacing: 24

                    // 顶部：左侧“欢迎登录”标题，右侧语言下拉，水平对齐
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.alignment: Qt.AlignTop
                            Layout.fillWidth: true
                            spacing: 4
                            Label {
                                text: qsTr("欢迎登录")
                                font.pixelSize: 24
                                font.bold: true
                                color: "#333A4D"
                            }

                        }

                        Item { Layout.fillWidth: true }

                        ComboBox {
                            Layout.alignment: Qt.AlignTop
                            model: [qsTr("简体中文"), qsTr("English")]
                            flat: true
                            Layout.preferredHeight: 32
                            font.pixelSize: 16
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        Label {
                            text: qsTr("UKey 类型")
                            font.pixelSize: 12
                            color: "#5D6575"
                        }
                        ComboBox {
                            id: ukeyTypeCombo
                            Layout.fillWidth: true
                            Layout.preferredHeight: 32
                            font.pixelSize: 16
                            model: [qsTr("国芯"), qsTr("渔翁")]
                            currentIndex: 0
                        }
                    }

                    // 配置服务器地址：IP + 历史下拉 + 端口
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Label {
                            text: qsTr("配置服务器地址")
                            font.pixelSize: 12
                            color: "#5D6575"
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            ComboBox {
                                id: serverIpCombo
                                Layout.fillWidth: true
                                Layout.preferredHeight: 32
                                font.pixelSize: 16
                                editable: true
                                model: [ "172.17.43.25", "192.168.1.100", "127.0.0.1" ]
                                currentIndex: 0
                            }

                            TextField {
                                id: serverPortField
                                width: 80
                                Layout.preferredHeight: 32
                                font.pixelSize: 16
                                inputMethodHints: Qt.ImhDigitsOnly
                                placeholderText: qsTr("端口")
                                text: "5062"
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        Label {
                            text: qsTr("国标ID")
                            font.pixelSize: 12
                            color: "#5D6575"
                        }
                        TextField {
                            id: gbIdField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 32
                            font.pixelSize: 16
                            placeholderText: qsTr("请输入国标ID")
                            text: "13100000002001000111"
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        Label {
                            text: qsTr("密码")
                            font.pixelSize: 12
                            color: "#5D6575"
                        }
                        TextField {
                            id: passwordField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 32
                            font.pixelSize: 16
                            echoMode: TextInput.Password
                            placeholderText: qsTr("请输入密码")
                        }
                    }

                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    text: qsTr("登录")
                    font.pixelSize: 16
                    font.bold: true
                    onClicked: {
                        var ok = LoginManager.login(
                                    ukeyTypeCombo.currentText,
                                    passwordField.text,
                                    gbIdField.text,
                                    serverIpCombo.editText,
                                    serverPortField.text)
                        if (ok) {
                            // 登录成功后设置 SDK PIN：PlaybackManager 会在首次播放时用它初始化 p35114_sdk.dll
                            PlaybackManager.setSdkPin(passwordField.text)
                            var component = Qt.createComponent("HomePage.qml")
                            if (component.status === Component.Ready) {
                                root.homeWindowRef = component.createObject(null, {
                                    "serverIp": serverIpCombo.editText,
                                    "serverPort": serverPortField.text
                                })
                                if (!root.homeWindowRef)
                                    console.log("HomePage create failed")
                                else
                                    root.homeWindowRef.destroyed.connect(function() { root.homeWindowRef = null })
                            } else {
                                console.log("HomePage component error:", component.errorString())
                            }
                            root.close()
                        } else {
                            loginErrorDialog.text = qsTr("登录失败: ") + LoginManager.lastError()
                            loginErrorDialog.open()
                        }
                    }
                }
            }
        }
    }
}

