#include "dvconfig.hpp"
#include "dvwindowhook.hpp"
#include "dvqmlcommunication.hpp"
#include "dvfolderlisting.hpp"
#include "dvpluginmanager.hpp"
#include "dvvirtualscreenmanager.hpp"
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
#include <QtMath>

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

struct Vertex {
    QVector3D pos;
    QVector2D tex;
};
#define vert_offset(x) (const GLvoid*)offsetof(Vertex, x)

void makeSphere(uint32_t slices, uint32_t stacks, QOpenGLBuffer& sphereVerts, QOpenGLBuffer& sphereTris, GLuint& sphereTriCount) {
    QVector<Vertex> verts;
    QVector<GLuint> triangles;

    /* The amount of rotation needed for each stack, ranging from pole to pole. */
    qreal vstep = M_PI / stacks;
    /* The amount of rotation needed for each slice, ranging all the way around the sphere. */
    qreal hstep = (2.0 * M_PI) / slices;

    /* The offset for the index to connect to in the next stack.  */
    const GLuint w = slices + 1;

    for (uint32_t v = 0; v <= stacks; ++v) {
        /* Calculate the height and radius of the stack. */
        qreal z = qCos(v * vstep);
        qreal r = qSin(v * vstep);

        for (uint32_t h = 0; h <= slices; ++h) {
            GLuint current = verts.size();

            Vertex vert;
            /* Make a circle with the radius of the current stack. */
            vert.pos = QVector3D(qCos(h * hstep) * r,
                                 z,
                                 qSin(h * hstep) * r);

            vert.tex = QVector2D(float(h) / float(slices),
                                 float(v) / float(stacks));

            verts.append(vert);

            if (h != slices && v != stacks) {
                /* A triangle with the current vertex, the next one, and the one above it. */
                triangles << current
                          << current + w
                          << current + 1;

                /* A triangle with the next vertex, the one above it, and the one above the current. */
                triangles << current + w + 1
                          << current + 1
                          << current + w;
            }
        }
    }

    sphereVerts.create();
    sphereVerts.bind();
    sphereVerts.allocate(verts.data(), verts.size() * sizeof(Vertex));

    sphereTris.create();
    sphereTris.bind();
    sphereTris.allocate(triangles.data(), triangles.size() * sizeof(GLuint));
    sphereTriCount = triangles.size();
}

void DVWindowHook::initializeGL() {
    QOpenGLExtraFunctions* f = openglContext()->extraFunctions();
    qDebug("GL Vendor: \"%s\", Renderer: \"%s\".", f->glGetString(GL_VENDOR), f->glGetString(GL_RENDERER));

    loadShaders();

    makeSphere(256, 128, sphereVerts, sphereTris, sphereTriCount);

    vrManager->init();
}

void DVWindowHook::shutdownGL() {
    delete renderFBO;

    /* For some reason this causes a SIGSEGV... */
//    sphereVerts.destroy();
//    sphereTris.destroy();

    vrManager->deinit();
}

void DVWindowHook::preSync() {
    /* Get input from the plugins. */
    pluginManager->doPluginInput(this);

    updateQmlSize();

    window->resetOpenGLState();
}

void DVWindowHook::paintGL() {
    /* Now we don't want QML messing us up. */
    window->resetOpenGLState();

    /* Bind the shader and set uniforms for the current draw mode. */
    switch (qmlCommunication->drawMode()) {
    case DVDrawMode::Anaglyph:
        doStandardSetup();
        shaderAnaglyph->bind();
        shaderAnaglyph->setUniformValue("greyFacL", float(qmlCommunication->greyFacL()));
        shaderAnaglyph->setUniformValue("greyFacR", float(qmlCommunication->greyFacR()));
        break;
    case DVDrawMode::SideBySide:
        doStandardSetup();
        shaderSideBySide->bind();
        shaderSideBySide->setUniformValue("mirrorL", qmlCommunication->mirrorLeft());
        shaderSideBySide->setUniformValue("mirrorR", qmlCommunication->mirrorRight());
        break;
    case DVDrawMode::TopBottom:
        doStandardSetup();
        shaderTopBottom->bind();
        shaderTopBottom->setUniformValue("mirrorL", qmlCommunication->mirrorLeft());
        shaderTopBottom->setUniformValue("mirrorR", qmlCommunication->mirrorRight());
        break;
    case DVDrawMode::InterlacedH:
        doStandardSetup();
        shaderInterlaced->bind();
        shaderInterlaced->setUniformValue("windowCorner", window->position());
        shaderInterlaced->setUniformValue("windowSize", window->size());
        shaderInterlaced->setUniformValue("horizontal", true);
        shaderInterlaced->setUniformValue("vertical", false);
        break;
    case DVDrawMode::InterlacedV:
        doStandardSetup();
        shaderInterlaced->bind();
        shaderInterlaced->setUniformValue("windowCorner", window->position());
        shaderInterlaced->setUniformValue("windowSize", window->size());
        shaderInterlaced->setUniformValue("horizontal", false);
        shaderInterlaced->setUniformValue("vertical", true);
        break;
    case DVDrawMode::Checkerboard:
        doStandardSetup();
        shaderInterlaced->bind();
        shaderInterlaced->setUniformValue("windowCorner", window->position());
        shaderInterlaced->setUniformValue("windowSize", window->size());
        shaderInterlaced->setUniformValue("horizontal", true);
        shaderInterlaced->setUniformValue("vertical", true);
        break;
    case DVDrawMode::Mono:
        doStandardSetup();
        shaderMono->bind();
        shaderMono->setUniformValue("left", true);
        break;
    case DVDrawMode::VirtualReality:
        if (vrManager->render()) {
            QOpenGLFramebufferObject::bindDefault();
            window->resetOpenGLState();

            if (vrManager->mirrorUI()) {
                doStandardSetup();
                shaderMono->bind();
                shaderMono->setUniformValue("left", true);
                break;
            }

            /* Make sure the viewport is the correct size, QML may have changed it. */
            openglContext()->extraFunctions()->glViewport(0, 0, window->width(), window->height());
            openglContext()->extraFunctions()->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            openglContext()->extraFunctions()->glClear(GL_COLOR_BUFFER_BIT);
            return;
        }
        DV_FALLTHROUGH;
    default:
        /* Whoops, invalid renderer. Reset to Anaglyph... */
        qmlCommunication->setDrawMode(DVDrawMode::Anaglyph);
        return;
    }

    renderStandardQuad();
}

QOpenGLContext* DVWindowHook::openglContext() {
    return window->openglContext();
}

void DVWindowHook::onFrameSwapped() {
    /* None of the built-in modes hold the mouse. */
    holdMouse = false;

    window->update();
}

void DVWindowHook::loadShaders() {
    shaderAnaglyph      = new QOpenGLShaderProgram(openglContext());
    shaderSideBySide    = new QOpenGLShaderProgram(openglContext());
    shaderTopBottom     = new QOpenGLShaderProgram(openglContext());
    shaderInterlaced    = new QOpenGLShaderProgram(openglContext());
    shaderMono          = new QOpenGLShaderProgram(openglContext());
    shaderSphere        = new QOpenGLShaderProgram(openglContext());

    /* Most draw modes use the standard vertex shader for a simple fullscreen quad. */
    loadShader(*shaderAnaglyph,     ":/glsl/standard.vsh", ":/glsl/anaglyph.fsh");
    loadShader(*shaderSideBySide,   ":/glsl/standard.vsh", ":/glsl/sidebyside.fsh");
    loadShader(*shaderTopBottom,    ":/glsl/standard.vsh", ":/glsl/topbottom.fsh");
    loadShader(*shaderInterlaced,   ":/glsl/standard.vsh", ":/glsl/interlaced.fsh");
    loadShader(*shaderMono,         ":/glsl/standard.vsh", ":/glsl/standard.fsh");
    loadShader(*shaderSphere,       ":/glsl/sphere.vsh",   ":/glsl/sphere.fsh");
}

void DVWindowHook::loadShader(QOpenGLShaderProgram& shader, const char* vshader, const char* fshader) {
    /* Load the shaders from the qrc. */
    shader.addShaderFromSourceFile(QOpenGLShader::Vertex, vshader);

    QFile res(fshader);
    res.open(QIODevice::ReadOnly | QIODevice::Text);
    QString fshaderSrc = res.readAll();

#ifndef Q_OS_MAC
    if (!openglContext()->isOpenGLES())
        fshaderSrc.prepend("#version 130\n");
#endif

    shader.addShaderFromSourceCode(QOpenGLShader::Fragment, fshaderSrc);

    /* Bind the attribute handles. */
    shader.bindAttributeLocation("vertex", vertex);
    shader.bindAttributeLocation("uv", uv);

    shader.link();

    /* Bind so we set the texture sampler uniform values. */
    shader.bind();

    /* Left image is TEXTURE0. */
    shader.setUniformValue("textureL", DVStereoEye::LeftEye);
    /* Right image is TEXTURE1. */
    shader.setUniformValue("textureR", DVStereoEye::RightEye);
}

void DVWindowHook::createFBO() {
    openglContext()->makeCurrent(window);

    /* Delete the old FBO if it exsists. */
    if (renderFBO != nullptr)
        delete renderFBO;

    QOpenGLExtraFunctions* f = openglContext()->extraFunctions();

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

    window->setRenderTarget(renderFBO);
}

const QOpenGLFramebufferObject& DVWindowHook::getInterfaceFramebuffer() {
    return *renderFBO;
}

GLuint DVWindowHook::getInterfaceTexture(DVStereoEye::Type eye) const {
    return renderFBO->textures()[qmlCommunication->swapEyes() ? 1-eye : eye];
}

QSGTexture* DVWindowHook::getCurrentTexture(QRectF& left, QRectF& right) {
    getTextureRects(left, right, qmlCommunication->openImageTexture(),
                    folderListing->currentFileStereoSwap(), folderListing->currentFileStereoMode());

    return qmlCommunication->openImageTexture();
}

void DVWindowHook::getTextureRects(QRectF& left, QRectF& right, QSGTexture* texture, bool swap, DVSourceMode::Type mode) {
    if (texture == nullptr) return;

    right = left = texture->normalizedTextureSubRect();

    switch (mode) {
    case DVSourceMode::TopBottom:
    case DVSourceMode::TopBottomAnamorphic:
        left.setHeight(left.height() * 0.5f);
        right.setHeight(right.height() * 0.5f);

        if (swap)
            left.translate(0.0f, left.y() + left.height());
        else
            right.translate(0.0f, right.y() + left.height());
        break;
    case DVSourceMode::SideBySide:
    case DVSourceMode::SideBySideAnamorphic:
        left.setWidth(left.width() * 0.5f);
        right.setWidth(right.width() * 0.5f);

        if (swap)
            left.translate(left.x() + left.width(), 0.0f);
        else
            right.translate(right.x() + right.width(), 0.0f);
        break;
    case DVSourceMode::Mono:
        /* Do nothing for mono images. */
        break;
    }
}

void DVWindowHook::renderStandardSphere() {
    QOpenGLExtraFunctions* f = openglContext()->extraFunctions();

    /* Enable the vertex and UV arrays. */
    f->glEnableVertexAttribArray(vertex);
    f->glEnableVertexAttribArray(uv);

    sphereVerts.bind();
    sphereTris.bind();

    /* Set up the attribute buffers once for all planets. */
    f->glVertexAttribPointer(vertex, 3, GL_FLOAT, GL_TRUE, sizeof(Vertex), vert_offset(pos));
    f->glVertexAttribPointer(uv,     2, GL_FLOAT, GL_TRUE, sizeof(Vertex), vert_offset(tex));

    f->glDrawElements(GL_TRIANGLES, sphereTriCount, GL_UNSIGNED_INT, nullptr);

    sphereVerts.release();
    sphereTris.release();
}

void DVWindowHook::renderStandardQuad() {
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

     QOpenGLExtraFunctions* f = openglContext()->extraFunctions();

     /* Enable the vertex and UV arrays, must be done every frame because of QML resetting things. */
     f->glEnableVertexAttribArray(vertex);
     f->glEnableVertexAttribArray(uv);

     f->glVertexAttribPointer(vertex, 2, GL_FLOAT, GL_FALSE, 0, quad);
     f->glVertexAttribPointer(uv,     2, GL_FLOAT, GL_FALSE, 0, quadUV);

     f->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void DVWindowHook::doStandardSetup() {
    QOpenGLExtraFunctions* f = openglContext()->extraFunctions();

    if (qmlCommunication->openImageTexture() && folderListing->isCurrentFileSurround()) {
        f->glViewport(0, 0, qmlSize.width(), qmlSize.height());

        f->glEnable(GL_BLEND);
        f->glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);

        renderFBO->bind();

        shaderSphere->bind();

        QRectF left, right;
        getCurrentTexture(left, right)->bind();

        shaderSphere->setUniformValue("leftRect", left.x(), left.y(), left.width(), left.height());
        shaderSphere->setUniformValue("rightRect", right.x(), right.y(), right.width(), right.height());

        QMatrix4x4 mat;
        /* Create a camera matrix using the surround FOV from QML and the aspect ratio of the FBO. */
        mat.perspective(qmlCommunication->surroundFOV(), float(qmlSize.width()) / float(qmlSize.height()), 0.01f, 1.0f);

        /* Rotate the matrix based on pan values. */
        mat.rotate(qmlCommunication->surroundPan().y(), 1.0f, 0.0f, 0.0f);
        mat.rotate(qmlCommunication->surroundPan().x(), 0.0f, 1.0f, 0.0f);

        /* Upload to shader. */
        shaderSphere->setUniformValue("cameraMatrix", mat);

        renderStandardSphere();

        renderFBO->release();

        f->glBlendFunc(GL_ONE, GL_ZERO);
        f->glDisable(GL_BLEND);
    }

    f->glViewport(0, 0, window->width(), window->height());

    f->glActiveTexture(GL_TEXTURE0);
    f->glBindTexture(GL_TEXTURE_2D, getInterfaceTexture(DVStereoEye::LeftEye));

    f->glActiveTexture(GL_TEXTURE1);
    f->glBindTexture(GL_TEXTURE_2D, getInterfaceTexture(DVStereoEye::RightEye));
}
