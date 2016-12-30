#include "dvthumbnailprovider.hpp"
#include "dvthumbnailresponse.hpp"

DVThumbnailProvider::DVThumbnailProvider() : QQuickAsyncImageProvider() { }

QQuickImageResponse* DVThumbnailProvider::requestImageResponse(const QString& id, const QSize& requestedSize) {
    qDebug("Thumbnail load started for \"%s\".", qPrintable(id));

    DVThumbnailResponse* response = new DVThumbnailResponse;

    response->requestedSize = requestedSize;
    response->frameExtractor.setSource(id);
    response->frameExtractor.extract();

    return response;
}
