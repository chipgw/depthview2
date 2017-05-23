#include "steamcontrollerplugin.hpp"
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>

bool SteamControllerPlugin::init(QQmlContext* qmlContext) {
    Q_INIT_RESOURCE(steamcontrollerplugin);

    qmlContext->setContextProperty("Plugin", this);

    QQmlComponent component(qmlContext->engine());

    component.loadUrl(QUrl(QStringLiteral("qrc:/SteamControllerPlugin/SteamControllerConfig.qml")));

    /* Wait for it to load... */
    while(component.isLoading());

    /* The program can't run if there was an error. */
    if (component.isError()) {
        errorString = component.errorString();
        return false;
    }

    configMenuObject = qobject_cast<QQuickItem*>(component.create(qmlContext));

    /* Critical error! abort! abort! */
    if (configMenuObject == nullptr) {
        errorString = "Unable to create configuration QML component.";
        return false;
    }

    if (!SteamAPI_Init()) {
        errorString = "Unable to initialize Steam API.";
        return false;
    }

    if (!SteamController()->Init()) {
        errorString = "Unable to initialize Steam Controller.";
        return false;
    }

    fileBrowserActionSet = SteamController()->GetActionSetHandle("FileBrowser");
    fileBrowserActions += Action { SteamController()->GetDigitalActionHandle("browser_accept"), &DVInputInterface::accept };
    fileBrowserActions += Action { SteamController()->GetDigitalActionHandle("browser_cancel"), &DVInputInterface::cancel };
    fileBrowserActions += Action { SteamController()->GetDigitalActionHandle("browser_up"), &DVInputInterface::up };
    fileBrowserActions += Action { SteamController()->GetDigitalActionHandle("browser_down"), &DVInputInterface::down };
    fileBrowserActions += Action { SteamController()->GetDigitalActionHandle("browser_left"), &DVInputInterface::left };
    fileBrowserActions += Action { SteamController()->GetDigitalActionHandle("browser_right"), &DVInputInterface::right };
    fileBrowserActions += Action { SteamController()->GetDigitalActionHandle("browser_goup"), &DVInputInterface::goUp };
    fileBrowserActions += Action { SteamController()->GetDigitalActionHandle("browser_goback"), &DVInputInterface::goBack };
    fileBrowserActions += Action { SteamController()->GetDigitalActionHandle("browser_goforward"), &DVInputInterface::goForward };

    imageViewerActionSet = SteamController()->GetActionSetHandle("ImageControls");
    imageViewerActions += Action { SteamController()->GetDigitalActionHandle("img_next"), &DVInputInterface::nextFile };
    imageViewerActions += Action { SteamController()->GetDigitalActionHandle("img_prev"), &DVInputInterface::previousFile };
    imageViewerActions += Action { SteamController()->GetDigitalActionHandle("img_open"), &DVInputInterface::openFileBrowser };
    imageViewerActions += Action { SteamController()->GetDigitalActionHandle("img_zoomactual"), &DVInputInterface::zoomActual };
    imageViewerActions += Action { SteamController()->GetDigitalActionHandle("img_zoomfit"), &DVInputInterface::zoomFit };

    videoPlayerActionSet = SteamController()->GetActionSetHandle("VideoControls");
    videoPlayerActions += Action { SteamController()->GetDigitalActionHandle("vid_next"), &DVInputInterface::nextFile };
    videoPlayerActions += Action { SteamController()->GetDigitalActionHandle("vid_prev"), &DVInputInterface::previousFile };
    videoPlayerActions += Action { SteamController()->GetDigitalActionHandle("vid_open"), &DVInputInterface::openFileBrowser };
    videoPlayerActions += Action { SteamController()->GetDigitalActionHandle("vid_play"), &DVInputInterface::playVideo };
    videoPlayerActions += Action { SteamController()->GetDigitalActionHandle("vid_pause"), &DVInputInterface::pauseVideo };
    videoPlayerActions += Action { SteamController()->GetDigitalActionHandle("vid_togglepause"), &DVInputInterface::playPauseVideo };
    videoPlayerActions += Action { SteamController()->GetDigitalActionHandle("vid_volup"), &DVInputInterface::volumeUp };
    videoPlayerActions += Action { SteamController()->GetDigitalActionHandle("vid_voldown"), &DVInputInterface::volumeDown };
    videoPlayerActions += Action { SteamController()->GetDigitalActionHandle("vid_mute"), &DVInputInterface::mute };
    videoPlayerActions += Action { SteamController()->GetDigitalActionHandle("vid_seekback"), &DVInputInterface::seekBack };
    videoPlayerActions += Action { SteamController()->GetDigitalActionHandle("vid_seekforward"), &DVInputInterface::seekForward };

    /* Call once to detect controllers. */
    SteamAPI_RunCallbacks();

    controllerCount = SteamController()->GetConnectedControllers(controllers);

    qDebug("Steam Controller plugin inited, %i controller(s) found.", controllerCount);

    return true;
}

bool SteamControllerPlugin::deinit() {
    SteamAPI_Shutdown();

    qDebug("Steam Controller plugin shutdown");

    return true;
}

QString SteamControllerPlugin::getErrorString() {
    return errorString;
}

void SteamControllerPlugin::frameSwapped() {
    /* NOTHIN */
}

QQuickItem* SteamControllerPlugin::getConfigMenuObject() {
    return configMenuObject;
}

bool SteamControllerPlugin::pollInput(DVInputInterface* inputInterface) {
    DVInputMode::Type mode = inputInterface->inputMode();

    if (controllerCount == 0)
        return false;

    switch (mode) {
    case DVInputMode::FileBrowser:
        SteamController()->ActivateActionSet(controllers[0], fileBrowserActionSet);
        break;
    case DVInputMode::ImageViewer:
        SteamController()->ActivateActionSet(controllers[0], imageViewerActionSet);
        break;
    case DVInputMode::VideoPlayer:
        SteamController()->ActivateActionSet(controllers[0], videoPlayerActionSet);
        break;
    }

    SteamAPI_RunCallbacks();

    handleActions(inputInterface, fileBrowserActions);
    handleActions(inputInterface, imageViewerActions);
    handleActions(inputInterface, videoPlayerActions);

    return false;
}

void SteamControllerPlugin::handleActions(DVInputInterface* inputInterface, QList<Action>& actions) {
    for (Action& action : actions) {
        /* Store the last state. */
        action.last = action.current;
        /* Get the new state. */
        action.current = SteamController()->GetDigitalActionData(controllers[0], action.handle);

        /* If the action went from disabled to enabled and was active for both last & current, call the function.
         * Checking both last & current prevents held down buttons (e.g. the one that caused the change) from having an effect. */
        if (action.current.bState && !action.last.bState && action.last.bActive && action.current.bActive)
            (inputInterface->*action.func)();
    }
}

void SteamControllerPlugin::openOverlay() {
    if (controllerCount > 0)
        SteamController()->ShowBindingPanel(controllers[0]);
}
