#include "dvwindow.hpp"
#include "dvqmlcommunication.hpp"
#include <QQuickRenderControl>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlComponent>
#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>
#include <QOpenGLFunctions>

const GLuint vertex = 0;
const GLuint uv     = 1;

class RenderControl : public QQuickRenderControl {
private:
    QWindow* window;
public:
    RenderControl(QWindow* win) : QQuickRenderControl(win), window(win) {}
    QWindow* renderWindow(QPoint*) { return window; }
};

DVWindow::DVWindow() : QOpenGLWindow(), qmlCommunication(new DVQmlCommunication(this)), fboRight(nullptr), fboLeft(nullptr) {
    qmlRenderControl = new RenderControl(this);
    qmlWindow = new QQuickWindow(qmlRenderControl);

    qmlEngine = new QQmlEngine(this);

    if (qmlEngine->incubationController() == nullptr)
        qmlEngine->setIncubationController(qmlWindow->incubationController());

    /* Needs to be registered for enum access. "DepthView" is used for enum values and "DV" is used for accessing members. */
    qmlRegisterUncreatableType<DVQmlCommunication>("DepthView", 2, 0, "DepthView", "Only usable as context property.");
    qmlEngine->rootContext()->setContextProperty("DV", qmlCommunication);

    setGeometry(0,0, 800, 600);
}

DVWindow::~DVWindow() {
    delete fboRight;
    delete fboLeft;
}

void DVWindow::initializeGL() {
    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);
    context()->setFormat(fmt);
    context()->create();

    createFBOs();
    loadShaders();

    QQmlComponent rootComponent(qmlEngine);

    rootComponent.loadUrl(QUrl(QStringLiteral("qrc:/qml/imageview.qml")));
    while(rootComponent.isLoading());

    if (rootComponent.isError()) {
        qDebug(qPrintable(rootComponent.errorString()));
        abort();
    }

    qmlRoot = qobject_cast<QQuickItem*>(rootComponent.create());

    if (qmlRoot == nullptr)
        abort();

    qmlRoot->setParentItem(qmlWindow->contentItem());

    qmlRenderControl->initialize(context());
}

void DVWindow::paintGL() {
    qmlWindow->resetOpenGLState();

    /* Render the right eye view. */
    qmlCommunication->rightImage();
    qmlWindow->setRenderTarget(fboRight);
    qmlRenderControl->polishItems();
    qmlRenderControl->sync();
    qmlRenderControl->render();

    qmlWindow->resetOpenGLState();

    /* Render the left eye view. */
    qmlCommunication->leftImage();
    qmlWindow->setRenderTarget(fboLeft);
    qmlRenderControl->polishItems();
    qmlRenderControl->sync();
    qmlRenderControl->render();

    qmlWindow->resetOpenGLState();

    glViewport(0, 0, width(), height());

    shaderAnglaph.bind();

    QOpenGLFunctions* f = context()->functions();

    f->glActiveTexture(GL_TEXTURE0);
    f->glBindTexture(GL_TEXTURE_2D, fboLeft->texture());
    shaderAnglaph.setUniformValue("textureL", 0);

    f->glActiveTexture(GL_TEXTURE1);
    f->glBindTexture(GL_TEXTURE_2D, fboRight->texture());
    shaderAnglaph.setUniformValue("textureR", 1);

    switch (qmlCommunication->drawMode()) {
    case DVQmlCommunication::AnglaphFull:
        shaderAnglaph.setUniformValue("greyFac", 0.0f);
        break;
    case DVQmlCommunication::AnglaphHalf:
        shaderAnglaph.setUniformValue("greyFac", 0.5f);
        break;
    case DVQmlCommunication::AnglaphGrey:
        shaderAnglaph.setUniformValue("greyFac", 1.0f);
        break;
    default:
        break;
    }

    static const float quad[] {
       -1.0f,-1.0f,
        1.0f,-1.0f,
        1.0f, 1.0f,
       -1.0f, 1.0f
    };

    static const float quadUV[] {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };

    shaderAnglaph.enableAttributeArray(vertex);
    shaderAnglaph.enableAttributeArray(uv);

    shaderAnglaph.setAttributeArray(vertex, quad, 2);
    shaderAnglaph.setAttributeArray(uv, quadUV, 2);

    f->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    /* Set the next frame to display once the current one is done. */
    update();
}

void DVWindow::loadShaders() {
    loadShader(shaderAnglaph, ":/glsl/standard.vsh", ":/glsl/anglaph.fsh");

    /* TODO - Load other shaders. */
}

void DVWindow::loadShader(QOpenGLShaderProgram& shader, const char* vshader, const char* fshader) {
    /* Load the shaders from the qrc. */
    shader.addShaderFromSourceFile(QOpenGLShader::Vertex, vshader);
    shader.addShaderFromSourceFile(QOpenGLShader::Fragment, fshader);

    /* Bind the attribute handles. */
    shader.bindAttributeLocation("vertex", vertex);
    shader.bindAttributeLocation("uv", uv);

    shader.link();
}

void DVWindow::createFBOs() {
    makeCurrent();

    /* Delete the old FBOs if they exsisted. */
    if (fboRight != nullptr)
        delete fboRight;
    if (fboLeft != nullptr)
        delete fboLeft;

    /* Create the FBOs with the same size as this window. */
    /* TODO - This may need to be smaller in non-anamorphic sbs or t/b */
    fboRight = new QOpenGLFramebufferObject(size(), QOpenGLFramebufferObject::CombinedDepthStencil);
    fboLeft = new QOpenGLFramebufferObject(size(), QOpenGLFramebufferObject::CombinedDepthStencil);
}

void DVWindow::resizeGL(int w, int h) {
    createFBOs();

    qmlRoot->setWidth(w);
    qmlRoot->setHeight(h);

    qmlWindow->setGeometry(0, 0, w, h);
}

void DVWindow::mouseMoveEvent(QMouseEvent* e) {
    QCoreApplication::sendEvent(qmlWindow, e);
}

void DVWindow::mousePressEvent(QMouseEvent* e) {
    QCoreApplication::sendEvent(qmlWindow, e);
}

void DVWindow::mouseReleaseEvent(QMouseEvent* e) {
    QCoreApplication::sendEvent(qmlWindow, e);
}

void DVWindow::mouseDoubleClickEvent(QMouseEvent* e) {
    QCoreApplication::sendEvent(qmlWindow, e);
}

void DVWindow::wheelEvent(QWheelEvent* e) {
    QCoreApplication::sendEvent(qmlWindow, e);
}
