#include "testplugin.hpp"
#include "dvrenderinterface.hpp"
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQmlContext>

bool TestPlugin::init(QOpenGLExtraFunctions*, QQmlContext* qmlContext) {
    Q_INIT_RESOURCE(testplugin);

    shader = new QOpenGLShaderProgram;

    shader->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/TestPlugin/glsl/plugin.vsh");
    shader->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/TestPlugin/glsl/plugin.fsh");

    shader->link();

    /* Bind so we set the texture sampler uniform values. */
    shader->bind();

    /* Left image is TEXTURE0. */
    shader->setUniformValue("textureL", 0);
    /* Right image is TEXTURE1. */
    shader->setUniformValue("textureR", 1);

    QQmlComponent component(qmlContext->engine());

    component.loadUrl(QUrl(QStringLiteral("qrc:/TestPlugin/TestPluginConfig.qml")));

    /* Wait for it to load... */
    while(component.isLoading());

    /* The program can't run if there was an error. */
    if (component.isError()) {
        qDebug(qPrintable(component.errorString()));
        return false;
    }

    configMenuObject = qobject_cast<QQuickItem*>(component.create(qmlContext));

    /* Critical error! abort! abort! */
    if (configMenuObject == nullptr)
        return false;

    QObject* obj = QQmlProperty(configMenuObject, "settings").read().value<QObject*>();
    logRenderStart = QQmlProperty(obj, "logRenderStart");
    logRenderEnd = QQmlProperty(obj, "logRenderEnd");
    logFrameSwap = QQmlProperty(obj, "logFrameSwap");
    lockMouse = QQmlProperty(obj, "lockMouse");
    renderSizeFactor = QQmlProperty(obj, "renderSizeFactor");

    qDebug("Test plugin inited.");

    return true;
}

bool TestPlugin::deinit() {
    delete shader;

    qDebug("Test plugin deinited.");

    return true;
}

bool TestPlugin::render(const QString&, DVRenderInterface* renderInterface) {
    if (logRenderStart.isValid() && logRenderStart.read().toBool())
        qDebug("Plugin rendering start.");

    renderInterface->doStandardSetup();

    /* This is just the default fullscreen quad from the built-in modes. */
    shader->bind();

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

    QOpenGLExtraFunctions* f = renderInterface->getOpenGLFunctions();

    /* Enable the vertex and UV arrays, must be done every frame because of QML resetting things. */
    f->glEnableVertexAttribArray(0);
    f->glEnableVertexAttribArray(1);

    f->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, quad);
    f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, quadUV);

    f->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    if (logRenderEnd.isValid() && logRenderEnd.read().toBool())
        qDebug("Plugin rendering end.");

    return true;
}

void TestPlugin::frameSwapped(QOpenGLExtraFunctions*) {
    if (logFrameSwap.isValid() && logFrameSwap.read().toBool())
        qDebug("Plugin frame swapped.");
}

QStringList TestPlugin::drawModeNames() {
    return QStringList("Test Plugin Mode");
}

QQuickItem* TestPlugin::getConfigMenuObject() {
    return configMenuObject;
}

bool TestPlugin::shouldLockMouse() {
    return lockMouse.read().toBool();
}

QSize TestPlugin::getRenderSize(const QSize& windowSize) {
    bool ok;
    qreal sizeFactor = renderSizeFactor.read().toReal(&ok);
    return windowSize * (ok ? sizeFactor : 1.0);
}

bool TestPlugin::pollInput(DVInputInterface*) {
    return false;
}

