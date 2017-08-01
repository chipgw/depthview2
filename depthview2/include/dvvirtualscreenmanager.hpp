#pragma once

#include "dvenums.hpp"
#include <QObject>
#include <QSGTexture>
#include <QUrl>

class DV_VRDriver;
class DVInputInterface;
class DVWindow;
class QQuickItem;

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

    Q_PROPERTY(QQuickItem* backgroundImageTarget READ backgroundImageTarget WRITE setBackgroundImageTarget NOTIFY backgroundImageTargetChanged)
    Q_PROPERTY(bool isInited READ isInited NOTIFY initedChanged)

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
    void backgroundImageTargetChanged();

    void initedChanged();

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

    QQuickItem* backgroundImageTarget() const;
    void setBackgroundImageTarget(QQuickItem* target);

    bool isInited() const;
};
