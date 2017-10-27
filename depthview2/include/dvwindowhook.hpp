#pragma once

#include <QSettings>
#include <QDir>
#include "dvinputinterface.hpp"

/* DepthView forward declarations. */
class DVQmlCommunication;
class DVFolderListing;
class DVPluginManager;
class DVRenderer;
class DVVirtualScreenManager;

/* Qt forward declarations. */
class QQuickItem;
class QQmlApplicationEngine;
class QQuickWindow;

/* QtAV forward declarations. */
namespace QtAV {
class AVPlayer;
}

class DVWindowHook : public QObject, public DVInputInterface {
    Q_OBJECT

    QQuickWindow* window;

public:
    DVWindowHook(QQmlApplicationEngine* engine);
    ~DVWindowHook();

    /* Parse command line arguments from QApplication. */
    void doCommandLine(class QCommandLineParser& parser);

    /* -------------------------------- *
     * Begin DVInputInterface functions *
     * -------------------------------- */

    /* Get the current input mode. */
    DVInputMode::Type inputMode() const;

    /* Navigation controls, used primarily in the file browser. */
    void left();
    void right();
    void up();
    void down();

    /* Accept the currently highlighted item. */
    void accept();
    /* Closes any and all popups, basically the same as pressing the escape key. */
    void cancel();

    /* Directly open the file browser. */
    void openFileBrowser();

    /* Navigation for file browser. */
    void goBack();
    void goForward();
    void goUp();

    /* Show the file info popup. */
    void fileInfo();

    /* Open the next/previous file in the current directory. */
    void nextFile();
    void previousFile();

    void zoomActual();
    void zoomFit();

    /* Video controls. */
    void playVideo();
    void pauseVideo();
    void playPauseVideo();
    void seekBack();
    void seekForward();
    void seekAmount(qint64 msec);

    void volumeUp();
    void volumeDown();
    void mute();
    void setVolume(qreal volume);

    /* Object to send simulated mouse/keyboard events to. */
    QObject* inputEventObject();

    /* When the volume control functions are called, they call these in the main thread. */
    Q_INVOKABLE void muteImpl();
    Q_INVOKABLE void setVolumeImpl(qreal volume);

    /* ------------------------------ *
     * End DVInputInterface functions *
     * ------------------------------ */

public slots:
    void preSync();

    void updateTitle();

protected:
    bool eventFilter(QObject*, QEvent* event);

private:
    QSettings settings;

    DVQmlCommunication* qmlCommunication;
    DVFolderListing* folderListing;
    DVRenderer* renderer;
    DVPluginManager* pluginManager;
    DVVirtualScreenManager* vrManager;
    QtAV::AVPlayer* player;

    bool holdMouse;
};
