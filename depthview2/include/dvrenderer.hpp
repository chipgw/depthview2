#pragma once

#include <QOpenGLShaderProgram>
#include <QSettings>
#include <QDir>
#include <QOpenGLBuffer>
#include "dvinputinterface.hpp"

/* DepthView forward declarations. */
class DVQmlCommunication;
class DVFolderListing;
class DVPluginManager;
class DVVirtualScreenManager;
class DVInputInterface;

/* Qt forward declarations. */
class QQuickWindow;
class QOpenGLFramebufferObject;
class QSGTexture;

/* QtAV forward declarations. */
namespace QtAV {
class AVPlayer;
}

/* Vertex attrib locations. */
constexpr unsigned int vertex = 0;
constexpr unsigned int uv     = 1;

class DVRenderer : public QObject {
    Q_OBJECT

    QQuickWindow* window;

public:
    DVRenderer(QSettings& s, DVQmlCommunication& q, DVFolderListing& f);

    void setWindow(QQuickWindow* w);

    /* Get the FBO that QML is rendered to. */
    virtual const QOpenGLFramebufferObject& getInterfaceFramebuffer();

    /* Get the OpenGL textures for each eye. */
    virtual GLuint getInterfaceTexture(DVStereoEye::Type eye) const;

    /* Returns the texture handle the current image / video, and sets left & right to where on the texture each eye is. */
    QSGTexture* getCurrentTexture(QRectF& left, QRectF& right);

    /* Get the rectangles of a texture based on the source mode and swap. */
    void getTextureRects(QRectF& left, QRectF& right, QSGTexture* texture, bool swap, DVSourceMode::Type mode);

    /* Draw the default sphere (for surround images). */
    void renderStandardSphere();

    /* Draw the default fullscreen quad. */
    void renderStandardQuad();

    /* Set up the renderer exactly as all the built-in modes have it set up.
     * The left and right image textures will be bound to TEXTURE0 and TEXTURE1, respectively,
     * the viewport is set to the window size, and surround images will be rendered under the UI. */
    void doStandardSetup();

    QSettings& settings;

    DVQmlCommunication& qmlCommunication;
    DVFolderListing& folderListing;
    DVVirtualScreenManager* vrManager;
    DVInputInterface* input;

    /* The size of the FBO QML is being rendered to. */
    QSize qmlSize;

    QOpenGLContext* openglContext();

public slots:
    void updateQmlSize();
    void onFrameSwapped();

protected:
    void initializeGL();
    void shutdownGL();
    void paintGL();
    void preSync();

private:
    /* Shaders for built-in draw modes. */
    QOpenGLShaderProgram* shaderAnaglyph;
    QOpenGLShaderProgram* shaderSideBySide;
    QOpenGLShaderProgram* shaderTopBottom;
    QOpenGLShaderProgram* shaderInterlaced;
    QOpenGLShaderProgram* shaderMono;
    QOpenGLShaderProgram* shaderSphere;

    /* The FBO that QML renders to. */
    QOpenGLFramebufferObject* renderFBO;

    QOpenGLBuffer sphereVerts;
    QOpenGLBuffer sphereTris;
    GLuint sphereTriCount;

    bool holdMouse;

    void loadShaders();
    void loadShader(QOpenGLShaderProgram& shader, const char* vshader, const char* fshader);
    void createFBO();
};
