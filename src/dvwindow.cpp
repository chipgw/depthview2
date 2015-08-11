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

    connect(qmlCommunication, &DVQmlCommunication::drawModeChanged, this, &DVWindow::updateQmlSize);
    connect(qmlCommunication, &DVQmlCommunication::anamorphicDualViewChanged, this, &DVWindow::updateQmlSize);

    setCursor(Qt::BlankCursor);

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

    makeCurrent();
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

    QOpenGLFunctions* f = context()->functions();

    f->glViewport(0, 0, width(), height());

    f->glActiveTexture(GL_TEXTURE0);
    f->glBindTexture(GL_TEXTURE_2D, fboLeft->texture());

    f->glActiveTexture(GL_TEXTURE1);
    f->glBindTexture(GL_TEXTURE_2D, fboRight->texture());

    switch (qmlCommunication->drawMode()) {
    case DVQmlCommunication::AnglaphFull:
        shaderAnglaph.bind();
        shaderAnglaph.setUniformValue("greyFac", 0.0f);
        break;
    case DVQmlCommunication::AnglaphHalf:
        shaderAnglaph.bind();
        shaderAnglaph.setUniformValue("greyFac", 0.5f);
        break;
    case DVQmlCommunication::AnglaphGrey:
        shaderAnglaph.bind();
        shaderAnglaph.setUniformValue("greyFac", 1.0f);
        break;
    case DVQmlCommunication::SidebySide:
        shaderSideBySide.bind();
        shaderSideBySide.setUniformValue("mirrorL", false);
        shaderSideBySide.setUniformValue("mirrorR", false);
        break;
    case DVQmlCommunication::SidebySideMLeft:
        shaderSideBySide.bind();
        shaderSideBySide.setUniformValue("mirrorL", true);
        shaderSideBySide.setUniformValue("mirrorR", false);
        break;
    case DVQmlCommunication::SidebySideMRight:
        shaderSideBySide.bind();
        shaderSideBySide.setUniformValue("mirrorL", false);
        shaderSideBySide.setUniformValue("mirrorR", true);
        break;
    case DVQmlCommunication::SidebySideMBoth:
        shaderSideBySide.bind();
        shaderSideBySide.setUniformValue("mirrorL", true);
        shaderSideBySide.setUniformValue("mirrorR", true);
        break;
    case DVQmlCommunication::TopBottom:
        shaderTopBottom.bind();
        shaderTopBottom.setUniformValue("mirrorL", false);
        shaderTopBottom.setUniformValue("mirrorR", false);
        break;
    case DVQmlCommunication::TopBottomMTop:
        shaderTopBottom.bind();
        shaderTopBottom.setUniformValue("mirrorL", true);
        shaderTopBottom.setUniformValue("mirrorR", false);
        break;
    case DVQmlCommunication::TopBottomMBottom:
        shaderTopBottom.bind();
        shaderTopBottom.setUniformValue("mirrorL", false);
        shaderTopBottom.setUniformValue("mirrorR", true);
        break;
    case DVQmlCommunication::TopBottomMBoth:
        shaderTopBottom.bind();
        shaderTopBottom.setUniformValue("mirrorL", true);
        shaderTopBottom.setUniformValue("mirrorR", true);
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

    f->glEnableVertexAttribArray(vertex);
    f->glEnableVertexAttribArray(uv);

    f->glVertexAttribPointer(vertex, 2, GL_FLOAT, GL_FALSE, 0, quad);
    f->glVertexAttribPointer(uv,     2, GL_FLOAT, GL_FALSE, 0, quadUV);

    f->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    /* Set the next frame to display once the current one is done. */
    update();
}

void DVWindow::updateQmlSize() {
    qmlSize = size();

    /* If Side-by-Side and not anamorphic we only render QML at half of the window size (horizontally). */
    if(qmlCommunication->isSideBySide() && !qmlCommunication->anamorphicDualView())
        qmlSize.setWidth(qmlSize.width() / 2);

    /* If Top/Bottom and not anamorphic we only render QML at half of the window size (vertically). */
    if(qmlCommunication->isTopBottom() && !qmlCommunication->anamorphicDualView())
        qmlSize.setHeight(qmlSize.height() / 2);

    /* Don't recreate fbo's unless they are null or size is wrong. */
    if(fboLeft == nullptr || fboLeft->size() != qmlSize || fboRight == nullptr || fboRight->size() != qmlSize)
        createFBOs();

    qmlRoot->setSize(qmlSize);

    qmlWindow->setGeometry(QRect(QPoint(), qmlSize));
}

void DVWindow::loadShaders() {
    /* Most draw modes use the standard vertex shader for a simple fullscreen quad. */
    loadShader(shaderAnglaph,       ":/glsl/standard.vsh", ":/glsl/anglaph.fsh");
    loadShader(shaderSideBySide,    ":/glsl/standard.vsh", ":/glsl/sidebyside.fsh");
    loadShader(shaderTopBottom,     ":/glsl/standard.vsh", ":/glsl/topbottom.fsh");

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

    /* Bind so we set the texture sampler uniform values. */
    shader.bind();

    /* Left image is TEXTURE0. */
    shader.setUniformValue("textureL", 0);
    /* Right image is TEXTURE1. */
    shader.setUniformValue("textureR", 1);
}

void DVWindow::createFBOs() {
    makeCurrent();

    /* Delete the old FBOs if they exsisted. */
    if (fboRight != nullptr)
        delete fboRight;
    if (fboLeft != nullptr)
        delete fboLeft;

    QOpenGLFunctions* f = context()->functions();

    /* Create the FBOs with the same size as this window. */
    /* TODO - This may need to be smaller in non-anamorphic sbs or t/b */
    fboRight = new QOpenGLFramebufferObject(qmlSize, QOpenGLFramebufferObject::CombinedDepthStencil);
    f->glBindTexture(GL_TEXTURE_2D, fboRight->texture());
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    fboLeft = new QOpenGLFramebufferObject(qmlSize, QOpenGLFramebufferObject::CombinedDepthStencil);
    f->glBindTexture(GL_TEXTURE_2D, fboLeft->texture());
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void DVWindow::resizeGL(int, int) {
    updateQmlSize();
}

void DVWindow::mouseMoveEvent(QMouseEvent* e) {
    QCoreApplication::sendEvent(qmlWindow, e);

    emit qmlCommunication->mouseMoved(e->localPos());

    setCursor(Qt::BlankCursor);
}

void DVWindow::mousePressEvent(QMouseEvent* e) {
    QCoreApplication::sendEvent(qmlWindow, e);

    setCursor(Qt::BlankCursor);
}

void DVWindow::mouseReleaseEvent(QMouseEvent* e) {
    QCoreApplication::sendEvent(qmlWindow, e);

    setCursor(Qt::BlankCursor);
}

void DVWindow::mouseDoubleClickEvent(QMouseEvent* e) {
    QCoreApplication::sendEvent(qmlWindow, e);

    setCursor(Qt::BlankCursor);
}

void DVWindow::wheelEvent(QWheelEvent* e) {
    QCoreApplication::sendEvent(qmlWindow, e);

    setCursor(Qt::BlankCursor);
}
