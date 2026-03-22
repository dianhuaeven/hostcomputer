import QtQuick
import QtQuick3D
import QtQuick.Controls

Item {
    id: root

    // 由C++ rootContext注入的 robotViewModel
    property var viewModel: typeof robotViewModel !== "undefined" ? robotViewModel : null

    function clampAngle(angle) {
        angle = angle % 360
        if (angle > 180) angle -= 360
        if (angle < -180) angle += 360
        return angle
    }

    View3D {
        id: view3D
        anchors.fill: parent

        environment: SceneEnvironment {
            clearColor: "white"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }

        Node {
            id: scene

            PerspectiveCamera {
                id: sceneCamera
                x: -1.869
                y: 9.006
                z: 556.8877
            }

            // 车体底盘
            Model {
                id: cubeModel
                x: 0; y: 0; z: 0
                eulerRotation.x: root.viewModel ? root.viewModel.pitch : 0
                eulerRotation.y: root.viewModel ? root.viewModel.yaw   : 0
                eulerRotation.z: root.viewModel ? root.viewModel.roll  : 0
                materials: PrincipledMaterial {
                    id: defaultMaterial
                    baseColor: "#FFFFFF"
                }
                source: "#Cube"
                scale: Qt.vector3d(2.0, 0.6, 1.0)

                // 腿1 - 左前
                Node {
                    id: rotationPivot1
                    x: -39.0; y: 40.0; z: 55.0
                    eulerRotation.z: root.viewModel ? root.viewModel.leg1Angle : 0
                    Model {
                        x: 0.0; y: 20.0; z: 0.0
                        source: "#Cube"
                        materials: PrincipledMaterial { baseColor: "black" }
                        scale: Qt.vector3d(0.2, 1.8, 0.3)
                    }
                }

                // 腿2 - 右前
                Node {
                    id: rotationPivot2
                    x: 39.0; y: 40.0; z: 55.0
                    eulerRotation.z: root.viewModel ? root.viewModel.leg2Angle : 0
                    Model {
                        x: 0.0; y: 20.0; z: 0.0
                        source: "#Cube"
                        materials: PrincipledMaterial { baseColor: "black" }
                        scale: Qt.vector3d(0.2, 1.8, 0.3)
                    }
                }

                // 腿3 - 右后
                Node {
                    id: rotationPivot3
                    x: 35.0; y: 40.0; z: -55.0
                    eulerRotation.z: root.viewModel ? root.viewModel.leg3Angle : 0
                    Model {
                        x: 4.50; y: 20.0; z: 0.0
                        source: "#Cube"
                        materials: PrincipledMaterial { baseColor: "black" }
                        scale: Qt.vector3d(0.2, 1.8, 0.3)
                    }
                }

                // 腿4 - 左后
                Node {
                    id: rotationPivot4
                    x: -37.0; y: 40.0; z: -55.0
                    eulerRotation.z: root.viewModel ? root.viewModel.leg4Angle : 0
                    Model {
                        x: -2.50; y: 20.0; z: 0.0
                        source: "#Cube"
                        materials: PrincipledMaterial { baseColor: "black" }
                        scale: Qt.vector3d(0.2, 1.8, 0.3)
                    }
                }

                // 前方向指示锥
                Model {
                    source: "#Cone"
                    x: -100; y: 0; z: 0
                    eulerRotation.z: -90
                    materials: PrincipledMaterial { baseColor: "red" }
                    scale: Qt.vector3d(0.2, 0.2, 0.2)
                }
            }

            SpotLight {
                x: 2.553; y: 25.21; z: 800.71
                brightness: 20.0
            }
        }
    }

    // 姿态角显示（左上）
    Column {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 8
        spacing: 2

        Text {
            color: "black"
            font.pixelSize: 14
            text: "Pitch: " + (root.viewModel ? Math.round(clampAngle(root.viewModel.pitch)) : 0) + "°"
        }
        Text {
            color: "black"
            font.pixelSize: 14
            text: "Yaw:   " + (root.viewModel ? Math.round(clampAngle(root.viewModel.yaw))   : 0) + "°"
        }
        Text {
            color: "black"
            font.pixelSize: 14
            text: "Roll:  " + (root.viewModel ? Math.round(clampAngle(root.viewModel.roll))  : 0) + "°"
        }
    }

    // 腿部角度显示（右上）
    Column {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 8
        spacing: 2

        Text {
            color: "black"
            font.pixelSize: 14
            text: "腿1: " + (root.viewModel ? Math.round(root.viewModel.leg1Angle) : 0) + "°"
        }
        Text {
            color: "black"
            font.pixelSize: 14
            text: "腿2: " + (root.viewModel ? Math.round(root.viewModel.leg2Angle) : 0) + "°"
        }
        Text {
            color: "black"
            font.pixelSize: 14
            text: "腿3: " + (root.viewModel ? Math.round(root.viewModel.leg3Angle) : 0) + "°"
        }
        Text {
            color: "black"
            font.pixelSize: 14
            text: "腿4: " + (root.viewModel ? Math.round(root.viewModel.leg4Angle) : 0) + "°"
        }
    }
}
