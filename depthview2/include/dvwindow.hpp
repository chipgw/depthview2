#pragma once

#include <QQuickWindow>
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

/* Qt forward declarations. */
class QQuickItem;

/* QtAV forward declarations. */
namespace QtAV {
class AVPlayer;
}

/* Vertex attrib locations. */
constexpr unsigned int vertex = 0;
constexpr unsigned int uv     = 1;

class DVWindow : public QQuickWindow, public DVInputInterface {
    Q_OBJECT

public:
    DVWindow();
    ~DVWindow();

    /* Parse command line arguments from QApplication. */
    void doCommandLine(class QCommandLineParser& parser);

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

    QSettings settings;

    DVQmlCommunication* qmlCommunication;
    DVFolderListing* folderListing;

    /* The size of the FBO QML is being rendered to. */
    QSize qmlSize;

    /* -------------------------------- *
     * Begin DVInputInterface functions *
     * -------------------------------- */

    /* Get the current input mode. */
    DVInputMode::Type inputMode() const;

    /* Navigation controls, used primarily in the file browser. */
    void left();
    void right();
    void up();
    void down();

    /* Accept the currently highlighted item. */
    void accept();
    /* Closes any and all popups, basically the same as pressing the escape key. */
    void cancel();

    /* Directly open the file browser. */
    void openFileBrowser();

    /* Navigation for file browser. */
    void goBack();
    void goForward();
    void goUp();

    /* Show the file info popup. */
    void fileInfo();

    /* Open the next/previous file in the current directory. */
    void nextFile();
    void previousFile();

    void zoomActual();
    void zoomFit();

    /* Video controls. */
    void playVideo();
    void pauseVideo();
    void playPauseVideo();
    void seekBack();
    void seekForward();
    void seekAmount(qint64 msec);

    void volumeUp();
    void volumeDown();
    void mute();
    void setVolume(qreal volume);

    /* When the volume control functions are called, they call these in the main thread. */
    Q_INVOKABLE void muteImpl();
    Q_INVOKABLE void setVolumeImpl(qreal volume);

    /* ------------------------------ *
     * End DVInputInterface functions *
     * ------------------------------ */

public slots:
    void updateQmlSize();
    void onFrameSwapped();

    void updateTitle();

protected:
    void initializeGL();
    void shutdownGL();
    void paintGL();
    void preSync();

    bool event(QEvent* event);

private:
    /* QML Stuff. */
    QQmlEngine* qmlEngine;
    QQuickItem* qmlRoot;

    DVPluginManager* pluginManager;
    DVVirtualScreenManager* vrManager;
    QtAV::AVPlayer* player;

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
