#include "dvvirtualscreenmanager.hpp"
#include "dvwindow.hpp"
#include "dv_vrdriver.hpp"
#include <QOpenGLExtraFunctions>
#include <QSGTextureProvider>
#include <QQuickItem>
#include <QVector>
#include <QVector2D>
#include <QVector3D>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QOpenGLBuffer>
#include <QDebug>
#include <cmath>

DV_VRDriver::DV_VRDriver(DVWindow *w) : window(w) {
    /* TODO - Load from settings. */
    lockMouse = false;
    mirrorUI = true;
    snapSurroundPan = true;
    screenCurve = 0.0;
    screenSize = 7.0;
    screenDistance = 8.0;
    screenHeight = 2.0;
    renderSizeFac = 1.0;
}

DVVirtualScreenManager::DVVirtualScreenManager(DVWindow *parent) : QObject(parent) {
    p = nullptr;
    window = parent;
}

DVVirtualScreenManager::~DVVirtualScreenManager() {
    deinit();
}

bool DVVirtualScreenManager::init() {
#ifdef DV_OPENVR
    p = DV_VRDriver::createOpenVRDriver(window);
#endif
    return p != nullptr && !p->errorString.isEmpty();
}

bool DVVirtualScreenManager::deinit() {
    if (p != nullptr)
        delete p;

    return !p->errorString.isEmpty();
}

QString DVVirtualScreenManager::getErrorString() {
    return p->errorString;
}

bool DVVirtualScreenManager::render() {
    /* TODO - Update only when the render size changes. */
    updateScreen();
    return p != nullptr && p->render();
}

void DVVirtualScreenManager::frameSwapped() {
    if (p != nullptr)
        p->frameSwapped();
}

QSize DVVirtualScreenManager::getRenderSize(const QSize& windowSize) {
    return windowSize * ((p != nullptr && p->renderSizeFac > 0.5) ? p->renderSizeFac : 1.0);
}

float interpolate(float v1, float v2, float a) {
    return v1 * a + v2 * (1.0f-a);
}

void DVVirtualScreenManager::updateScreen() {
    if (p == nullptr)
        return;

    /* Get the properties from QML. */
    float distance = p->screenDistance;
    float z = p->screenHeight;
    float size = p->screenSize;

    float height = size / (float(window->width()) / float(window->height()));

    /* Get rid of any previous geometry. */
    p->screen.clear();
    p->screenUV.clear();

    float curviness = p->screenCurve;

    constexpr int halfSteps = 50;

    /* size/distance gives the angle of half the screen, so this is angle of each step.  */
    float step = size / (distance * halfSteps);

    for (int current = -halfSteps; current <= halfSteps; ++current) {
        /* Interpolate between the cylindrical coordinate and the flat coordinate based on curviness. */
        float xCoord = interpolate(std::sin(current * step) * distance, current * size / halfSteps, curviness);
        float yCoord = interpolate(std::cos(current * step) * -distance, -distance, curviness);

        /* Calculate the coordinate of the vertex. */
        p->screen += {{xCoord, z - height, yCoord}, {xCoord, z + height, yCoord}};

        /* Map current from [-halfStep, halfStep] to [0, 1]. */
        float U = 0.5f * current / halfSteps + 0.5f;

        p->screenUV += {{U, 0.0f}, {U, 1.0f}};
    }
}

bool DVVirtualScreenManager::pollInput(DVInputInterface*) {
    /* TODO - Use tracked motion controllers... */
    return false;
}

bool DVVirtualScreenManager::lockMouse() {
    return p != nullptr && p->lockMouse;
}

void DVVirtualScreenManager::setLockMouse(bool lock) {
    if (p != nullptr && p->lockMouse != lock) {
        p->lockMouse = lock;
        emit lockMouseChanged();
    }
}

bool DVVirtualScreenManager::mirrorUI() {
    return p != nullptr && p->mirrorUI;
}
void DVVirtualScreenManager::setMirrorUI(bool mirror) {
    if (p != nullptr && p->mirrorUI != mirror) {
        p->mirrorUI = mirror;
        emit mirrorUIChanged();
    }
}

bool DVVirtualScreenManager::snapSurroundPan() {
    return p != nullptr && p->snapSurroundPan;
}
void DVVirtualScreenManager::setSnapSurroundPan(bool snap) {
    if (p != nullptr && p->snapSurroundPan != snap) {
        p->snapSurroundPan = snap;
        emit snapSurroundPanChanged();
    }
}

qreal DVVirtualScreenManager::screenCurve() {
    return p != nullptr ? p->screenCurve : 0.0;
}
void DVVirtualScreenManager::setScreenCurve(qreal curve) {
    if (p != nullptr && p->screenCurve != curve) {
        p->screenCurve = curve;
        emit screenCurveChanged();
    }
}

qreal DVVirtualScreenManager::screenSize() {
    return p != nullptr ? p->screenSize : 0.0;
}
void DVVirtualScreenManager::setScreenSize(qreal size) {
    if (p != nullptr && p->screenSize != size) {
        p->screenSize = size;
        emit screenSizeChanged();
    }
}

qreal DVVirtualScreenManager::screenDistance() {
    return p != nullptr ? p->screenDistance : 0.0;
}
void DVVirtualScreenManager::setScreenDistance(qreal distance) {
    if (p != nullptr && p->screenDistance != distance) {
        p->screenDistance = distance;
        emit screenDistanceChanged();
    }
}

qreal DVVirtualScreenManager::screenHeight() {
return p != nullptr ? p->screenHeight : 0.0;
}
void DVVirtualScreenManager::setScreenHeight(qreal height) {
    if (p != nullptr && p->screenHeight != height) {
        p->screenHeight = height;
        emit screenHeightChanged();
    }
}

qreal DVVirtualScreenManager::renderSizeFac() {
    return p != nullptr ? p->renderSizeFac : 0.0;
}
void DVVirtualScreenManager::setRenderSizeFac(qreal fac) {
    if (p != nullptr && p->renderSizeFac != fac) {
        p->renderSizeFac = fac;
        emit renderSizeFacChanged();
    }
}

QUrl DVVirtualScreenManager::backgroundImage() {
    return p != nullptr ? p->backgroundImage : QUrl();
}
void DVVirtualScreenManager::setBackgroundImage(QUrl image) {
    if (p != nullptr && p->backgroundImage != image) {
        p->backgroundImage = image;
        emit backgroundImageChanged();
    }
}

DVSourceMode::Type DVVirtualScreenManager::backgroundSourceMode() {
    return p != nullptr ? p->backgroundSourceMode : DVSourceMode::Mono;
}
void DVVirtualScreenManager::setBackgroundSourceMode(DVSourceMode::Type mode) {
    if (p != nullptr && p->backgroundSourceMode != mode) {
        p->backgroundSourceMode = mode;
        emit backgroundSourceModeChanged();
    }
}

bool DVVirtualScreenManager::backgroundSwap() {
    return p != nullptr && p->backgroundSwap;
}
void DVVirtualScreenManager::setBackgroundSwap(bool swap) {
    if (p != nullptr && p->backgroundSwap != swap) {
        p->backgroundSwap = swap;
        emit backgroundSwapChanged();
    }
}

qreal DVVirtualScreenManager::backgroundPan() {
    return p != nullptr && p->backgroundSwap;
}
void DVVirtualScreenManager::setBackgroundPan(qreal pan) {
    if (p != nullptr && p->backgroundPan != pan) {
        p->backgroundPan = pan;
        emit backgroundPanChanged();
    }
}
