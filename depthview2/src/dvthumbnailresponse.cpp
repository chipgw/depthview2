#include "dvthumbnailresponse.hpp"

DVThumbnailResponse::DVThumbnailResponse() {
    frameExtractor.setAutoExtract(false);

    /* We don't need it to be precise, and making it large reduces the chance that extraction will fail. */
    frameExtractor.setPrecision(500);

    /* TODO - Set dynamically based on video length... */
    frameExtractor.setPosition(60000);

    connect(&frameExtractor, &QtAV::VideoFrameExtractor::frameExtracted, this, &DVThumbnailResponse::frameReceived);
    connect(&frameExtractor, &QtAV::VideoFrameExtractor::error, this, &DVThumbnailResponse::frameError);

    hadError = false;
}

void DVThumbnailResponse::frameReceived(const QtAV::VideoFrame& frame) {
    originalSize = frame.size();
    /* Scale the frame size to fit inside the requested size while maintaining aspect ratio. */
    image = frame.toImage(QImage::Format_RGB32, originalSize.scaled(requestedSize, Qt::KeepAspectRatio));

    qDebug("Thumbnail for %s loaded.", qPrintable(frameExtractor.source()));

    emit finished();
}

void DVThumbnailResponse::frameError() {
    hadError = true;
}

QQuickTextureFactory* DVThumbnailResponse::textureFactory() const {
    return QQuickTextureFactory::textureFactoryForImage(image);
}

QString DVThumbnailResponse::errorString() const {
    return hadError ? "Error extracting frame!" : QString();
}
