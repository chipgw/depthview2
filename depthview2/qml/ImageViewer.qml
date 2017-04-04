import QtQuick 2.5
import QtQuick.Controls 2.0
import QtAV 1.6
import DepthView 2.0

Item {
    id: root

    property real zoom: -1

    /* The zoom/scale value that fits the whole image on the screen. */
    readonly property real fitScale: FolderListing.currentFileIsSurround ? 1 : Math.min(width / stereoSize.width, height / stereoSize.height)
    /* The actual scale value to be applied to the image/video, if zoom is -1 use the size-to-fit scale. */
    readonly property real targetScale: (zoom < 0) ? fitScale : zoom

    function playPause() {
        if (FolderListing.currentFileIsVideo) {
            /* Reset speed. */
            media.playbackRate = 1.0;

            if (media.playbackState === MediaPlayer.PlayingState)
                media.pause()
            else
                media.play()
        }
    }

    function timeString(ms) {
        /* Turn a time value in milliseconds into a string in the format: "[h:]mm:ss". */
        return (ms > 3600000 ? (Math.floor(ms / 3600000) + ":") : "") /* Don't show hours if there are none. */
                + ("0" + Math.floor(ms / 60000) % 60).slice(-2) /* Always pad minutes with a zero. */
                + ":" + ("0" + Math.floor(ms / 1000) % 60).slice(-2) /* Always put a colon in between and pad with a zero. */
    }

    function fastForward() {
        media.play()
        media.playbackRate = Math.min(media.playbackRate * 2.0, 8.0);
    }

    readonly property bool isPlaying: FolderListing.currentFileIsVideo && media.playbackState === MediaPlayer.PlayingState

    property url source: FolderListing.currentURL

    onSourceChanged: {
        /* Reset video position and speed. */
        seek(0);
        media.playbackRate = 1.0;

        /* Reset the zoom to fit. */
        zoom = -1
    }

    Binding {
        /* Give C++ access to the texture for the currently open image or video. */
        target: DepthView
        property: "openImageTarget"
        value: FolderListing.currentFileIsVideo ? vid : image.sourceImage
    }

    /* Wrap video properties for use in UI. */
    property alias videoPosition: media.position
    property alias videoDuration: media.duration
    property alias videoVolume: media.volume

    readonly property string mediaInfo: FolderListing.currentFileInfo + (FolderListing.currentFileIsVideo ?
                                   qsTr("<br>Duration: ") + timeString(media.metaData.duration) +
                                   qsTr("<h2>Video Info:</h2>") +
                                   qsTr("Codec: ") + media.metaData.videoCodec +
                                   qsTr("<br>Frame Rate: ") + media.metaData.videoFrameRate +
                                   qsTr("<br>Bit Rate: ") + media.metaData.videoBitRate +
                                   (media.metaData.resolution ? qsTr("<br>Resolution: ") + media.metaData.resolution.width + "x" + media.metaData.resolution.height : "") +
                                   qsTr("<br>Pixel Format: ") + media.metaData.pixelFormat +
                                   qsTr("<h2>Audio Info:</h2>") +
                                   qsTr("Codec: ") + media.metaData.audioCodec +
                                   qsTr("<br>Bit Rate: ") + media.metaData.audioBitRate
                                 : qsTr("<br>Resolution: ") + image.width + "x" + image.height)

    /* The size of the full image/video in its raw form, no stereo accounted for. */
    readonly property size sourceSize: FolderListing.currentFileIsVideo ? Qt.size(vid.width, vid.height) : image.sourceSize
    /* The size of a single eye of stereo video/image. */
    readonly property size stereoSize: FolderListing.currentFileIsVideo ? Qt.size(vidWrapper.width, vidWrapper.height) :
                                                                          Qt.size(image.width, image.height)

    readonly property int stereoMode: FolderListing.currentFileStereoMode

    function seek(offset) {
        if (FolderListing.currentFileIsVideo)
            media.seek(offset)
    }

    Flickable {
        id: imageFlickable
        anchors.fill: parent

        contentWidth: imageContainer.width
        contentHeight: imageContainer.height

        /* Only enable panning when the image is zoomed in enough. */
        interactive: (contentWidth > width || contentHeight > height)

        ScrollBar.horizontal: ScrollBar { }
        ScrollBar.vertical: ScrollBar { }

        /* The location that the center of the screen is focused on. */
        property point currentCenter: Qt.point(0.5, 0.5)

        /* Used to track differences in the visible screen area. */
        property size oldVisibleArea

        visibleArea.onHeightRatioChanged: {
            /* Only run if visible area has actually changed and height is valid. */
            if (oldVisibleArea.height !== visibleArea.heightRatio && height > 0) {
                /* Find the maximum possible contentY. */
                var hidden = contentHeight - height

                /* Scale the stored center to match the new visible area. */
                var center = currentCenter.y * (oldVisibleArea.height / visibleArea.heightRatio)

                /* Put center from the range [-hidden/2, hidden/2] to [0, hidden] and make sure it doesn't exceed that range. */
                contentY = Math.max(0, Math.min(hidden * 0.5 + center, hidden))

                /* When contentY is zero the center isn't updated automatically. */
                if (contentY === 0)
                    onContentYChanged()

                /* Store the values for next time. */
                oldVisibleArea.height = visibleArea.heightRatio
            }
        }

        /* Current center is the offset in pixels from the image center. */
        onContentYChanged: currentCenter.y = contentY - (contentHeight - height) * 0.5

        visibleArea.onWidthRatioChanged: {
            /* Only run if visible area has actually changed and width is valid. */
            if (oldVisibleArea.width !== visibleArea.widthRatio && width > 0) {
                /* Find the maximum possible contentX. */
                var hidden = contentWidth - width

                /* Scale the stored center to match the new visible area. */
                var center = currentCenter.x * (oldVisibleArea.width / visibleArea.widthRatio)

                /* Put center from the range [-hidden/2, hidden/2] to [0, hidden] and make sure it doesn't exceed that range. */
                contentX = Math.max(0, Math.min(hidden * 0.5 + center, hidden))

                /* When contentX is zero the center isn't updated automatically. */
                if (contentX === 0)
                    onContentXChanged()

                /* Store the values for next time. */
                oldVisibleArea.width = visibleArea.widthRatio
            }
        }

        /* Current center is the offset in pixels from the image center. */
        onContentXChanged: currentCenter.x = contentX - (contentWidth - width) * 0.5

        Item {
            id: imageContainer
            width: Math.max(image.width * image.scale, imageFlickable.width)
            height: Math.max(image.height * image.scale, imageFlickable.height)

            StereoImage {
                anchors.centerIn: parent
                id: image

                imageMode: stereoMode

                source: FolderListing.currentFileIsVideo ? "" : root.source

                /* If zoom is negative we scale to fit, otherwise just use the value of zoom. */
                scale: targetScale

                visible: !FolderListing.currentFileIsSurround

                swap: FolderListing.currentFileStereoSwap
            }
        }
    }

    Item {
        id: vidWrapper
        visible: FolderListing.currentFileIsVideo && !FolderListing.currentFileIsSurround

        anchors.centerIn: parent

        /* Calculate the size difference for side by side and top bottom source modes. */
        width: (stereoMode === SourceMode.SidebySide || stereoMode === SourceMode.SidebySideAnamorphic) ? vid.width / 2 : vid.width
        height: (stereoMode === SourceMode.TopBottom || stereoMode === SourceMode.TopBottomAnamorphic) ? vid.height / 2 : vid.height

        scale: targetScale

        MediaPlayer {
            id: media
            source: FolderListing.currentFileIsVideo ? root.source : ""

            autoPlay: true
        }

        VideoOutput2 {
            id: vid
            source: media

            width: (stereoMode === SourceMode.SidebySideAnamorphic) ? sourceRect.width * 2 : sourceRect.width
            height: (stereoMode === SourceMode.TopBottomAnamorphic) ? sourceRect.height * 2 : sourceRect.height

            /* Always stretch. We set the VideoOutput to the size we want. */
            fillMode: VideoOutput.Stretch

            /* Hide the video, just use it as a source for the ShaderEffect. */
            opacity: 0
        }

        StereoShader {
            target: vid
            stereoMode: root.stereoMode

            swap: FolderListing.currentFileStereoSwap
        }
    }
    BusyIndicator {
        anchors.centerIn: parent
        running: image.status === Image.Loading || media.status === MediaPlayer.Loading || media.status === MediaPlayer.Buffering
    }

    MouseArea {
        id: imageMouseArea

        anchors.fill: parent

        acceptedButtons: Qt.MiddleButton | Qt.ForwardButton | Qt.BackButton | (FolderListing.currentFileIsSurround ? Qt.LeftButton : 0)

        /* Reset zoom on wheel double-click. */
        onDoubleClicked: if (mouse.button === Qt.MiddleButton) zoom = (zoom === -1) ? 1 : -1

        onWheel:
            /* Don't zoom or seek if covered. */
            if (!fileBrowser.visible) {
                /* For videos, the mouse wheel seeks the video. */
                if (FolderListing.currentFileIsVideo) {
                    if (wheel.angleDelta.y > 0)
                        media.seekForward()
                    else if (wheel.angleDelta.y < 0)
                        media.seekBackward()
                } else if (wheel.angleDelta.y != 0) {
                    /* If zoom-to-fit is active retrieve the current scale before zooming in or out. */
                    if (zoom < 0)
                        zoom = targetScale

                    zoom += wheel.angleDelta.y * zoom * 0.001
                    zoom = Math.max(0.2, Math.min(zoom, 4.0))
                }

                /* Sideways scroll goes through files, unless a video is open. */
                if (!FolderListing.currentFileIsVideo && wheel.angleDelta.x > 0)
                    FolderListing.openPrevious()
                if (!FolderListing.currentFileIsVideo && wheel.angleDelta.x < 0)
                    FolderListing.openNext()
            }

        onClicked:
            /* Thumb buttons go through files. */
            if (mouse.button === Qt.BackButton)
                FolderListing.openPrevious()
            else if (mouse.button === Qt.ForwardButton)
                FolderListing.openNext()

        property var surroundPanning: undefined

        onPressed: if (mouse.button === Qt.LeftButton) surroundPanning = Qt.point(mouseX, mouseY)
        onReleased: if (mouse.button === Qt.LeftButton) surroundPanning = undefined

        Connections {
            target: DepthView

            /* The FOV is vertical, so dividing by the root element's height gives us how many degrees are in one pixel of movement. */
            property real panRate: DepthView.surroundFOV / root.height

            onMouseMoved:
                if (imageMouseArea.surroundPanning != undefined) {
                    /* Update the pan value with the mouse delta multiplied by the panning rate. */
                    DepthView.surroundPan = Qt.point(DepthView.surroundPan.x - panRate * (imageMouseArea.surroundPanning.x - pos.x),
                                                     DepthView.surroundPan.y + panRate * (imageMouseArea.surroundPanning.y - pos.y))
                    imageMouseArea.surroundPanning = pos
                }
        }
        Binding {
            target: DepthView
            property: "surroundFOV"
            value: 120 * Math.pow(2, -targetScale)
        }
    }

    PinchArea {
        anchors.fill: parent

        enabled: !FolderListing.currentFileIsVideo

        property real startZoom

        onPinchStarted: {
            /* If zoom-to-fit is active retrieve the current scale before zooming in or out. */
            if (zoom < 0)
                zoom = targetScale

            startZoom = zoom
        }

        onPinchUpdated: {
            zoom = startZoom * pinch.scale
            zoom = Math.max(0.2, Math.min(zoom, 4.0))

            /* TODO - Use the pinch center. */
        }
    }
}
