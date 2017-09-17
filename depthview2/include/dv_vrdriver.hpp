#include "dvvirtualscreenmanager.hpp"
#include <QVector>
#include <QVector2D>
#include <QVector3D>

class QQuickItem;
class QOpenGLExtraFunctions;

class DV_VRDriver {
protected:
    DVWindow* window;
    DVVirtualScreenManager* manager;

    /* Only used by subclasses. */
    DV_VRDriver(DVWindow* w, DVVirtualScreenManager* m);

    struct Ray {
        QVector3D origin;
        QVector3D direction;
    };

    struct RayHit {
        Ray ray;
        QVector2D uvCoord;
        QVector3D hitPoint;
        bool isValid = false;
    };

    const RayHit screenTrace(const Ray& ray) const;
    bool triangleTrace(RayHit& hit, std::array<QVector3D, 3> triangle, std::array<QVector2D, 3> triangleUV) const;

public:
    virtual ~DV_VRDriver() = default;
    virtual bool render(QOpenGLExtraFunctions* f, DVInputInterface* input) = 0;

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
    qreal backgroundDim;

    QVector<QVector3D> screen;
    QVector<QVector2D> screenUV;

    QString errorString;

    bool setError(const QString& error);

#ifdef DV_OPENVR
    static DV_VRDriver* createOpenVRDriver(DVWindow* window, DVVirtualScreenManager *manager);
#endif
};

