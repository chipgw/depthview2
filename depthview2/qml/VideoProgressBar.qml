import QtQuick 2.5
import QtQuick.Controls 2.0
import QtQuick.Window 2.2
import QtAV 1.6

ProgressBar {
    background.height: Screen.pixelDensity > 12 ? 32 : 12
    contentItem.implicitHeight: Screen.pixelDensity > 12 ? 24 : 8

    to: image.videoDuration
    value: image.videoPosition

    MouseArea {
        id: mouseArea
        anchors.fill: parent.background

        /* mouseX * this = the position of the video at the point under the cursor.
                 * (mouseX is already relative to this object, which makes it easy.) */
        property real screenPosToTime: image.videoDuration / width

        /* When the mouse moves (only when pressed) or is clicked, seek to that position. */
        onMouseXChanged: if (containsPress) image.seek(screenPosToTime * mouseX);
        onClicked: image.seek(screenPosToTime * mouseX);

        acceptedButtons: Qt.LeftButton

        hoverEnabled: true

        ToolTip {
            id: toolTip

            /* Limit to parent's width to avoid bugging to the wrong side of the screen when near the edge. */
            x: Math.min(Math.max(parent.mouseX - implicitWidth / 2, 0), parent.width - implicitWidth)
            visible: mouseArea.containsMouse

            contentItem: Item {
                implicitWidth: Math.max(thumbWrapper.width, thumbText.implicitWidth)
                implicitHeight: childrenRect.height

                Item {
                    id: thumbWrapper

                    property size maxSize: Qt.size(320, 240)

                    /* Why this is needed when the parent is the same exact width IDK, but it doesn't work without it. */
                    anchors.horizontalCenter: parent.horizontalCenter

                    /* Fit the output size inside the limits maintaining aspect ratio. */
                    width: Math.min(maxSize.width / image.stereoSize.width, maxSize.height / image.stereoSize.height) * image.stereoSize.width
                    height: Math.min(maxSize.width / image.stereoSize.width, maxSize.height / image.stereoSize.height) * image.stereoSize.height

                    VideoPreview {
                        id: thumbnail

                        /* This needs to be to its parent as the source size is to the stereo size. */
                        width: parent.width * image.sourceSize.width / image.stereoSize.width
                        height: parent.height * image.sourceSize.height / image.stereoSize.height

                        file: image.source
                        timestamp: mouseArea.screenPosToTime * mouseArea.mouseX

                        /* Always stretch. We set the VideoOutput to the size we want. */
                        fillMode: VideoOutput.Stretch
                    }
                    StereoShader {
                        target: thumbnail
                    }
                }

                Text {
                    id: thumbText
                    anchors {
                        horizontalCenter: parent.horizontalCenter
                        top: thumbWrapper.bottom
                    }
                    text: image.timeString(mouseArea.screenPosToTime * mouseArea.mouseX)
                    font: toolTip.font
                }
            }
        }
    }
}
