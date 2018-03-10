#include "dvthumbnailprovider.hpp"
#include <QThread>

DVThumbnailProvider::DVThumbnailProvider() : QQuickImageProvider(QQmlImageProviderBase::Texture) {
    frameExtractor.setAutoExtract(false);
    frameExtractor.setAsync(false);

    /* We don't need it to be precise, and making it large reduces the chance that extraction will fail. */
    frameExtractor.setPrecision(500);

    /* TODO - Set dynamically based on video length... */
    frameExtractor.setPosition(60000);

    connect(&frameExtractor, &QtAV::VideoFrameExtractor::frameExtracted, this, &DVThumbnailProvider::frameReceived, Qt::DirectConnection);
    connect(&frameExtractor, &QtAV::VideoFrameExtractor::error, this, &DVThumbnailProvider::frameError, Qt::DirectConnection);
}

QQuickTextureFactory* DVThumbnailProvider::requestTexture(const QString& id, QSize* size, const QSize& requestedSize) {
    /* This only works for one thumbnail at a time. */
    QMutexLocker locker(&waitReady);

    qDebug("Thumbnail load started for \"%s\".", qPrintable(id));

    /* Reset the variables. */
    image = QImage();
    originalFrameSize = QSize();

    requestedFrameSize = requestedSize;
    frameExtractor.setSource(id);

    tryLoadThumbnail(4, id);

    if (size)
        *size = originalFrameSize;

    return QQuickTextureFactory::textureFactoryForImage(image);
}

bool DVThumbnailProvider::tryLoadThumbnail(int retries, const QString& id) {
    hadError = false;

    frameExtractor.extract();

    if (!hadError) return true;

    if (retries > 0) {
        qDebug("Error loading thumbnail for \"%s\"! Trying %i more times...", qPrintable(id), retries);
        return tryLoadThumbnail(--retries, id);
    }
    qDebug("Error loading thumbnail for \"%s\"! Giving up.", qPrintable(id));

    return false;
}

void DVThumbnailProvider::frameReceived(const QtAV::VideoFrame& frame) {
    originalFrameSize = frame.size();
    /* Scale the frame size to fit inside the requested size while maintaining aspect ratio. */
    image = frame.toImage(QImage::Format_RGB32, originalFrameSize.scaled(requestedFrameSize, Qt::KeepAspectRatio));

    qDebug("Thumbnail for %s loaded.", qPrintable(frameExtractor.source()));
}

void DVThumbnailProvider::frameError() {
    hadError = true;
}
