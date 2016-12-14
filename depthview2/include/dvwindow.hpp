#pragma once

#include <QOpenGLWindow>
#include <QOpenGLShaderProgram>
#include <QSettings>
#include "dvinputinterface.hpp"

/* DepthView forward declarations. */
class DVQmlCommunication;
class DVFolderListing;
class DVInputPlugin;
class DVRenderPlugin;

/* Qt forward declarations. */
class QQuickRenderControl;
class QQuickWindow;
class QQuickItem;
class QQmlEngine;
class QOpenGLFramebufferObject;

/* QtAV forward declarations. */
namespace QtAV {
class AVPlayer;
}

class DVWindow : public QOpenGLWindow, public DVInputInterface {
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

    /* Video controls. */
    void playVideo();
    void pauseVideo();
    void playPauseVideo();
    void seekBack();
    void seekForward();
    void seekAmount(qint64 msec);

    /* ------------------------------ *
     * End DVInputInterface functions *
     * ------------------------------ */

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
    QtAV::AVPlayer* player;

    QSettings settings;

    /* Shaders for built-in draw modes. */
    QOpenGLShaderProgram shaderAnaglyph;
    QOpenGLShaderProgram shaderSideBySide;
    QOpenGLShaderProgram shaderTopBottom;
    QOpenGLShaderProgram shaderInterlaced;
    QOpenGLShaderProgram shaderMono;

    /* The FBO that QML renders to. */
    QOpenGLFramebufferObject* renderFBO;

    bool holdMouse;

    void loadShaders();
    void loadShader(QOpenGLShaderProgram& shader, const char* vshader, const char* fshader);
    void createFBO();

    /* Any loaded plugins. */
    QList<DVRenderPlugin*> renderPlugins;
    QList<DVInputPlugin*> inputPlugins;

    /* Load static and dynamic plugins and init them. */
    void loadPlugins();
    /* Call deinit() of all loaded plugins, so as to garbage collect anything they created. */
    void unloadPlugins();

    /* Functions that get the current plugin and interface with it. */
    DVRenderPlugin* getCurrentRenderPlugin();
    bool doPluginRender();
    void getPluginSize();
    void pluginOnFrameSwapped();
    void doPluginInput();
};
