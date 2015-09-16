#pragma once

#include <QOpenGLWindow>
#include <QOpenGLShaderProgram>

/* DepthView forward declarations. */
class DVQmlCommunication;
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

public slots:
    void updateQmlSize();

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int, int);

    /* We need to relay these events to qmlWindow. */
    void mouseMoveEvent(QMouseEvent* e);
    void mousePressEvent(QMouseEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
    void mouseDoubleClickEvent(QMouseEvent* e);
    void wheelEvent(QWheelEvent* e);

private:
    /* QML Stuff. */
    QQuickRenderControl* qmlRenderControl;
    QQuickWindow* qmlWindow;
    QQmlEngine* qmlEngine;
    QQuickItem* qmlRoot;

    QSize qmlSize;

    DVQmlCommunication* qmlCommunication;

    /* Shaders for built-in draw modes. */
    QOpenGLShaderProgram shaderAnglaph;
    QOpenGLShaderProgram shaderSideBySide;
    QOpenGLShaderProgram shaderTopBottom;
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