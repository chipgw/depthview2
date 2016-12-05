#include "gamepadplugin.hpp"
#include "dvinputinterface.hpp"
#include <QQmlComponent>
#include <QQuickItem>
#include <QGamepad>

bool GamepadPlugin::init(QQmlEngine* qmlEngine) {
    Q_INIT_RESOURCE(gamepadplugin);

    QQmlComponent component(qmlEngine);

    component.loadUrl(QUrl(QStringLiteral("qrc:/GamepadPlugin/Config.qml")));

    /* Wait for it to load... */
    while(component.isLoading());

    /* The program can't run if there was an error. */
    if (component.isError()) {
        qDebug(qPrintable(component.errorString()));
        return false;
    }

    configMenuObject = qobject_cast<QQuickItem*>(component.create());

    /* Critical error! abort! abort! */
    if (configMenuObject == nullptr)
        return false;

    /* TODO - Currently the config menu is completely ignored and useless... */

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

    buttonAJustChanged = false;
    buttonBJustChanged = false;
    buttonCenterJustChanged = false;
    buttonDownJustChanged = false;
    buttonGuideJustChanged = false;
    buttonL1JustChanged = false;
    buttonL3JustChanged = false;
    buttonLeftJustChanged = false;
    buttonR1JustChanged = false;
    buttonR3JustChanged = false;
    buttonRightJustChanged = false;
    buttonSelectJustChanged = false;
    buttonStartJustChanged = false;
    buttonUpJustChanged = false;
    buttonXJustChanged = false;
    buttonYJustChanged = false;

    /* Get the list of connected gamepads. */
    auto gamepads = QGamepadManager::instance()->connectedGamepads();

    /* If there are any connected, use the first. */
    if (!gamepads.isEmpty())
        gamepad.setDeviceId(gamepads.first());

    qDebug("Gamepad plugin inited, controller%sfound.", gamepad.isConnected() ? " " : " not ");

    return true;
}

bool GamepadPlugin::deinit() {
    qDebug("Gamepad plugin shutdown");

    return true;
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
    DVInputMode::Type mode = inputInterface->inputMode();

    if (mode == DVInputMode::FileBrowser) {
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
        if (JUST_RELEASED(buttonY))
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
        }
    }

    /* This happens no matter the mode. */
    if (JUST_RELEASED(buttonB) || JUST_RELEASED(buttonSelect))
        inputInterface->cancel();

    /* Reset the change tracking variables. */
    buttonAJustChanged = false;
    buttonBJustChanged = false;
    buttonCenterJustChanged = false;
    buttonDownJustChanged = false;
    buttonGuideJustChanged = false;
    buttonL1JustChanged = false;
    buttonL3JustChanged = false;
    buttonLeftJustChanged = false;
    buttonR1JustChanged = false;
    buttonR3JustChanged = false;
    buttonRightJustChanged = false;
    buttonSelectJustChanged = false;
    buttonStartJustChanged = false;
    buttonUpJustChanged = false;
    buttonXJustChanged = false;
    buttonYJustChanged = false;

    return false;
}

void GamepadPlugin::buttonAChanged(bool value) {
    Q_UNUSED(value)
    buttonAJustChanged = true;
}

void GamepadPlugin::buttonBChanged(bool value) {
    Q_UNUSED(value)
    buttonBJustChanged = true;
}

void GamepadPlugin::buttonCenterChanged(bool value) {
    Q_UNUSED(value)
    buttonCenterJustChanged = true;
}

void GamepadPlugin::buttonDownChanged(bool value) {
    Q_UNUSED(value)
    buttonDownJustChanged = true;
}

void GamepadPlugin::buttonGuideChanged(bool value) {
    Q_UNUSED(value)
    buttonGuideJustChanged = true;
}

void GamepadPlugin::buttonL1Changed(bool value) {
    Q_UNUSED(value)
    buttonL1JustChanged = true;
}

void GamepadPlugin::buttonL3Changed(bool value) {
    Q_UNUSED(value)
    buttonL3JustChanged = true;
}

void GamepadPlugin::buttonLeftChanged(bool value) {
    Q_UNUSED(value)
    buttonLeftJustChanged = true;
}

void GamepadPlugin::buttonR1Changed(bool value) {
    Q_UNUSED(value)
    buttonR1JustChanged = true;
}

void GamepadPlugin::buttonR3Changed(bool value) {
    Q_UNUSED(value)
    buttonR3JustChanged = true;
}

void GamepadPlugin::buttonRightChanged(bool value) {
    Q_UNUSED(value)
    buttonRightJustChanged = true;
}

void GamepadPlugin::buttonSelectChanged(bool value) {
    Q_UNUSED(value)
    buttonSelectJustChanged = true;
}

void GamepadPlugin::buttonStartChanged(bool value) {
    Q_UNUSED(value)
    buttonStartJustChanged = true;
}

void GamepadPlugin::buttonUpChanged(bool value) {
    Q_UNUSED(value)
    buttonUpJustChanged = true;
}

void GamepadPlugin::buttonXChanged(bool value) {
    Q_UNUSED(value)
    buttonXJustChanged = true;
}

void GamepadPlugin::buttonYChanged(bool value) {
    Q_UNUSED(value)
    buttonYJustChanged = true;
}