#include "dvwindow.hpp"
#include "dvshortcut.hpp"
#include "dvqmlcommunication.hpp"
#include "dvrenderplugin.hpp"
#include <QQuickRenderControl>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlComponent>
#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QPluginLoader>
#include <QDir>

/* Vertex attrib locations. */
const GLuint vertex = 0;
const GLuint uv     = 1;

DVWindow::DVWindow() : QOpenGLWindow(), qmlCommunication(new DVQmlCommunication(this)), fboRight(nullptr), fboLeft(nullptr) {
    qmlRenderControl = new  QQuickRenderControl(this);
    qmlWindow = new QQuickWindow(qmlRenderControl);

    qmlEngine = new QQmlEngine(this);

    if (qmlEngine->incubationController() == nullptr)
        qmlEngine->setIncubationController(qmlWindow->incubationController());

    qmlRegisterType<DVShortcut>("DepthView", 2, 0, "Shortcut");
    /* Needs to be registered for enum access. "DepthView" is used for enum values and "DV" is used for accessing members. */
    qmlRegisterUncreatableType<DVQmlCommunication>("DepthView", 2, 0, "DepthView", "Only usable as context property.");
    qmlEngine->rootContext()->setContextProperty("DV", qmlCommunication);

    /* Update QML size whenever draw mode or anamorphic are changed. */
    connect(qmlCommunication, &DVQmlCommunication::drawModeChanged, this, &DVWindow::updateQmlSize);
    connect(qmlCommunication, &DVQmlCommunication::anamorphicDualViewChanged, this, &DVWindow::updateQmlSize);

    /* We render a cursor inside QML so it is shown for both eyes. */
    setCursor(Qt::BlankCursor);

    /* TODO - Remember previous geometry. */
    setGeometry(0,0, 800, 600);
}

DVWindow::~DVWindow() {
    unloadPlugins();

    delete fboRight;
    delete fboLeft;

    /* TODO - I'm pretty sure there is more that needs to be deleted here... */
}

void DVWindow::initializeGL() {
    loadShaders();

    loadPlugins();

    QQmlComponent rootComponent(qmlEngine);

    rootComponent.loadUrl(QUrl(QStringLiteral("qrc:/qml/Window.qml")));

    /* Wait for it to load... */
    while(rootComponent.isLoading());

    /* The program can't run if there was an error. */
    if (rootComponent.isError()) {
        qDebug(qPrintable(rootComponent.errorString()));
        abort();
    }

    qmlRoot = qobject_cast<QQuickItem*>(rootComponent.create());

    /* Critical error! abort! abort! */
    if (qmlRoot == nullptr)
        abort();

    /* This is the root item, make it so. */
    qmlRoot->setParentItem(qmlWindow->contentItem());

    /* Init to this window's OpenGL context. */
    qmlRenderControl->initialize(context());
}

void DVWindow::paintGL() {
    /* So QML doesn't freak out because of stuff we did last frame. */
    qmlWindow->resetOpenGLState();

    /* Render the right eye view. */
    qmlCommunication->rightImage();
    qmlWindow->setRenderTarget(fboRight);
    qmlRenderControl->polishItems();
    qmlRenderControl->sync();
    qmlRenderControl->render();

    /* Render the left eye view. */
    qmlCommunication->leftImage();
    qmlWindow->setRenderTarget(fboLeft);
    qmlRenderControl->polishItems();
    qmlRenderControl->sync();
    qmlRenderControl->render();

    /* Now we don't want QML messing us up. */
    qmlWindow->resetOpenGLState();

    QOpenGLFunctions* f = context()->functions();

    /* Make sure the viewport is the correct size, QML may have changed it. */
    f->glViewport(0, 0, width(), height());

    f->glActiveTexture(GL_TEXTURE0);
    f->glBindTexture(GL_TEXTURE_2D, fboLeft->texture());

    f->glActiveTexture(GL_TEXTURE1);
    f->glBindTexture(GL_TEXTURE_2D, fboRight->texture());

    /* Bind the shader and set uniforms for the current draw mode. */
    switch (qmlCommunication->drawMode()) {
    case DVQmlCommunication::Anglaph:
        shaderAnglaph.bind();
        shaderAnglaph.setUniformValue("greyFac", float(qmlCommunication->greyFac()));
        break;
    case DVQmlCommunication::SidebySide:
        shaderSideBySide.bind();
        shaderSideBySide.setUniformValue("mirrorL", qmlCommunication->mirrorLeft());
        shaderSideBySide.setUniformValue("mirrorR", qmlCommunication->mirrorRight());
        break;
    case DVQmlCommunication::TopBottom:
        shaderTopBottom.bind();
        shaderTopBottom.setUniformValue("mirrorL", qmlCommunication->mirrorLeft());
        shaderTopBottom.setUniformValue("mirrorR", qmlCommunication->mirrorRight());
        break;
    case DVQmlCommunication::MonoLeft:
        shaderMono.bind();
        shaderMono.setUniformValue("left", true);
        break;
    case DVQmlCommunication::MonoRight:
        shaderMono.bind();
        shaderMono.setUniformValue("left", false);
        break;
    case DVQmlCommunication::Plugin:
        for (DVRenderPlugin* plugin : renderPlugins) {
            /* Find the first plugin that contains the mode we want. */
            if (plugin->drawModeNames().contains(qmlCommunication->pluginMode())) {
                /* Let it do it's thing. */
                plugin->render(qmlCommunication->pluginMode(), f);

                /* Don't check any other plugins. */
                break;
            }
        }

        /* Return from here to avoid the default fullscreen quad.
         * We still want to queue up the next frame as we do below. */
        return update();
    default:
        /* Whoops invalid renderer... */
        /* TODO - What happens here? */
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

    /* Enable the vertex and UV arrays, must be done every frame because of QML resetting things. */
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
    if(qmlCommunication->drawMode() == DVQmlCommunication::SidebySide && !qmlCommunication->anamorphicDualView())
        qmlSize.setWidth(qmlSize.width() / 2);

    /* If Top/Bottom and not anamorphic we only render QML at half of the window size (vertically). */
    if(qmlCommunication->drawMode() == DVQmlCommunication::TopBottom && !qmlCommunication->anamorphicDualView())
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
    loadShader(shaderMono,          ":/glsl/standard.vsh", ":/glsl/standard.fsh");

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

    /* Create the FBOs with the calculated QML size. */
    fboRight = new QOpenGLFramebufferObject(qmlSize, QOpenGLFramebufferObject::CombinedDepthStencil);
    fboLeft = new QOpenGLFramebufferObject(qmlSize, QOpenGLFramebufferObject::CombinedDepthStencil);

    /* Use Linear filtering for nicer scaling/. */
    f->glBindTexture(GL_TEXTURE_2D, fboRight->texture());
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    f->glBindTexture(GL_TEXTURE_2D, fboLeft->texture());
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void DVWindow::resizeGL(int, int) {
    /* Delegate to updateQmlSize to resize FBO's and stuff. */
    updateQmlSize();
}

/* These events need only be passed on to the qmlWindow. */
void DVWindow::mouseMoveEvent(QMouseEvent* e) {
    QCoreApplication::sendEvent(qmlWindow, e);

    /* We also emit a special signal for this one so that the fake cursor
     * can be set to the right position without having a MouseArea that absorbs events. */
    emit qmlCommunication->mouseMoved(e->localPos());

    setCursor(Qt::BlankCursor);
}

/* These events need only be passed on to the qmlWindow. */
void DVWindow::mousePressEvent(QMouseEvent* e) {
    QCoreApplication::sendEvent(qmlWindow, e);

    setCursor(Qt::BlankCursor);
}

/* These events need only be passed on to the qmlWindow. */
void DVWindow::mouseReleaseEvent(QMouseEvent* e) {
    QCoreApplication::sendEvent(qmlWindow, e);

    setCursor(Qt::BlankCursor);
}

/* These events need only be passed on to the qmlWindow. */
void DVWindow::mouseDoubleClickEvent(QMouseEvent* e) {
    QCoreApplication::sendEvent(qmlWindow, e);

    setCursor(Qt::BlankCursor);
}

/* These events need only be passed on to the qmlWindow. */
void DVWindow::wheelEvent(QWheelEvent* e) {
    QCoreApplication::sendEvent(qmlWindow, e);

    setCursor(Qt::BlankCursor);
}

void DVWindow::loadPlugins() {
    /* Load any statically linked plugins. (Currently there aren't any) */
    for (QObject *obj : QPluginLoader::staticInstances()) {
        DVRenderPlugin* plugin;
        if ((plugin = qobject_cast<DVRenderPlugin*>(obj)) != nullptr)
            renderPlugins.append(plugin);
    }

    /* Start with the path the application is in. */
    QDir pluginsDir(qApp->applicationDirPath());

#if defined(Q_OS_WIN)
    /* If we're in a "debug" or "release" folder go up a level, because that's where plugins are copied by the build system. */
    if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release")
        pluginsDir.cdUp();
#elif defined(Q_OS_MAC)
    /* I haven't actually tried this on Mac. This is just what the Qt plugin example said to do... */
    if (pluginsDir.dirName() == "MacOS") {
        pluginsDir.cdUp();
        pluginsDir.cdUp();
        pluginsDir.cdUp();
    }
#endif

    /* Go into thhe "plugins" folder from there. */
    pluginsDir.cd("plugins");

    /* Try to load all files in the directory. */
    for (const QString& filename : pluginsDir.entryList(QDir::Files)) {
        QPluginLoader loader(pluginsDir.absoluteFilePath(filename));
        QObject *obj = loader.instance();
        DVRenderPlugin* plugin;

        /* If it can't be cast to a DVRenderPlugin* it isn't a valid plugin. */
        if (obj != nullptr && (plugin = qobject_cast<DVRenderPlugin*>(obj)) != nullptr) {
            renderPlugins.append(plugin);
            qDebug("Found plugin: \"%s\"", qPrintable(filename));
        }
    }

    /* Init any/all loaded plugins. */
    for (DVRenderPlugin* plugin : renderPlugins)
        if (plugin->init(context()->functions())) qmlCommunication->addPluginModes(plugin->drawModeNames());
}

void DVWindow::unloadPlugins() {
    /* Deinit any/all loaded plugins. */
    for (DVRenderPlugin* plugin : renderPlugins)
        plugin->deinit();

    /* Clear the list. Not that it should be used anymore... */
    renderPlugins.clear();
}
