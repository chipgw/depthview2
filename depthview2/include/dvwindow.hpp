#pragma once

#include <QOpenGLWindow>
#include <QOpenGLShaderProgram>
#include <QSettings>

/* DepthView forward declarations. */
class DVQmlCommunication;
class DVFolderListing;
class DVRenderPlugin;

/* Qt forward declarations. */
class QQuickRenderControl;
class QQuickWindow;
class QQuickItem;
class QQmlEngine;
class QOpenGLFramebufferObject;

class DVWindow : public QOpenGLWindow {
public:
    DVWindow();
    ~DVWindow();

    /* Parse command line arguments from QApplication. */
    void doCommandLine(class QCommandLineParser& parser);

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

    QSettings settings;

    /* Shaders for built-in draw modes. */
    QOpenGLShaderProgram shaderAnaglyph;
    QOpenGLShaderProgram shaderSideBySide;
    QOpenGLShaderProgram shaderTopBottom;
    QOpenGLShaderProgram shaderInterlaced;
    QOpenGLShaderProgram shaderMono;

    /* The FBOs that QML renders to. */
    QOpenGLFramebufferObject* fboRight;
    QOpenGLFramebufferObject* fboLeft;

    void loadShaders();
    void loadShader(QOpenGLShaderProgram& shader, const char* vshader, const char* fshader);
    void createFBOs();

    /* Any loaded plugins. */
    QList<DVRenderPlugin*> renderPlugins;

    /* Load static and dynamic plugins and init them. */
    void loadPlugins();
    /* Call deinit() of all loaded plugins, so as to garbage collect anything they created. */
    void unloadPlugins();
};
