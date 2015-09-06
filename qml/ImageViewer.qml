import QtQuick 2.5
import Qt.labs.folderlistmodel 2.1

Item {
    id: root

    property FolderListModel model

    /* The index of the current file in the FolderListModel. */
    property int currentIndex

    function prevFile() {
        /* Use a local so that it doesn't try to load anything until the end. */
        var index = currentIndex

        /* Decrease by one. If it is a folder, go to the end of the list. If it's STILL a folder there are no images. */
        if ((model.isFolder(--index) || index < 0) && model.isFolder(index = model.count - 1))
            return

        currentIndex = index
    }
    function nextFile() {
        /* Use a local so that it doesn't try to load anything until the end. */
        var index = currentIndex

        /* Advance by one. If we're at the end of the list go back to the start. */
        if (++index >= folderModel.count)
            index = 0

        /* Skip over all folders. */
        while (model.isFolder(index))
            if (++index >= folderModel.count)
                /* No files in the folder! */
                return

        currentIndex = index
    }

    /* Whenever the index, we changes load the new image. */
    onCurrentIndexChanged: image.source = model.get(currentIndex, "fileURL")

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
            }
        }
    }

    MouseArea {
        anchors.fill: parent

        acceptedButtons: Qt.MiddleButton

        onWheel: {
            /* Don't zoom if covered. */
            if (!fileBrowser.visible) {
                image.scale += wheel.angleDelta.y * image.scale * 0.001
                image.scale = Math.max(0.2, Math.min(image.scale, 4.0))
            }
        }
    }
}

