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
#include <array>

DV_VRDriver::DV_VRDriver(DVWindow* w) : window(w) {
    window->settings.beginGroup("VRSettings");

    lockMouse = window->settings.contains("LockMouse") ? window->settings.value("LockMouse").toBool() : false;
    mirrorUI = window->settings.contains("MirrorUI") ? window->settings.value("MirrorUI").toBool() : true;
    snapSurroundPan = window->settings.contains("SnapSurroundPan") ? window->settings.value("SnapSurroundPan").toBool() : true;

    screenCurve = window->settings.contains("ScreenCurve") ? window->settings.value("ScreenCurve").toReal() : 0.5;
    screenSize = window->settings.contains("ScreenSize") ? window->settings.value("ScreenSize").toReal() : 4.0;
    screenDistance = window->settings.contains("ScreenDistance") ? window->settings.value("ScreenDistance").toReal() : 8.0;
    screenHeight = window->settings.contains("ScreenHeight") ? window->settings.value("ScreenHeight").toReal() : 2.0;
    renderSizeFac = window->settings.contains("RenderSizeFac") ? window->settings.value("RenderSizeFac").toReal() : 1.0;

    backgroundImage = window->settings.contains("BackgroundImage") ? window->settings.value("BackgroundImage").toUrl() : QUrl();
    backgroundPan = window->settings.contains("BackgroundPan") ? window->settings.value("BackgroundPan").toReal() : 0.0;
    backgroundDim = window->settings.contains("BackgroundDim") ? window->settings.value("BackgroundDim").toReal() : 0.0;
    backgroundSourceMode = window->settings.contains("BackgroundSourceMode") ?
                window->settings.value("BackgroundSourceMode").value<DVSourceMode::Type>() : DVSourceMode::Mono;

    window->settings.endGroup();
}

const DV_VRDriver::RayHit DV_VRDriver::screenTrace(const Ray& ray) const {
    RayHit hit;

    /* Do a triangle trace on the entire screen. */
    for (int i = 0; i + 2 < screen.size(); ++i)
        if (triangleTrace(ray, hit, {screen[i], screen[i+1], screen[i+2]}, {screenUV[i], screenUV[i+1], screenUV[i+2]})) break;

    return hit;
}

bool DV_VRDriver::triangleTrace(const Ray& ray, RayHit& hit, std::array<QVector3D, 3> triangle, std::array<QVector2D, 3> triangleUV) const {
    QVector3D v0v1 = triangle[1] - triangle[0];
    QVector3D v0v2 = triangle[2] - triangle[0];

    QVector3D pvec = QVector3D::crossProduct(ray.direction, v0v2);
    float det = QVector3D::dotProduct(v0v1, pvec);

    if (qAbs(det) < std::numeric_limits<float>::epsilon()) return false;

    float invDet = 1.0f / det;

    QVector3D tv = ray.origin - triangle[0];

    float t,a,b;

    a = QVector3D::dotProduct(tv, pvec) * invDet;
    if (a < 0.0f || a > 1.0f)
        return false;

    QVector3D qv = QVector3D::crossProduct(tv, v0v1);

    b = QVector3D::dotProduct(ray.direction, qv) * invDet;
    if (b < 0.0f || a + b > 1.0f)
        return false;

    t = QVector3D::dotProduct(v0v2, qv) * invDet;

    if (t <= 0.0f)
        return false;

    /* Convert the calculated values to a world position and UV coordinate. */
    hit.hitPoint = ray.origin + ray.direction * t;
    hit.uvCoord = triangleUV[1] * a + triangleUV[2] * b + triangleUV[0] * (1.0f - a - b);

    return hit.isValid = true;
}

DVVirtualScreenManager::DVVirtualScreenManager(DVWindow* parent) : QObject(parent) {
    p = nullptr;
    window = parent;

    /* Update screen geometry when any of the screen settings change. */
    connect(this, &DVVirtualScreenManager::screenCurveChanged, this, &DVVirtualScreenManager::updateScreen);
    connect(this, &DVVirtualScreenManager::screenSizeChanged, this, &DVVirtualScreenManager::updateScreen);
    connect(this, &DVVirtualScreenManager::screenDistanceChanged, this, &DVVirtualScreenManager::updateScreen);
    connect(this, &DVVirtualScreenManager::screenHeightChanged, this, &DVVirtualScreenManager::updateScreen);
}

DVVirtualScreenManager::~DVVirtualScreenManager() {
    deinit();
}

bool DVVirtualScreenManager::init() {
#ifdef DV_OPENVR
    p = DV_VRDriver::createOpenVRDriver(window);
#endif

    /* Update the settings values now that they've been loaded. */
    emit lockMouseChanged();
    emit mirrorUIChanged();
    emit snapSurroundPanChanged();
    emit screenCurveChanged();
    emit screenSizeChanged();
    emit screenDistanceChanged();
    emit screenHeightChanged();
    emit renderSizeFacChanged();
    emit backgroundImageChanged();
    emit backgroundSourceModeChanged();
    emit backgroundSwapChanged();
    emit backgroundPanChanged();
    emit backgroundDimChanged();
    emit backgroundImageTargetChanged();
    emit initedChanged();

    return p != nullptr && !p->errorString.isEmpty();
}

void DVVirtualScreenManager::deinit() {
    if (p != nullptr)
        delete p;
    p = nullptr;
}

QString DVVirtualScreenManager::getErrorString() {
    return p->errorString;
}

bool DVVirtualScreenManager::render() {
    return p != nullptr && p->render();
}

void DVVirtualScreenManager::frameSwapped() {
    if (p != nullptr) p->frameSwapped();
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

bool DVVirtualScreenManager::lockMouse() const {
    return p != nullptr && p->lockMouse;
}

void DVVirtualScreenManager::setLockMouse(bool lock) {
    window->settings.setValue("VRSettings/LockMouse", lock);

    if (p != nullptr && p->lockMouse != lock) {
        p->lockMouse = lock;
        emit lockMouseChanged();
    }
}

bool DVVirtualScreenManager::mirrorUI() const {
    return p != nullptr && p->mirrorUI;
}
void DVVirtualScreenManager::setMirrorUI(bool mirror) {
    window->settings.setValue("VRSettings/MirrorUI", mirror);

    if (p != nullptr && p->mirrorUI != mirror) {
        p->mirrorUI = mirror;
        emit mirrorUIChanged();
    }
}

bool DVVirtualScreenManager::snapSurroundPan() const {
    return p != nullptr && p->snapSurroundPan;
}
void DVVirtualScreenManager::setSnapSurroundPan(bool snap) {
    window->settings.setValue("VRSettings/SnapSurroundPan", snap);

    if (p != nullptr && p->snapSurroundPan != snap) {
        p->snapSurroundPan = snap;
        emit snapSurroundPanChanged();
    }
}

qreal DVVirtualScreenManager::screenCurve() const {
    return p != nullptr ? p->screenCurve : 0.0;
}
void DVVirtualScreenManager::setScreenCurve(qreal curve) {
    window->settings.setValue("VRSettings/ScreenCurve", curve);

    if (p != nullptr && p->screenCurve != curve) {
        p->screenCurve = curve;
        emit screenCurveChanged();
    }
}

qreal DVVirtualScreenManager::screenSize() const {
    return p != nullptr ? p->screenSize : 0.0;
}
void DVVirtualScreenManager::setScreenSize(qreal size) {
    window->settings.setValue("VRSettings/ScreenSize", size);

    if (p != nullptr && p->screenSize != size) {
        p->screenSize = size;
        emit screenSizeChanged();
    }
}

qreal DVVirtualScreenManager::screenDistance() const {
    return p != nullptr ? p->screenDistance : 0.0;
}
void DVVirtualScreenManager::setScreenDistance(qreal distance) {
    window->settings.setValue("VRSettings/ScreenDistance", distance);

    if (p != nullptr && p->screenDistance != distance) {
        p->screenDistance = distance;
        emit screenDistanceChanged();
    }
}

qreal DVVirtualScreenManager::screenHeight() const {
return p != nullptr ? p->screenHeight : 0.0;
}
void DVVirtualScreenManager::setScreenHeight(qreal height) {
    window->settings.setValue("VRSettings/ScreenHeight", height);

    if (p != nullptr && p->screenHeight != height) {
        p->screenHeight = height;
        emit screenHeightChanged();
    }
}

qreal DVVirtualScreenManager::renderSizeFac() const {
    return p != nullptr ? p->renderSizeFac : 0.0;
}
void DVVirtualScreenManager::setRenderSizeFac(qreal fac) {
    window->settings.setValue("VRSettings/RenderSizeFac", fac);

    if (p != nullptr && p->renderSizeFac != fac) {
        p->renderSizeFac = fac;
        emit renderSizeFacChanged();
    }
}

QUrl DVVirtualScreenManager::backgroundImage() const {
    return p != nullptr ? p->backgroundImage : QUrl();
}
void DVVirtualScreenManager::setBackgroundImage(QUrl image) {
    window->settings.setValue("VRSettings/BackgroundImage", image);

    if (p != nullptr && p->backgroundImage != image) {
        p->backgroundImage = image;
        emit backgroundImageChanged();
    }
}

DVSourceMode::Type DVVirtualScreenManager::backgroundSourceMode() const {
    return p != nullptr ? p->backgroundSourceMode : DVSourceMode::Mono;
}
void DVVirtualScreenManager::setBackgroundSourceMode(DVSourceMode::Type mode) {
    window->settings.setValue("VRSettings/BackgroundSourceMode", mode);

    if (p != nullptr && p->backgroundSourceMode != mode) {
        p->backgroundSourceMode = mode;
        emit backgroundSourceModeChanged();
    }
}

bool DVVirtualScreenManager::backgroundSwap() const {
    return p != nullptr && p->backgroundSwap;
}
void DVVirtualScreenManager::setBackgroundSwap(bool swap) {
    window->settings.setValue("VRSettings/BackgroundSwap", swap);

    if (p != nullptr && p->backgroundSwap != swap) {
        p->backgroundSwap = swap;
        emit backgroundSwapChanged();
    }
}

qreal DVVirtualScreenManager::backgroundPan() const {
    return p != nullptr ? p->backgroundPan : 0.0;
}
void DVVirtualScreenManager::setBackgroundPan(qreal pan) {
    window->settings.setValue("VRSettings/BackgroundPan", pan);

    if (p != nullptr && p->backgroundPan != pan) {
        p->backgroundPan = pan;
        emit backgroundPanChanged();
    }
}

qreal DVVirtualScreenManager::backgroundDim() const {
    return p != nullptr ? p->backgroundDim : 0.0;
}
void DVVirtualScreenManager::setBackgroundDim(qreal dim) {
    window->settings.setValue("VRSettings/BackgroundDim", dim);

    if (p != nullptr && p->backgroundDim != dim) {
        p->backgroundDim = dim;
        emit backgroundDimChanged();
    }
}

QQuickItem* DVVirtualScreenManager::backgroundImageTarget() const {
    return p != nullptr ? p->backgroundImageItem : nullptr;
}
void DVVirtualScreenManager::setBackgroundImageTarget(QQuickItem* target){
    if (p != nullptr && target != p->backgroundImageItem) {
        p->backgroundImageItem = target;
        emit backgroundImageTarget();
    }
}

bool DVVirtualScreenManager::isInited() const {
    return p != nullptr;
}
