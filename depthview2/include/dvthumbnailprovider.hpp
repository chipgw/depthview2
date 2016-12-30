#pragma once

#include <QQuickAsyncImageProvider>

class DVThumbnailProvider : public QQuickAsyncImageProvider {
public:
    DVThumbnailProvider();

    QQuickImageResponse* requestImageResponse(const QString& id, const QSize& requestedSize);
};
