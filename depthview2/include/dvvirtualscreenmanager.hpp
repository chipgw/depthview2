#pragma once

#include "dvenums.hpp"
#include <QObject>
#include <QSGTexture>
#include <QUrl>

class DV_VRDriver;
class DVInputInterface;
class DVWindow;

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

public:
    DVVirtualScreenManager(DVWindow* parent);
    ~DVVirtualScreenManager();

    bool init();
    bool deinit();

    QString getErrorString();

    bool render();

    QSize getRenderSize(const QSize& windowSize);

    bool pollInput(DVInputInterface* inputInterface);

private:
    DV_VRDriver* p;
    DVWindow* window;

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

public:
    bool lockMouse();
    void setLockMouse(bool lock);

    bool mirrorUI();
    void setMirrorUI(bool mirror);

    bool snapSurroundPan();
    void setSnapSurroundPan(bool snap);

    qreal screenCurve();
    void setScreenCurve(qreal curve);

    qreal screenSize();
    void setScreenSize(qreal size);

    qreal screenDistance();
    void setScreenDistance(qreal distance);

    qreal screenHeight();
    void setScreenHeight(qreal height);

    qreal renderSizeFac();
    void setRenderSizeFac(qreal fac);

    QUrl backgroundImage();
    void setBackgroundImage(QUrl image);

    DVSourceMode::Type backgroundSourceMode();
    void setBackgroundSourceMode(DVSourceMode::Type mode);

    bool backgroundSwap();
    void setBackgroundSwap(bool swap);

    qreal backgroundPan();
    void setBackgroundPan(qreal pan);
};
