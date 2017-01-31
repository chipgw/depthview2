#pragma once

#include <QQuickAsyncImageProvider>
#include <QQuickImageResponse>
#include <QtAV/VideoFrameExtractor.h>

class DVThumbnailProvider : public QQuickAsyncImageProvider {
public:
    DVThumbnailProvider();

    QQuickImageResponse* requestImageResponse(const QString& id, const QSize& requestedSize);
};

class DVThumbnailResponse : public QQuickImageResponse {
    Q_OBJECT

    bool hadError;

public:
    DVThumbnailResponse();

    QSize requestedSize;
    QSize originalSize;
    QtAV::VideoFrameExtractor frameExtractor;
    QImage image;

    QQuickTextureFactory* textureFactory() const;

    QString errorString() const;

public slots:
    void frameReceived(const QtAV::VideoFrame& frame);
    void frameError();
};
