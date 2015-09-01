import QtQuick 2.5
import Qt.labs.folderlistmodel 2.1

Item {
    id: root

    property FolderListModel model

    /* The index of the current file in the FolderListModel. */
    property int currentIndex

    function prevFile() {
        var index = currentIndex
        if (model.isFolder(--index) || index < 0) {
            index = model.count - 1

            if (model.isFolder(index))
                /* If it's STILL a folder there are no images. */
                return
        }

        currentIndex = index
    }
    function nextFile() {
        var index = currentIndex
        if (++index >= folderModel.count)
            index = 0

        while (model.isFolder(index)) {
            if (++index >= folderModel.count)
                /* No files in the folder! */
                return
        }
        currentIndex = index
    }

    onCurrentIndexChanged: {
        image.source = model.get(currentIndex, "fileURL")
    }

    Flickable {
        id: imageFlickable
        anchors.fill: parent

        contentWidth: imageContainer.width
        contentHeight: imageContainer.height

        /* Only enable panning when the image is zoomed in enough. */
        interactive: (contentWidth > width || contentHeight > height)

        /* The location that the center of the screen is focused on. */
        property point oldCenter: Qt.point(0.5, 0.5)

        /* Used to track differences in the visible screen area. */
        property size oldSize
        property size oldContentSize

        visibleArea.onHeightRatioChanged: {
            /* Only run if  height or contentHeight has changed and niether is zero. */
            if ((oldSize.height != height || oldContentSize.height != contentHeight) && height != 0 && contentHeight != 0) {
                /* Find out the difference between how many pixels were hidden and how many are hidden now. */
                var hidden = (oldContentSize.height - oldSize.height) - (contentHeight - height)

                /* Multiply by the calculated center to get the offset needed to maintain center. */
                contentY -= hidden * oldCenter.y

                /* Make sure we didn't get out of bounds. Needed when zooming out. */
                returnToBounds()

                /* Store the values for next time. */
                oldCenter.y = visibleArea.yPosition + visibleArea.heightRatio / 2
                oldContentSize.height = contentHeight
                oldSize.height = height
            }
        }
        visibleArea.onWidthRatioChanged: {
            /* Only run if either width or contentWidth has changed and niether is zero. */
            if ((oldSize.width != width || oldContentSize.width != contentWidth) && width != 0 && contentWidth != 0) {
                /* Find out the difference between how many pixels were hidden and how many are hidden now. */
                var hidden = (oldContentSize.width - oldSize.width) - (contentWidth - width)

                /* Multiply by the calculated center to get the offset needed to maintain center. */
                contentX -= hidden * oldCenter.x

                /* Make sure we didn't get out of bounds. Needed when zooming out. */
                returnToBounds()

                /* Store the values for next time. */
                oldCenter.x = visibleArea.xPosition + visibleArea.widthRatio / 2
                oldContentSize.width = contentWidth
                oldSize.width = width
            }
        }

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

