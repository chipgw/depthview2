import QtQuick 2.5
import Qt.labs.folderlistmodel 2.1
import QtMultimedia 5.4
import DepthView 2.0

Item {
    id: root

    property FolderListModel model
    property real zoom: -1

    /* The index of the current file in the FolderListModel. */
    property int currentIndex

    function prevFile() {
        /* Decrease by one. If it is a folder, go to the end of the list. If it's STILL a folder there are no images. */
        if ((model.isFolder(--currentIndex) || currentIndex < 0) && model.isFolder(currentIndex = model.count - 1))
            return

        updateImage()
    }

    function nextFile() {
        /* Advance by one. If we're at the end of the list go back to the start. */
        if (++currentIndex >= folderModel.count)
            currentIndex = 0

        /* Skip over all folders. */
        while (model.isFolder(currentIndex))
            if (++currentIndex >= folderModel.count)
                /* No files in the folder! */
                return

        updateImage()
    }

    function updateImage() {
        source = model.get(currentIndex, "fileURL")
    }

    function playPause() {
        if (isVideo) {
            if (media.playbackState == MediaPlayer.PlayingState)
                media.pause()
            else
                media.play()
        }
    }

    property bool isPlaying: isVideo && media.playbackState == MediaPlayer.PlayingState

    property url source: "qrc:/images/test.pns"

    readonly property bool isVideo: media.hasVideo && media.hasAudio
    property alias videoPosition: media.position
    property alias videoDuration: media.duration
    property alias videoVolume: media.volume

    property int videoMode: SourceMode.Mono

    function seek(offset) {
        if (isVideo)
            media.seek(offset)
    }

    Flickable {
        id: imageFlickable
        anchors.fill: parent

        contentWidth: imageContainer.width
        contentHeight: imageContainer.height

        /* Only enable panning when the image is zoomed in enough. */
        interactive: (contentWidth > width || contentHeight > height)

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

                source: isVideo ? "" : root.source

                /* If zoom is negative we scale to fit, otherwise just use the value of zoom. */
                scale: (zoom < 0) ? Math.min(root.width / image.width, root.height / image.height) : zoom
            }
        }
    }

    Item {
        visible: isVideo

        anchors.centerIn: parent
        clip: true

        width: (videoMode == SourceMode.SidebySide || videoMode == SourceMode.SidebySideAnamorphic) ? vid.width / 2 : vid.width
        height: (videoMode == SourceMode.TopBottom || videoMode == SourceMode.TopBottomAnamorphic) ? vid.height / 2 : vid.height

        scale: (zoom < 0) ? Math.min(root.width / width, root.height / height) : zoom

        MediaPlayer {
            id: media
            source: root.source

            autoPlay: true
        }

        VideoOutput {
            id: vid
            source: media

            width: (videoMode == SourceMode.SidebySideAnamorphic) ? sourceRect.width * 2 : sourceRect.width
            height: (videoMode == SourceMode.TopBottomAnamorphic) ? sourceRect.height * 2 : sourceRect.height

            /* Always stretch. We set the VideoOutput to the size we want. */
            fillMode: VideoOutput.Stretch

            /* Same thing as the StereoImage does. Show half for each eye. */
            x: (!DepthView.isLeft && (videoMode == SourceMode.SidebySide || videoMode == SourceMode.SidebySideAnamorphic)) ? -width / 2 : 0
            y: (!DepthView.isLeft && (videoMode == SourceMode.TopBottom || videoMode == SourceMode.TopBottomAnamorphic)) ? -height / 2 :0
        }
    }

    MouseArea {
        anchors.fill: parent

        enabled: !isVideo

        acceptedButtons: Qt.MiddleButton

        /* Reset zoom on wheel double-click. */
        onDoubleClicked: zoom = -1;

        onWheel: {
            /* Don't zoom if covered. */
            if (!fileBrowser.visible) {
                /* If zoom-to-fit is active retrieve the current scale before zooming in or out. */
                if (zoom < 0)
                    zoom = image.scale

                zoom += wheel.angleDelta.y * image.scale * 0.001
                zoom = Math.max(0.2, Math.min(zoom, 4.0))
            }
        }
    }
    PinchArea {
        anchors.fill: parent

        enabled: !isVideo

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
}

