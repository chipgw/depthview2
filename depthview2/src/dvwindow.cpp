#include "version.hpp"
#include "dvwindow.hpp"
#include "dvqmlcommunication.hpp"
#include "dvfolderlisting.hpp"
#include <QApplication>
#include <QQuickRenderControl>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlComponent>
#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QCommandLineParser>
#include <QMessageBox>

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

/* This class is needed for making forwarded keyboard events be recognized by QML. */
class RenderControl : public QQuickRenderControl {
public:
    RenderControl(DVWindow* win) : QQuickRenderControl(win), window(win) { }

    QWindow* window;

    /* Apparently it has something to do with QML making sure the window has focus... */
    QWindow* renderWindow(QPoint* offset) {
        return window == nullptr ? QQuickRenderControl::renderWindow(offset) : window;
    }
};

#ifdef DV_PORTABLE
/* Portable builds store settings in a "DepthView.conf" next to the application executable. */
#define SETTINGS_ARGS QApplication::applicationDirPath() + "/DepthView.conf", QSettings::IniFormat
#else
/* Non-portable builds use an ini file in "%APPDATA%/chipgw" or "~/.config/chipgw". */
#define SETTINGS_ARGS QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName()
#endif

DVWindow::DVWindow() : QOpenGLWindow(), settings(SETTINGS_ARGS), renderFBO(nullptr) {
    qmlCommunication = new DVQmlCommunication(this, settings);
    folderListing = new DVFolderListing(this, settings);

    /* Use the class defined above. */
    qmlRenderControl = new RenderControl(this);
    qmlWindow = new QQuickWindow(qmlRenderControl);

    qmlEngine = new QQmlEngine(this);

    if (qmlEngine->incubationController() == nullptr)
        qmlEngine->setIncubationController(qmlWindow->incubationController());

    qmlEngine->rootContext()->setContextProperty("DepthView", qmlCommunication);
    qmlEngine->rootContext()->setContextProperty("FolderListing", folderListing);

    /* When the Qt.quit() function is called in QML, close this window. */
    connect(qmlEngine, &QQmlEngine::quit, this, &DVWindow::close);

    qmlRegisterUncreatableType<DVDrawMode>(DV_URI_VERSION, "DrawMode", "Only for enum values.");
    qmlRegisterUncreatableType<DVSourceMode>(DV_URI_VERSION, "SourceMode", "Only for enum values.");

    /* Update QML size whenever draw mode or anamorphic are changed. */
    connect(qmlCommunication, &DVQmlCommunication::drawModeChanged, this, &DVWindow::updateQmlSize);
    connect(qmlCommunication, &DVQmlCommunication::anamorphicDualViewChanged, this, &DVWindow::updateQmlSize);

    /* Update window title whenever file changes. */
    connect(folderListing, &DVFolderListing::currentFileChanged, [this](){setTitle(folderListing->currentFile() + " - DepthView");});

    connect(this, &DVWindow::frameSwapped, this, &DVWindow::onFrameSwapped);

    /* We render a cursor inside QML so it is shown for both eyes. */
    setCursor(Qt::BlankCursor);

    setMinimumSize(QSize(1000, 500));
    setGeometry(0,0, 1000, 600);
}

DVWindow::~DVWindow() {
    unloadPlugins();

    delete renderFBO;

    /* TODO - I'm pretty sure there is more that needs to be deleted here... */

    /* Save the window geometry so that it can be restored next run. */
    settings.beginGroup("Window");
    settings.setValue("Geometry", geometry());
    settings.setValue("State", windowState());
    settings.endGroup();
}

void DVWindow::initializeGL() {
    QOpenGLExtraFunctions* f = context()->extraFunctions();
    qDebug("GL Vendor: \"%s\", Renderer: \"%s\".", f->glGetString(GL_VENDOR), f->glGetString(GL_RENDERER));

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
    f->glBindTexture(GL_TEXTURE_2D, renderFBO->textures()[0]);

    f->glActiveTexture(GL_TEXTURE1);
    f->glBindTexture(GL_TEXTURE_2D, renderFBO->textures()[1]);

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

void DVWindow::updateQmlSize() {
    qmlSize = size();

    /* If Side-by-Side and not anamorphic we only render QML at half of the window size (horizontally). */
    if(qmlCommunication->drawMode() == DVDrawMode::SidebySide && !qmlCommunication->anamorphicDualView())
        qmlSize.setWidth(qmlSize.width() / 2);

    /* If Top/Bottom and not anamorphic we only render QML at half of the window size (vertically). */
    if(qmlCommunication->drawMode() == DVDrawMode::TopBottom && !qmlCommunication->anamorphicDualView())
        qmlSize.setHeight(qmlSize.height() / 2);

    if (qmlCommunication->drawMode() == DVDrawMode::Plugin)
        getPluginSize();

    /* Don't recreate fbo's unless they are null or size is wrong. */
    if(renderFBO == nullptr || renderFBO->size() != qmlSize)
        createFBO();

    qmlRoot->setSize(qmlSize);

    qmlWindow->setGeometry(QRect(QPoint(), qmlSize));
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

/* These events need only be passed on to the qmlWindow. */
bool DVWindow::event(QEvent* e) {
    switch (e->type()) {
    case QEvent::Leave:
        /* TODO - This still doesn't always work right, but it's better than using setMouseGrabEnabled()... */
        if (holdMouse) {
            QPoint pos = mapFromGlobal(QCursor::pos());

            /* Generate a new coordinate on screen. */
            pos.setX(qBound(1, pos.x(), width()-1));
            pos.setY(qBound(1, pos.y(), height()-1));

            /* Will generate a new event. */
            QCursor::setPos(mapToGlobal(pos));
        }
        break;
    case QEvent::MouseMove:
        /* We also emit a special signal for this one so that the fake cursor
         * can be set to the right position without having a MouseArea that absorbs events. */
        emit qmlCommunication->mouseMoved(static_cast<QMouseEvent*>(e)->localPos());
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::Wheel:
        QCoreApplication::sendEvent(qmlWindow, e);

        setCursor(Qt::BlankCursor);
        return true;
    case QEvent::TouchBegin:
    case QEvent::TouchEnd:
    case QEvent::TouchUpdate:
    case QEvent::TouchCancel:
        /* TODO - Remap touch location into the modified screen coordinates,
         * in particular for Side by Side & Top/Bottom modes. */
        QCoreApplication::sendEvent(qmlWindow, e);

        emit qmlCommunication->touchEvent();
        return true;
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
        QCoreApplication::sendEvent(qmlWindow, e);
        return true;
    default:
        break;
    }

    return QOpenGLWindow::event(e);
}

void DVWindow::doCommandLine(QCommandLineParser& parser) {
    /* We use one string to hold all warning messages, so we only have to show one dialog. */
    QString warning;

    if(parser.isSet("f"))
        setWindowState(Qt::WindowFullScreen);

    if(parser.isSet("d") && !folderListing->initDir(parser.value("d")))
        warning += tr("<p>Invalid directory \"%1\" passed to \"--startdir\" argument!</p>").arg(parser.value("d"));

    if(parser.isSet("r")){
        const QString& renderer = parser.value("r");

        int mode = qmlCommunication->getModes().indexOf(renderer);

        if(mode == -1)
            warning += tr("<p>Invalid renderer \"%1\" passed to \"--renderer\" argument!</p>").arg(renderer);

        if (mode >= DVDrawMode::Plugin) {
            qmlCommunication->setPluginMode(renderer);
            qmlCommunication->setDrawMode(DVDrawMode::Plugin);
        } else {
            qmlCommunication->setDrawMode(DVDrawMode::Type(mode));
        }
    }

    for (const QString& arg : parser.positionalArguments()) {
        QFileInfo file(arg);

        /* The file extension is checked by openFile(). */
        if (file.exists() && folderListing->openFile(file))
            break;
    }

    /* If there weren't any warnings we don't show the dialog. */
    if(!warning.isEmpty())
        /* TODO - Perhaps this should be done within QML? */
        QMessageBox::warning(nullptr, tr("Invalid Command Line!"), warning);
}
