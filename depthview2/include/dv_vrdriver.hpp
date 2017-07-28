#include "dvvirtualscreenmanager.hpp"
#include <QVector>
#include <QVector2D>
#include <QVector3D>

class QQuickItem;

class DV_VRDriver {
protected:
    DVWindow* window;

    /* Only used by subclasses. */
    DV_VRDriver(DVWindow* w);

public:
    virtual bool render() = 0;

    virtual void frameSwapped() = 0;

    uint32_t renderWidth, renderHeight;

    QQuickItem* backgroundImageItem = nullptr;

    bool lockMouse;
    bool mirrorUI;
    bool snapSurroundPan;
    qreal screenCurve;
    qreal screenSize;
    qreal screenDistance;
    qreal screenHeight;
    qreal renderSizeFac;
    QUrl backgroundImage;
    DVSourceMode::Type backgroundSourceMode;
    bool backgroundSwap;
    qreal backgroundPan;

    QVector<QVector3D> screen;
    QVector<QVector2D> screenUV;

    QString errorString;

#ifdef DV_OPENVR
    static DV_VRDriver* createOpenVRDriver(DVWindow* window);
#endif
};

