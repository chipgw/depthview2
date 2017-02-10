#include "dvwindow.hpp"
#include "dvqmlcommunication.hpp"
#include "dvfolderlisting.hpp"
#include <QQuickRenderControl>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlComponent>
#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <AVPlayer.h>

/* Android in particular may not have this defined. */
#ifndef GL_COLOR_ATTACHMENT1
/* If there's an EXT defined, use it. */
#ifdef GL_COLOR_ATTACHMENT1_EXT
#define GL_COLOR_ATTACHMENT1 GL_COLOR_ATTACHMENT1_EXT
#else
/* Otherwise resort to just using ATTACHMENT0 plus one. */
#define GL_COLOR_ATTACHMENT1 GL_COLOR_ATTACHMENT0+1
#endif
#endif

/* Vertex attrib locations. */
const GLuint vertex = 0;
const GLuint uv     = 1;

void DVWindow::initializeGL() {
    QOpenGLExtraFunctions* f = context()->extraFunctions();
    qDebug("GL Vendor: \"%s\", Renderer: \"%s\".", f->glGetString(GL_VENDOR), f->glGetString(GL_RENDERER));

    loadShaders();

    loadPlugins();

    QQmlComponent rootComponent(qmlEngine);

    rootComponent.loadUrl(QUrl(QStringLiteral("qrc:/qml/Window.qml")));

    /* Wait for it to load... */
    while (rootComponent.isLoading());

    /* The program can't run if there was an error. */
    if (rootComponent.isError())
        qFatal(qPrintable(rootComponent.errorString()));

    qmlRoot = qobject_cast<QQuickItem*>(rootComponent.create());

    /* Critical error! abort! abort! */
    if (qmlRoot == nullptr)
        qFatal(qPrintable(rootComponent.errorString()));

    /* This is the root item, make it so. */
    qmlRoot->setParentItem(qmlWindow->contentItem());

    player = qmlRoot->findChild<QtAV::AVPlayer*>();
    if (player == nullptr)
        qFatal("Unable to find AVPlayer!");

    /* Init to this window's OpenGL context. */
    qmlRenderControl->initialize(context());

    qmlCommunication->postQmlInit();
    folderListing->postQmlInit();

    /* The setGeometry() and setState() calls may try to set the qmlRoot geometry,
     * which means this needs to be done after QML is all set up. */
    if (settings.childGroups().contains("Window")) {
        /* Restore window state from the stored geometry. */
        settings.beginGroup("Window");
        setGeometry(settings.value("Geometry").toRect());
        setWindowState(Qt::WindowState(settings.value("State").toInt()));
        settings.endGroup();
    }
}

void DVWindow::paintGL() {
    /* Get input from the plugins. */
    doPluginInput();

    /* So QML doesn't freak out because of stuff we did last frame. */
    qmlWindow->resetOpenGLState();

    qmlWindow->setRenderTarget(renderFBO);
    qmlRenderControl->polishItems();
    qmlRenderControl->sync();
    qmlRenderControl->render();

    /* Now we don't want QML messing us up. */
    qmlWindow->resetOpenGLState();

    QOpenGLExtraFunctions* f = context()->extraFunctions();

    /* Make sure the viewport is the correct size, QML may have changed it. */
    f->glViewport(0, 0, width(), height());

    f->glActiveTexture(GL_TEXTURE0);
    f->glBindTexture(GL_TEXTURE_2D, renderFBO->textures()[qmlCommunication->swapEyes() ? 1 : 0]);

    f->glActiveTexture(GL_TEXTURE1);
    f->glBindTexture(GL_TEXTURE_2D, renderFBO->textures()[qmlCommunication->swapEyes() ? 0 : 1]);

    /* Bind the shader and set uniforms for the current draw mode. */
    switch (qmlCommunication->drawMode()) {
    case DVDrawMode::Anaglyph:
        shaderAnaglyph.bind();
        shaderAnaglyph.setUniformValue("greyFac", float(qmlCommunication->greyFac()));
        break;
    case DVDrawMode::SidebySide:
        shaderSideBySide.bind();
        shaderSideBySide.setUniformValue("mirrorL", qmlCommunication->mirrorLeft());
        shaderSideBySide.setUniformValue("mirrorR", qmlCommunication->mirrorRight());
        break;
    case DVDrawMode::TopBottom:
        shaderTopBottom.bind();
        shaderTopBottom.setUniformValue("mirrorL", qmlCommunication->mirrorLeft());
        shaderTopBottom.setUniformValue("mirrorR", qmlCommunication->mirrorRight());
        break;
    case DVDrawMode::InterlacedH:
        shaderInterlaced.bind();
        shaderInterlaced.setUniformValue("windowCorner", position());
        shaderInterlaced.setUniformValue("windowSize", size());
        shaderInterlaced.setUniformValue("horizontal", true);
        shaderInterlaced.setUniformValue("vertical", false);
        break;
    case DVDrawMode::InterlacedV:
        shaderInterlaced.bind();
        shaderInterlaced.setUniformValue("windowCorner", position());
        shaderInterlaced.setUniformValue("windowSize", size());
        shaderInterlaced.setUniformValue("horizontal", false);
        shaderInterlaced.setUniformValue("vertical", true);
        break;
    case DVDrawMode::Checkerboard:
        shaderInterlaced.bind();
        shaderInterlaced.setUniformValue("windowCorner", position());
        shaderInterlaced.setUniformValue("windowSize", size());
        shaderInterlaced.setUniformValue("horizontal", true);
        shaderInterlaced.setUniformValue("vertical", true);
        break;
    case DVDrawMode::Mono:
        shaderMono.bind();
        shaderMono.setUniformValue("left", true);
        break;
    case DVDrawMode::Plugin:
        if (doPluginRender())
            /* If it worked, return from here to avoid the default fullscreen quad. */
            return;
    default:
        /* Whoops, invalid renderer. Reset to Anaglyph... */
        qmlCommunication->setDrawMode(DVDrawMode::Anaglyph);
        return;
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

    /* Enable the vertex and UV arrays, must be done every frame because of QML resetting things. */
    f->glEnableVertexAttribArray(vertex);
    f->glEnableVertexAttribArray(uv);

    f->glVertexAttribPointer(vertex, 2, GL_FLOAT, GL_FALSE, 0, quad);
    f->glVertexAttribPointer(uv,     2, GL_FLOAT, GL_FALSE, 0, quadUV);

    f->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void DVWindow::onFrameSwapped() {
    /* None of the built-in modes hold the mouse. */
    holdMouse = false;

    /* In case one of the plugins needs to do something OpenGL related. */
    makeCurrent();

    if (qmlCommunication->drawMode() == DVDrawMode::Plugin)
        pluginOnFrameSwapped();

    update();
}

void DVWindow::loadShaders() {
    /* Most draw modes use the standard vertex shader for a simple fullscreen quad. */
    loadShader(shaderAnaglyph,      ":/glsl/standard.vsh", ":/glsl/anaglyph.fsh");
    loadShader(shaderSideBySide,    ":/glsl/standard.vsh", ":/glsl/sidebyside.fsh");
    loadShader(shaderTopBottom,     ":/glsl/standard.vsh", ":/glsl/topbottom.fsh");
    loadShader(shaderInterlaced,    ":/glsl/standard.vsh", ":/glsl/interlaced.fsh");
    loadShader(shaderMono,          ":/glsl/standard.vsh", ":/glsl/standard.fsh");
}

void DVWindow::loadShader(QOpenGLShaderProgram& shader, const char* vshader, const char* fshader) {
    /* Load the shaders from the qrc. */
    shader.addShaderFromSourceFile(QOpenGLShader::Vertex, vshader);

    QFile res(fshader);
    res.open(QIODevice::ReadOnly | QIODevice::Text);
    QString fshaderSrc = res.readAll();

    if (!context()->isOpenGLES())
        fshaderSrc.prepend("#version 130\n");

    shader.addShaderFromSourceCode(QOpenGLShader::Fragment, fshaderSrc);

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

void DVWindow::createFBO() {
    makeCurrent();

    /* Delete the old FBO if it exsists. */
    if (renderFBO != nullptr)
        delete renderFBO;

    QOpenGLExtraFunctions* f = context()->extraFunctions();

    /* Create the FBOs with the calculated QML size. */
    renderFBO = new QOpenGLFramebufferObject(qmlSize, QOpenGLFramebufferObject::CombinedDepthStencil);

    renderFBO->bind();
    renderFBO->addColorAttachment(qmlSize);
    
    const GLenum buf[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    f->glDrawBuffers(2, buf);

    /* Use Linear filtering for nicer scaling. */
    for (GLuint texture : renderFBO->textures()) {
        f->glBindTexture(GL_TEXTURE_2D, texture);
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
}

void DVWindow::resizeGL(int, int) {
    /* Delegate to updateQmlSize to resize FBO's and stuff. */
    updateQmlSize();
}
