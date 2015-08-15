#pragma once

#include <QOpenGLWindow>
#include <QOpenGLShaderProgram>

class DVQmlCommunication;
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
};
