#pragma once

#include "dvenums.hpp"
#include <QObject>
#include <QSGTexture>
#include <QUrl>

class DV_VRDriver;
class DVInputInterface;
class DVQmlCommunication;
class DVFolderListing;
class DVRenderer;

class QQuickItem;
class QSettings;

class DVVirtualScreenManager : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool lockMouse READ lockMouse WRITE setLockMouse NOTIFY lockMouseChanged)
    Q_PROPERTY(bool mirrorUI READ mirrorUI WRITE setMirrorUI NOTIFY mirrorUIChanged)
    Q_PROPERTY(bool snapSurroundPan READ snapSurroundPan WRITE setSnapSurroundPan NOTIFY snapSurroundPanChanged)
    Q_PROPERTY(qreal screenCurve READ screenCurve WRITE setScreenCurve NOTIFY screenCurveChanged)
    Q_PROPERTY(qreal screenSize READ screenSize WRITE setScreenSize NOTIFY screenSizeChanged)
    Q_PROPERTY(qreal screenDistance READ screenDistance WRITE setScreenDistance NOTIFY screenDistanceChanged)
    Q_PROPERTY(qreal screenHeight READ screenHeight WRITE setScreenHeight NOTIFY screenHeightChanged)
    Q_PROPERTY(qreal renderSizeFac READ renderSizeFac WRITE setRenderSizeFac NOTIFY renderSizeFacChanged)
    Q_PROPERTY(QUrl backgroundImage READ backgroundImage WRITE setBackgroundImage NOTIFY backgroundImageChanged)
    Q_PROPERTY(DVSourceMode::Type backgroundSourceMode READ backgroundSourceMode WRITE setBackgroundSourceMode NOTIFY backgroundSourceModeChanged)
    Q_PROPERTY(bool backgroundSwap READ backgroundSwap WRITE setBackgroundSwap NOTIFY backgroundSwapChanged)
    Q_PROPERTY(qreal backgroundPan READ backgroundPan WRITE setBackgroundPan NOTIFY backgroundPanChanged)
    Q_PROPERTY(qreal backgroundDim READ backgroundDim WRITE setBackgroundDim NOTIFY backgroundDimChanged)

    Q_PROPERTY(QQuickItem* backgroundImageTarget READ backgroundImageTarget WRITE setBackgroundImageTarget NOTIFY backgroundImageTargetChanged)

    Q_PROPERTY(bool isInited READ isInited NOTIFY initedChanged)
    Q_PROPERTY(bool isError READ isError NOTIFY errorChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorChanged)

public:
    DVVirtualScreenManager(DVRenderer* parent, DVQmlCommunication& q, DVFolderListing& f);
    ~DVVirtualScreenManager();

    bool init();
    void deinit();

    QString getErrorString();

    bool render(DVInputInterface* input);

    QSize getRenderSize(const QSize& windowSize);

    bool isCurrentFileSurround() const;
    qreal surroundPan() const;
    void setSurroundPan(qreal pan);

    QPointF pointFromScreenUV(const QVector2D& uv) const;

    DVQmlCommunication& qmlCommunication;
    DVFolderListing& folderListing;

private:
    QSettings& settings;
    DV_VRDriver* driver;
    DVRenderer* renderer;

public slots:
    void updateScreen();
    void frameSwapped();

signals:
    void lockMouseChanged();
    void mirrorUIChanged();
    void snapSurroundPanChanged();
    void screenCurveChanged();
    void screenSizeChanged();
    void screenDistanceChanged();
    void screenHeightChanged();
    void renderSizeFacChanged();
    void backgroundImageChanged();
    void backgroundSourceModeChanged();
    void backgroundSwapChanged();
    void backgroundPanChanged();
    void backgroundDimChanged();
    void backgroundImageTargetChanged();

    void initedChanged();
    void errorChanged();

public:
    bool lockMouse() const;
    void setLockMouse(bool lock);

    bool mirrorUI() const;
    void setMirrorUI(bool mirror);

    bool snapSurroundPan() const;
    void setSnapSurroundPan(bool snap);

    qreal screenCurve() const;
    void setScreenCurve(qreal curve);

    qreal screenSize() const;
    void setScreenSize(qreal size);

    qreal screenDistance() const;
    void setScreenDistance(qreal distance);

    qreal screenHeight() const;
    void setScreenHeight(qreal height);

    qreal renderSizeFac() const;
    void setRenderSizeFac(qreal fac);

    QUrl backgroundImage() const;
    void setBackgroundImage(QUrl image);

    DVSourceMode::Type backgroundSourceMode() const;
    void setBackgroundSourceMode(DVSourceMode::Type mode);

    bool backgroundSwap() const;
    void setBackgroundSwap(bool swap);

    qreal backgroundPan() const;
    void setBackgroundPan(qreal pan);

    qreal backgroundDim() const;
    void setBackgroundDim(qreal dim);

    QQuickItem* backgroundImageTarget() const;
    void setBackgroundImageTarget(QQuickItem* target);

    bool isInited() const;
    bool isError() const;
    QString errorString() const;
};
