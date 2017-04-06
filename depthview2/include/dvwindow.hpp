#pragma once

#include <QOpenGLWindow>
#include <QOpenGLShaderProgram>
#include <QSettings>
#include <QDir>
#include <QOpenGLBuffer>
#include "dvinputinterface.hpp"
#include "dvrenderinterface.hpp"

/* DepthView forward declarations. */
class DVQmlCommunication;
class DVFolderListing;
class DVPluginManager;

/* Qt forward declarations. */
class QQuickRenderControl;
class QQuickWindow;
class QQuickItem;
class QQmlEngine;

/* QtAV forward declarations. */
namespace QtAV {
class AVPlayer;
}

class DVWindow : public QOpenGLWindow, public DVInputInterface, public DVRenderInterface {
public:
    DVWindow();
    ~DVWindow();

    /* Parse command line arguments from QApplication. */
    void doCommandLine(class QCommandLineParser& parser);

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

    /* ------------------------------ *
     * End DVInputInterface functions *
     * ------------------------------ */

    /* --------------------------------- *
     * Begin DVRenderInterface functions *
     * --------------------------------- */

    /* Get the FBO that QML is rendered to. */
    virtual const QOpenGLFramebufferObject& getInterfaceFramebuffer();

    /* Get the OpenGL textures for each eye. */
    virtual unsigned int getInterfaceLeftEyeTexture();
    virtual unsigned int getInterfaceRightEyeTexture();

    /* Returns the texture handle the current image / video, and sets left & right to where on the texture each eye is. */
    virtual QSGTexture* getCurrentTexture(QRectF& left, QRectF& right);

    /* Get whether the current image is surround. */
    virtual bool isSurround();

    /* Draw the default sphere (for surround images). */
    virtual void renderStandardSphere();

    /* Get the OpenGL functions. */
    virtual QOpenGLExtraFunctions* getOpenGLFunctions();

    /* Set up the renderer exactly as all the built-in modes have it set up.
     * The left and right image textures will be bound to TEXTURE0 and TEXTURE1, respectively,
     * the viewport is set to the window size, and surround images will be rendered under the UI. */
    virtual void doStandardSetup();

    /* ------------------------------- *
     * End DVRenderInterface functions *
     * ------------------------------- */

public slots:
    void updateQmlSize();

    void onFrameSwapped();

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int, int);

    /* We need to relay these events to qmlWindow. */
    bool event(QEvent *event);

private:
    /* QML Stuff. */
    QQuickRenderControl* qmlRenderControl;
    QQuickWindow* qmlWindow;
    QQmlEngine* qmlEngine;
    QQuickItem* qmlRoot;

    QSize qmlSize;

    DVQmlCommunication* qmlCommunication;
    DVFolderListing* folderListing;
    DVPluginManager* pluginManager;
    QtAV::AVPlayer* player;

    QSettings settings;

    /* Shaders for built-in draw modes. */
    QOpenGLShaderProgram shaderAnaglyph;
    QOpenGLShaderProgram shaderSideBySide;
    QOpenGLShaderProgram shaderTopBottom;
    QOpenGLShaderProgram shaderInterlaced;
    QOpenGLShaderProgram shaderMono;
    QOpenGLShaderProgram shaderSphere;

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
