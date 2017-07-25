#include "testplugin.hpp"
#include "dvrenderinterface.hpp"
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQmlContext>

bool TestPlugin::init(QOpenGLExtraFunctions*) {
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

    qDebug("Test plugin inited.");

    return true;
}

bool TestPlugin::deinit() {
    delete shader;

    qDebug("Test plugin deinited.");

    return true;
}

QString TestPlugin::getErrorString() {
    return errorString;
}

bool TestPlugin::render(const QString&, DVRenderInterface* renderInterface) {
    if (logRenderStart.isValid() && logRenderStart.read().toBool())
        qDebug("Plugin rendering start.");

    renderInterface->doStandardSetup();

    shader->bind();

    /* Just use the default fullscreen quad from the built-in modes. */
    renderInterface->renderStandardQuad();

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

bool TestPlugin::initConfigMenuObject(QQmlContext* qmlContext) {
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

