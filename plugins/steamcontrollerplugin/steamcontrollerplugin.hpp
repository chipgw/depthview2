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

    ControllerDigitalActionData_t last = ControllerDigitalActionData_t{};
    ControllerDigitalActionData_t  current = ControllerDigitalActionData_t{};
};

class SteamControllerPlugin : public QObject, public DVInputPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DVInputPlugin_iid FILE "steamcontrollerplugin.json")
    Q_INTERFACES(DVInputPlugin)

    QQuickItem* configMenuObject;

    ControllerHandle_t controllers[STEAM_CONTROLLER_MAX_COUNT];
    int controllerCount;

    ControllerActionSetHandle_t fileBrowserActionSet;
    QList<Action> fileBrowserActions;

    ControllerActionSetHandle_t imageViewerActionSet;
    QList<Action> imageViewerActions;

    ControllerActionSetHandle_t videoPlayerActionSet;
    QList<Action> videoPlayerActions;

public:
    bool init(QQmlContext* qmlContext);
    bool deinit();

    void frameSwapped();

    QQuickItem* getConfigMenuObject();

    bool pollInput(DVInputInterface* inputInterface);
    void handleActions(DVInputInterface* inputInterface, QList<Action>& actions);

    Q_INVOKABLE void openOverlay();
};