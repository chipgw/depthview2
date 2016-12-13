import QtQuick 2.5
import QtQuick.Controls 2.0
import QtAV 1.6
import DepthView 2.0

Item {
    id: root

    property real zoom: -1

    Connections {
        target: FolderListing

        onCurrentFileChanged: {
            /* Make sure the current mode is up to date. */
            stereoMode = FolderListing.currentFileStereoMode
            /* Reset video settings. */
            seek(0);
        }
    }

    function playPause() {
        if (FolderListing.currentFileIsVideo) {
            if (media.playbackState == MediaPlayer.PlayingState)
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

    property bool isPlaying: FolderListing.currentFileIsVideo && media.playbackState == MediaPlayer.PlayingState

    property url source: FolderListing.currentURL

    property alias videoPosition: media.position
    property alias videoDuration: media.duration
    property alias videoVolume: media.volume

    property string mediaInfo: FolderListing.currentFileInfo + (FolderListing.currentFileIsVideo ?
                                   "<br>Duration: " + timeString(media.metaData.duration) +
                                   "<h2>Video Info:</h2>" +
                                   "Codec: " + media.metaData.videoCodec +
                                   "<br>Frame Rate: " + media.metaData.videoFrameRate +
                                   "<br>Bit Rate: " + media.metaData.videoBitRate +
                                   (media.metaData.resolution ? "<br>Resolution: " + media.metaData.resolution.width + "x" + media.metaData.resolution.height : "") +
                                   "<br>Pixel Format: " + media.metaData.pixelFormat +
                                   "<h2>Audio Info:</h2>" +
                                   "Codec: " + media.metaData.audioCodec +
                                   "<br>Bit Rate: " + media.metaData.audioBitRate
                                 : "<br>Resolution: " + image.width + "x" + image.height)
                                 + "<hr>"

    property int stereoMode: FolderListing.currentFileStereoMode

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
            if (oldVisibleArea.height != visibleArea.heightRatio && height > 0) {
                /* Find the maximum possible contentY. */
                var hidden = contentHeight - height

                /* Scale the stored center to match the new visible area. */
                var center = currentCenter.y * (oldVisibleArea.height / visibleArea.heightRatio)

                /* Put center from the range [-hidden/2, hidden/2] to [0, hidden] and make sure it doesn't exceed that range. */
                contentY = Math.max(0, Math.min(hidden * 0.5 + center, hidden))

                /* When contentY is zero the center isn't updated automatically. */
                if (contentY == 0)
                    onContentYChanged()

                /* Store the values for next time. */
                oldVisibleArea.height = visibleArea.heightRatio
            }
        }

        /* Current center is the offset in pixels from the image center. */
        onContentYChanged: currentCenter.y = contentY - (contentHeight - height) * 0.5

        visibleArea.onWidthRatioChanged: {
            /* Only run if visible area has actually changed and width is valid. */
            if (oldVisibleArea.width != visibleArea.widthRatio && width > 0) {
                /* Find the maximum possible contentX. */
                var hidden = contentWidth - width

                /* Scale the stored center to match the new visible area. */
                var center = currentCenter.x * (oldVisibleArea.width / visibleArea.widthRatio)

                /* Put center from the range [-hidden/2, hidden/2] to [0, hidden] and make sure it doesn't exceed that range. */
                contentX = Math.max(0, Math.min(hidden * 0.5 + center, hidden))

                /* When contentX is zero the center isn't updated automatically. */
                if (contentX == 0)
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
                scale: (zoom < 0) ? Math.min(root.width / image.width, root.height / image.height) : zoom
            }
        }
    }

    Item {
        visible: FolderListing.currentFileIsVideo

        anchors.centerIn: parent

        width: (stereoMode == SourceMode.SidebySide || stereoMode == SourceMode.SidebySideAnamorphic) ? vid.width / 2 : vid.width
        height: (stereoMode == SourceMode.TopBottom || stereoMode == SourceMode.TopBottomAnamorphic) ? vid.height / 2 : vid.height

        scale: (zoom < 0) ? Math.min(root.width / width, root.height / height) : zoom

        MediaPlayer {
            id: media
            source: FolderListing.currentFileIsVideo ? root.source : ""

            autoPlay: true
        }

        VideoOutput2 {
            id: vid
            source: media

            width: (stereoMode == SourceMode.SidebySideAnamorphic) ? sourceRect.width * 2 : sourceRect.width
            height: (stereoMode == SourceMode.TopBottomAnamorphic) ? sourceRect.height * 2 : sourceRect.height

            /* Always stretch. We set the VideoOutput to the size we want. */
            fillMode: VideoOutput.Stretch

            /* Hide the video, just use it as a source for the ShaderEffect. */
            opacity: 0
        }

        StereoShader {
            target: vid
            stereoMode: root.stereoMode

            /* Videos tend to be the other way around from images... */
            swap: !DepthView.swapEyes;
        }
    }

    MouseArea {
        anchors.fill: parent

        acceptedButtons: Qt.MiddleButton | Qt.ForwardButton | Qt.BackButton

        /* Reset zoom on wheel double-click. */
        onDoubleClicked: if (mouse.button == Qt.MiddleButton) zoom = (zoom == -1) ? 1 : -1;

        onWheel: {
            /* Don't zoom or seek if covered. */
            if (!fileBrowser.visible) {
                if (FolderListing.currentFileIsVideo) {
                    if (wheel.angleDelta.y > 0)
                        media.seekForward()
                    else if (wheel.angleDelta.y < 0)
                        media.seekBackward()
                } else if (wheel.angleDelta.y != 0) {
                    /* If zoom-to-fit is active retrieve the current scale before zooming in or out. */
                    if (zoom < 0)
                        zoom = image.scale

                    zoom += wheel.angleDelta.y * image.scale * 0.001
                    zoom = Math.max(0.2, Math.min(zoom, 4.0))
                }

                /* Sideways scroll goes through files. */
                if (wheel.angleDelta.x > 0)
                    FolderListing.openPrevious()
                if (wheel.angleDelta.x < 0)
                    FolderListing.openNext()
            }
        }
        onClicked: {
            if (mouse.button == Qt.BackButton)
                FolderListing.openPrevious()
            if (mouse.button == Qt.ForwardButton)
                FolderListing.openNext()
        }
    }

    PinchArea {
        anchors.fill: parent

        enabled: !FolderListing.currentFileIsVideo

        property real startZoom

        onPinchStarted: {
            /* If zoom-to-fit is active retrieve the current scale before zooming in or out. */
            if (zoom < 0)
                zoom = image.scale

            startZoom = zoom
        }

        onPinchUpdated: {
            zoom = startZoom * pinch.scale
            zoom = Math.max(0.2, Math.min(zoom, 4.0))

            /* TODO - Use the pinch center. */
        }
    }

    Connections {
        target: DepthView

        onPlayVideo: media.play()
        onPauseVideo: media.pause()
        onPlayPauseVideo: playPause()
        onSeekBack: media.seekBackward()
        onSeekForward: media.seekForward()
        onSeekAmount: media.seek(msec)
    }
}

