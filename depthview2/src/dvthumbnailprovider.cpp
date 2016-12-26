#include "include/dvthumbnailprovider.hpp"

DVThumbnailProvider::DVThumbnailProvider() : QQuickImageProvider(Image) {
    frameExtractor.setAsync(false);
    frameExtractor.setAutoExtract(false);

    /* We don't need it to be precise, and making it large reduces the chance that extraction will fail. */
    frameExtractor.setPrecision(500);

    /* TODO - Set dynamically based on video length... */
    frameExtractor.setPosition(60000);

    /* Connect directly so it is called before extract() returns. */
    connect(&frameExtractor, &QtAV::VideoFrameExtractor::frameExtracted, this, &DVThumbnailProvider::frameReceived, Qt::DirectConnection);

    retryLoad = true;
}

QImage DVThumbnailProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize) {
    /* Make sure only one thumbnail is loading at once.
     * (I'm not sure this is actually necessary, but it feels safer...) */
    loaderLock.lock();
    if (retryLoad) qDebug("Thumbnail load started for \"%s\".", qPrintable(id));

    lastRequestedSize = requestedSize;
    frameExtractor.setSource(id);

    /* This will block until frameReceived has been called,
     * unless an error occured in which case frameReceived will never be called. */
    frameExtractor.extract();

    if (lastFrame.isNull()) {
        qWarning("Failed to load thumbnail! %s", retryLoad ? "Retrying..." : "No retries left.");
        loaderLock.unlock();

        /* The frame extractor sometimes randomly fails, so as a workaround try a second time. */
        if (retryLoad) {
            /* So that if it fails again it won't retry. */
            retryLoad = false;
            return requestImage(id, size, requestedSize);
        }
        /* No more retries for this file, reset for the next one. */
        retryLoad = true;

        /* Return a null image. */
        return QImage();
    }

    /* Let QML know what the original size was. */
    *size = lastFrameOriginalSize;

    /* Move the image to a new local variable, this way it should return without copying and clear the image for the next request. */
    QImage frame;
    frame.swap(lastFrame);

    /* Make sure the retry tracker is set correctly for the next video. */
    retryLoad = true;

    /* The next image can load now. */
    loaderLock.unlock();

    qDebug("Thumbnail loaded successfully!");

    return frame;
}

void DVThumbnailProvider::frameReceived(const QtAV::VideoFrame& frame) {
    lastFrameOriginalSize = frame.size();
    /* Scale the frame size to fit inside the requested size while maintaining aspect ratio. */
    lastFrame = frame.toImage(QImage::Format_RGB32, lastFrameOriginalSize.scaled(lastRequestedSize, Qt::KeepAspectRatio));
}
