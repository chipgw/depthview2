#pragma once

#include <QQuickImageProvider>
#include <QMutex>
#include <QtAV/VideoFrameExtractor.h>

class DVThumbnailProvider : public QObject, public QQuickImageProvider {
    Q_OBJECT

public:
    DVThumbnailProvider();

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize);

private:
    QtAV::VideoFrameExtractor frameExtractor;
    QImage lastFrame;
    QSize lastFrameOriginalSize;
    QSize lastRequestedSize;
    QMutex loaderLock;
    bool retryLoad;

public slots:
    void frameReceived(const QtAV::VideoFrame& frame);
};
