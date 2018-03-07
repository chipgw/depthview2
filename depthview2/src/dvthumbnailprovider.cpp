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
    /* This only works one at a time. */
    QMutexLocker locker(&waitReady);

    /* Wait for a bit to be safe, sometimes if you change the source too fast it causes problems. */
    QThread::msleep(100);

    qDebug("Thumbnail load started for \"%s\".", qPrintable(id));

    /* Reset the variables. */
    hadError = false;
    image = QImage();
    originalFrameSize = QSize();

    requestedFrameSize = requestedSize;
    frameExtractor.setSource(id);
    frameExtractor.extract();

    if (size)
        *size = originalFrameSize;

    return QQuickTextureFactory::textureFactoryForImage(image);
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
