#pragma once

#include "dvinputplugin.hpp"
#include "dvinputinterface.hpp"
#include <QObject>
#include <QQmlProperty>
#include <steam_api.h>

class QOpenGLShaderProgram;

struct Action {
    ControllerDigitalActionHandle_t handle;

    void (DVInputInterface::*func)();

    ControllerDigitalActionData_t last, current;
};

class SteamControllerPlugin : public QObject, public DVInputPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DVInputPlugin_iid FILE "steamcontrollerplugin.json")
    Q_INTERFACES(DVInputPlugin)

    ControllerHandle_t controllers[STEAM_CONTROLLER_MAX_COUNT];
    int controllerCount;

    ControllerActionSetHandle_t fileBrowserActionSet;
    QList<Action> fileBrowserActions;

    ControllerActionSetHandle_t imageViewerActionSet;
    QList<Action> imageViewerActions;

    ControllerActionSetHandle_t videoPlayerActionSet;
    QList<Action> videoPlayerActions;

    QString errorString;

public:
    bool init(QQmlContext*);
    bool deinit();

    QString getErrorString();

    void frameSwapped();

    QQuickItem* getConfigMenuObject();

    bool pollInput(DVInputInterface* inputInterface);
    void handleActions(DVInputInterface* inputInterface, QList<Action>& actions);

    Q_INVOKABLE void openOverlay();
};
