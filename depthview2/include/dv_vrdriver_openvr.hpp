#include "dv_vrdriver.hpp"
#include "dvrenderer.hpp"
#include "dvinputinterface.hpp"
#include <QOpenGLExtraFunctions>
#include <QSGTextureProvider>
#include <QQuickItem>
#include <QVector>
#include <QVector2D>
#include <QVector3D>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QDebug>
#include <QMap>
#include <QThread>
#include <QBitArray>
#include <QtMath>
#include <openvr.h>

struct ModelComponent {
    QOpenGLBuffer VBO;
    QOpenGLBuffer IBO;

    ModelComponent(const vr::RenderModel_t& model, const vr::RenderModel_TextureMap_t& tex, QOpenGLExtraFunctions* f)
        : VBO(QOpenGLBuffer::VertexBuffer), IBO(QOpenGLBuffer::IndexBuffer), texture(QOpenGLTexture::Target2D) {
        /* Upload the verts to the GPU. */
        VBO.create(); VBO.bind();
        VBO.allocate(model.rVertexData, int(sizeof(vr::RenderModel_Vertex_t) * model.unVertexCount));
        /* Upload the indexes to the GPU. */
        IBO.create(); IBO.bind();
        IBO.allocate(model.rIndexData, int(sizeof(uint16_t) * model.unTriangleCount * 3));

        vertexCount = GLsizei(model.unTriangleCount * 3);

        /* Prepare to upload the texture. */
        texture.create(); texture.bind();
        f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.unWidth, tex.unHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.rubTextureMapData);

        /* We want mipmaps. */
        f->glGenerateMipmap(GL_TEXTURE_2D);
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        GLfloat largest;
        f->glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &largest);
        f->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largest);
    }

    QOpenGLTexture texture;
    GLsizei vertexCount;
};

class DV_VRDriver_OpenVR : public DV_VRDriver {
    /* Model name -> (component name -> component model data). */
    QMap<QByteArray, QMap<QByteArray, ModelComponent*>> renderModels;

    /* List of all loaded model components by their component model name. */
    QMap<QByteArray, ModelComponent*> loadedComponents;

    /* Tracked device -> model name. */
    QByteArray modelForDevice[vr::k_unMaxTrackedDeviceCount];

    /* Which device are we using to simulate mouse events? */
    uint32_t mouseDevice = vr::k_unTrackedDeviceIndexInvalid;
    Qt::MouseButtons mouseButtonsDown;
    bool wasLastHitValid = false;

    vr::VRControllerState_t controllerStates[vr::k_unMaxTrackedDeviceCount];

    vr::TrackedDevicePose_t trackedDevicePose[vr::k_unMaxTrackedDeviceCount];

    QOpenGLTexture* lineTexture;

    QVector3D panTrackingVector;

    typedef void (DVInputInterface::*Action)();

    enum AxisAsButton { PositiveX, NegativeX, PositiveY, NegativeY, AxisAsButton_Max };

    /* First index = input more, second index = whether the device is the mouse device, third index = the button. */
    Action buttonActions[3][2][vr::k_EButton_Max];
    Action axisActions[3][2][AxisAsButton_Max];

public:
    DV_VRDriver_OpenVR(DVRenderer* w, DVVirtualScreenManager* m);

    ~DV_VRDriver_OpenVR();

    bool initVRSystem(QOpenGLExtraFunctions* f);

    RayHit deviceScreenPoint(vr::TrackedDeviceIndex_t dev);

    QBitArray getAxisAsButtons(vr::TrackedDeviceIndex_t device, int axis, const vr::VRControllerState_t& newState, float threshold);

    void sendMousePress(const QPointF& point, Qt::MouseButton button, DVInputInterface* input);

    void sendMouseRelease(const QPointF& point, Qt::MouseButton button, DVInputInterface* input);

    void sendMouseMove(const QPointF& point, DVInputInterface* input);

    void handleVREvents(QOpenGLExtraFunctions* f, DVInputInterface* input);

    QMatrix4x4 getComponentMatrix(uint32_t device, const char* componentName, bool render = true);

    void calculateEyeDistortion(vr::EVREye eye, QVector<QVector2D>& verts, QVector<GLushort>& indexes, GLushort offset);

    void renderEyeScene(vr::EVREye eye, const QMatrix4x4& head, QSGTexture* imgTexture, QRectF imgRect, qreal imgPan, bool isBackground, QOpenGLExtraFunctions* f);

    bool renderEyeDistortion(vr::EVREye eye, QOpenGLExtraFunctions* f);

    bool render(QOpenGLExtraFunctions* f, DVInputInterface* input);

    void frameSwapped();

    QOpenGLFramebufferObject* renderFBO[2];
    QOpenGLFramebufferObject* resolveFBO[2];

    QOpenGLBuffer distortionVBO;
    QOpenGLBuffer distortionIBO;
    GLsizei distortionNumIndexes;

    vr::IVRSystem* vrSystem = nullptr;

    QOpenGLShaderProgram vrSceneShader;
    QOpenGLShaderProgram distortionShader;

    /* Get a tracked device property string. */
    QByteArray getTrackedDeviceString(vr::TrackedDeviceIndex_t deviceIndex, vr::TrackedDeviceProperty prop);

    void setupDeviceModel(vr::TrackedDeviceIndex_t deviceIndex, QOpenGLExtraFunctions* f);
};
