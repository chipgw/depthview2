#include "gamepadplugin.hpp"
#include "dvinputinterface.hpp"
#include <QQmlComponent>
#include <QQuickItem>
#include <QQmlContext>
#include <QGamepad>

bool GamepadPlugin::init(QQmlContext* qmlContext) {
    Q_INIT_RESOURCE(gamepadplugin);

    QQmlComponent component(qmlContext->engine());

    component.loadUrl(QUrl(QStringLiteral("qrc:/GamepadPlugin/GamepadConfig.qml")));

    /* Wait for it to load... */
    while(component.isLoading());

    /* The program can't run if there was an error. */
    if (component.isError()) {
        errorString = component.errorString();
        qDebug(qPrintable(errorString));
        return false;
    }

    configMenuObject = qobject_cast<QQuickItem*>(component.create(qmlContext));

    /* Critical error! abort! abort! */
    if (configMenuObject == nullptr) {
        errorString = "Unable to create configuration QML component.";
        return false;
    }

    connect(&gamepad, &QGamepad::buttonAChanged,        this, &GamepadPlugin::buttonAChanged);
    connect(&gamepad, &QGamepad::buttonBChanged,        this, &GamepadPlugin::buttonBChanged);
    connect(&gamepad, &QGamepad::buttonCenterChanged,   this, &GamepadPlugin::buttonCenterChanged);
    connect(&gamepad, &QGamepad::buttonDownChanged,     this, &GamepadPlugin::buttonDownChanged);
    connect(&gamepad, &QGamepad::buttonGuideChanged,    this, &GamepadPlugin::buttonGuideChanged);
    connect(&gamepad, &QGamepad::buttonL1Changed,       this, &GamepadPlugin::buttonL1Changed);
    connect(&gamepad, &QGamepad::buttonL3Changed,       this, &GamepadPlugin::buttonL3Changed);
    connect(&gamepad, &QGamepad::buttonLeftChanged,     this, &GamepadPlugin::buttonLeftChanged);
    connect(&gamepad, &QGamepad::buttonR1Changed,       this, &GamepadPlugin::buttonR1Changed);
    connect(&gamepad, &QGamepad::buttonR3Changed,       this, &GamepadPlugin::buttonR3Changed);
    connect(&gamepad, &QGamepad::buttonRightChanged,    this, &GamepadPlugin::buttonRightChanged);
    connect(&gamepad, &QGamepad::buttonSelectChanged,   this, &GamepadPlugin::buttonSelectChanged);
    connect(&gamepad, &QGamepad::buttonStartChanged,    this, &GamepadPlugin::buttonStartChanged);
    connect(&gamepad, &QGamepad::buttonUpChanged,       this, &GamepadPlugin::buttonUpChanged);
    connect(&gamepad, &QGamepad::buttonXChanged,        this, &GamepadPlugin::buttonXChanged);
    connect(&gamepad, &QGamepad::buttonYChanged,        this, &GamepadPlugin::buttonYChanged);

    resetChangedTracker();

    connect(QGamepadManager::instance(), &QGamepadManager::gamepadConnected, this, &GamepadPlugin::gamepadConnected);

    gamepadEnable = QQmlProperty(QQmlProperty(configMenuObject, "settings").read().value<QObject*>(), "gamepadEnable");

    qDebug("Gamepad plugin inited, controller%sfound.", gamepad.isConnected() ? " " : " not ");

    return true;
}

bool GamepadPlugin::deinit() {
    qDebug("Gamepad plugin shutdown");

    return true;
}

QString GamepadPlugin::getErrorString() {
    return errorString;
}

void GamepadPlugin::frameSwapped() {
    /* NOTHIN */
}

QQuickItem* GamepadPlugin::getConfigMenuObject() {
    return configMenuObject;
}

#define JUST_RELEASED(X) (!gamepad.X() && X##JustChanged)
#define JUST_PRESSED(X) (gamepad.X() && X##JustChanged)

bool GamepadPlugin::pollInput(DVInputInterface* inputInterface) {
    if (gamepadEnable.read().toBool()) {
        DVInputMode::Type mode = inputInterface->inputMode();

        if (mode == DVInputMode::FileBrowser) {
            if (JUST_RELEASED(buttonStart))
                inputInterface->cancel();

            if (JUST_RELEASED(buttonL1))
                inputInterface->goBack();
            if (JUST_RELEASED(buttonR1))
                inputInterface->goForward();
            if (JUST_RELEASED(buttonY))
                inputInterface->goUp();

            if (JUST_RELEASED(buttonUp))
                inputInterface->up();
            if (JUST_RELEASED(buttonDown))
                inputInterface->down();
            if (JUST_RELEASED(buttonLeft))
                inputInterface->left();
            if (JUST_RELEASED(buttonRight))
                inputInterface->right();

            if (JUST_RELEASED(buttonA))
                inputInterface->accept();
        } else {
            if (JUST_RELEASED(buttonStart))
                inputInterface->openFileBrowser();
            if (JUST_RELEASED(buttonX))
                inputInterface->fileInfo();

            if (JUST_RELEASED(buttonLeft))
                inputInterface->previousFile();
            if (JUST_RELEASED(buttonRight))
                inputInterface->nextFile();

            if (mode == DVInputMode::VideoPlayer) {
                if (JUST_RELEASED(buttonA))
                    inputInterface->playPauseVideo();

                if (JUST_RELEASED(buttonL1))
                    inputInterface->seekBack();
                if (JUST_RELEASED(buttonR1))
                    inputInterface->seekForward();

                if (JUST_RELEASED(buttonUp))
                    inputInterface->volumeUp();
                if (JUST_RELEASED(buttonDown))
                    inputInterface->volumeDown();
            }
        }

        /* This happens no matter the mode. */
        if (JUST_RELEASED(buttonB) || JUST_RELEASED(buttonSelect))
            inputInterface->cancel();
    }

    /* Reset the change tracking variables. */
    resetChangedTracker();

    return false;
}

void GamepadPlugin::resetChangedTracker() {
    buttonAJustChanged = buttonBJustChanged = buttonCenterJustChanged = buttonDownJustChanged =
            buttonGuideJustChanged = buttonL1JustChanged = buttonL3JustChanged = buttonLeftJustChanged =
            buttonR1JustChanged = buttonR3JustChanged = buttonRightJustChanged = buttonSelectJustChanged =
            buttonStartJustChanged = buttonUpJustChanged = buttonXJustChanged = buttonYJustChanged = false;
}

void GamepadPlugin::gamepadConnected(int deviceId) {
    if (!gamepad.isConnected())
        gamepad.setDeviceId(deviceId);
}

void GamepadPlugin::buttonAChanged(bool) {
    buttonAJustChanged = true;
}

void GamepadPlugin::buttonBChanged(bool) {
    buttonBJustChanged = true;
}

void GamepadPlugin::buttonCenterChanged(bool) {
    buttonCenterJustChanged = true;
}

void GamepadPlugin::buttonDownChanged(bool) {
    buttonDownJustChanged = true;
}

void GamepadPlugin::buttonGuideChanged(bool) {
    buttonGuideJustChanged = true;
}

void GamepadPlugin::buttonL1Changed(bool) {
    buttonL1JustChanged = true;
}

void GamepadPlugin::buttonL3Changed(bool) {
    buttonL3JustChanged = true;
}

void GamepadPlugin::buttonLeftChanged(bool) {
    buttonLeftJustChanged = true;
}

void GamepadPlugin::buttonR1Changed(bool) {
    buttonR1JustChanged = true;
}

void GamepadPlugin::buttonR3Changed(bool) {
    buttonR3JustChanged = true;
}

void GamepadPlugin::buttonRightChanged(bool) {
    buttonRightJustChanged = true;
}

void GamepadPlugin::buttonSelectChanged(bool) {
    buttonSelectJustChanged = true;
}

void GamepadPlugin::buttonStartChanged(bool) {
    buttonStartJustChanged = true;
}

void GamepadPlugin::buttonUpChanged(bool) {
    buttonUpJustChanged = true;
}

void GamepadPlugin::buttonXChanged(bool) {
    buttonXJustChanged = true;
}

void GamepadPlugin::buttonYChanged(bool) {
    buttonYJustChanged = true;
}
