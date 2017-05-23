#pragma once

#include "dvinputplugin.hpp"
#include <QObject>
#include <QQmlProperty>
#include <QGamepad>

class QOpenGLShaderProgram;

class GamepadPlugin : public QObject, public DVInputPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DVInputPlugin_iid FILE "gamepadplugin.json")
    Q_INTERFACES(DVInputPlugin)

    QQuickItem* configMenuObject;

    QGamepad gamepad;

    QQmlProperty gamepadEnable;

    QString errorString;

    bool buttonAJustChanged;
    bool buttonBJustChanged;
    bool buttonCenterJustChanged;
    bool buttonDownJustChanged;
    bool buttonGuideJustChanged;
    bool buttonL1JustChanged;
    bool buttonL3JustChanged;
    bool buttonLeftJustChanged;
    bool buttonR1JustChanged;
    bool buttonR3JustChanged;
    bool buttonRightJustChanged;
    bool buttonSelectJustChanged;
    bool buttonStartJustChanged;
    bool buttonUpJustChanged;
    bool buttonXJustChanged;
    bool buttonYJustChanged;

    void resetChangedTracker();

public slots:
    void gamepadConnected(int deviceId);

    void buttonAChanged(bool value);
    void buttonBChanged(bool value);
    void buttonCenterChanged(bool value);
    void buttonDownChanged(bool value);
    void buttonGuideChanged(bool value);
    void buttonL1Changed(bool value);
    void buttonL3Changed(bool value);
    void buttonLeftChanged(bool value);
    void buttonR1Changed(bool value);
    void buttonR3Changed(bool value);
    void buttonRightChanged(bool value);
    void buttonSelectChanged(bool value);
    void buttonStartChanged(bool value);
    void buttonUpChanged(bool value);
    void buttonXChanged(bool value);
    void buttonYChanged(bool value);

public:
    bool init(QQmlContext* qmlContext);
    bool deinit();

    QString getErrorString();

    void frameSwapped();

    QQuickItem* getConfigMenuObject();

    bool pollInput(DVInputInterface* inputInterface);
};
