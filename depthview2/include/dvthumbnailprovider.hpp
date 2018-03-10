#pragma once

#include <QQuickImageProvider>
#include <QMutex>
#include <QtAV/VideoFrameExtractor.h>

class DVThumbnailProvider : public QObject, public QQuickImageProvider {
    Q_OBJECT

    QImage image;

    QtAV::VideoFrameExtractor frameExtractor;
    bool hadError;

    QSize requestedFrameSize;
    QSize originalFrameSize;

    QMutex waitReady;

public:
    DVThumbnailProvider();

    virtual QQuickTextureFactory* requestTexture(const QString& id, QSize* size, const QSize& requestedSize) override;

    bool tryLoadThumbnail(int retries, const QString& id);

public slots:
    void frameReceived(const QtAV::VideoFrame& frame);
    void frameError();
};
