#pragma once

#include <QObject>

class DVQmlCommunication : public QObject {
    Q_OBJECT
    Q_ENUMS(DrawMode)
    /* Can only be read. */
    Q_PROPERTY(bool isLeft READ isLeft NOTIFY isLeftChanged)

    /* Can be read, written, and notifies when changed. */
    Q_PROPERTY(DrawMode drawMode MEMBER m_drawMode READ drawMode WRITE setDrawMode NOTIFY drawModeChanged)

public:
    explicit DVQmlCommunication(QObject *parent = 0);

    enum DrawMode {
        AnglaphFull,
        AnglaphHalf,
        AnglaphGrey,
        SidebySide,
        SidebySideMLeft,
        SidebySideMRight,
        SidebySideMBoth,
    };

    /* Where QML reads the value of the current eye. */
    bool isLeft() const;

    /* Set the current eye. */
    void leftImage() { isLeftChanged(m_isLeft = true);  }
    void rightImage() { isLeftChanged(m_isLeft = false); }

    /* The current draw mode. */
    DrawMode drawMode() const;
    void setDrawMode(DrawMode mode);

signals:
    void isLeftChanged(bool isLeft);
    void drawModeChanged();

private:
    bool m_isLeft;
    DrawMode m_drawMode;
};
