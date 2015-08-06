#pragma once

#include <QObject>

class DVQmlCommunication : public QObject {
    Q_OBJECT
    Q_ENUMS(DrawMode)
    /* Can only be read. */
    Q_PROPERTY(bool isLeft READ isLeft NOTIFY isLeftChanged)

    /* Can be read, written, and notifies when changed. */
    Q_PROPERTY(DrawMode drawMode MEMBER m_drawMode READ drawMode WRITE setDrawMode NOTIFY drawModeChanged)
    Q_PROPERTY(bool anamorphicDualView MEMBER m_anamorphicDualView READ anamorphicDualView WRITE setAnamorphicDualView NOTIFY anamorphicDualViewChanged)

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
        TopBottom,
        TopBottomMTop,
        TopBottomMBottom,
        TopBottomMBoth,
    };

    /* Where QML reads the value of the current eye. */
    bool isLeft() const;

    Q_INVOKABLE bool isSideBySide() { return m_drawMode == SidebySide || m_drawMode == SidebySideMLeft || m_drawMode == SidebySideMRight || m_drawMode == SidebySideMBoth; }
    Q_INVOKABLE bool isTopBottom()  { return m_drawMode == TopBottom  || m_drawMode == TopBottomMTop   || m_drawMode == TopBottomMBottom || m_drawMode == TopBottomMBoth; }

    /* Set the current eye. */
    void leftImage() { isLeftChanged(m_isLeft = true);  }
    void rightImage() { isLeftChanged(m_isLeft = false); }

    /* The current draw mode. */
    DrawMode drawMode() const;
    void setDrawMode(DrawMode mode);

    bool anamorphicDualView() const;
    void setAnamorphicDualView(bool anamorphic);

signals:
    void isLeftChanged(bool isLeft);
    void drawModeChanged(DrawMode mode);
    void anamorphicDualViewChanged(bool anamorphicDualView);

private:
    bool m_isLeft;
    DrawMode m_drawMode;
    bool m_anamorphicDualView;
};
