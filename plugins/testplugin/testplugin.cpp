#include "testplugin.hpp"
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>

bool TestPlugin::init(QOpenGLFunctions* f) {
    Q_INIT_RESOURCE(testplugin);

    shader = new QOpenGLShaderProgram;

    shader->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/glsl/plugin.vsh");
    shader->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/glsl/plugin.fsh");

    shader->link();

    /* Bind so we set the texture sampler uniform values. */
    shader->bind();

    /* Left image is TEXTURE0. */
    shader->setUniformValue("textureL", 0);
    /* Right image is TEXTURE1. */
    shader->setUniformValue("textureR", 1);

    qDebug("inited");

    return true;
}

bool TestPlugin::deinit() {
    delete shader;

    qDebug("deinited");

    return true;
}

bool TestPlugin::render(const QString& drawModeName, QOpenGLFunctions* f) {
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

    /* Enable the vertex and UV arrays, must be done every frame because of QML resetting things. */
    f->glEnableVertexAttribArray(0);
    f->glEnableVertexAttribArray(1);

    f->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, quad);
    f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, quadUV);

    f->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    return true;
}

QStringList TestPlugin::drawModeNames() {
    return QStringList("Test Plugin Mode");
}
