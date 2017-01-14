#pragma once

#include "dvenums.hpp"

/* This class is virtual so it can be used in plugins without linking shenanigans, is implemented in DVWindow. */
class DVInputInterface {
public:
    /* Get the current input mode. */
    virtual DVInputMode::Type inputMode() const = 0;

    /* Navigation controls, used primarily in the file browser. */
    virtual void left() = 0;
    virtual void right() = 0;
    virtual void up() = 0;
    virtual void down() = 0;

    /* Accept the currently highlighted item. */
    virtual void accept() = 0;
    /* Closes any and all popups, basically the same as pressing the escape key. */
    virtual void cancel() = 0;

    /* Directly open the file browser. */
    virtual void openFileBrowser() = 0;

    /* Navigation for file browser. */
    virtual void goBack() = 0;
    virtual void goForward() = 0;
    virtual void goUp() = 0;

    /* Show the file info popup. */
    virtual void fileInfo() = 0;

    /* Open the next/previous file in the current directory. */
    virtual void nextFile() = 0;
    virtual void previousFile() = 0;

//    /* Move the mouse to the specified location (in window coordinates) or by the specified amount (if relative is true). */
//    virtual void setMousePosition(QPoint pos, bool relative = false) = 0;

//    /* Send a fake mouse click event to the window. */
//    virtual void sendMouseClick(Qt::MouseButton button) = 0;

    /* Video controls. */
    virtual void playVideo() = 0;
    virtual void pauseVideo() = 0;
    virtual void playPauseVideo() = 0;
    virtual void seekBack() = 0;
    virtual void seekForward() = 0;
    virtual void seekAmount(qint64 msec) = 0;

    virtual void volumeUp() = 0;
    virtual void volumeDown() = 0;
    virtual void mute() = 0;
    virtual void setVolume(qreal volume) = 0;
};
