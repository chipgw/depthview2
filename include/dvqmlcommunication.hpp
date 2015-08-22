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

    Q_PROPERTY(bool mirrorLeft READ mirrorLeft WRITE setMirrorLeft NOTIFY mirrorLeftChanged)
    Q_PROPERTY(bool mirrorRight READ mirrorRight WRITE setMirrorRight NOTIFY mirrorRightChanged)

    Q_PROPERTY(qreal greyFac READ greyFac WRITE setGreyFac NOTIFY greyFacChanged)

public:
    explicit DVQmlCommunication(QObject *parent = 0);

    enum DrawMode {
        Anglaph,
        SidebySide,
        TopBottom,
        MonoLeft,
        MonoRight
    };

    /* Where QML reads the value of the current eye. */
    bool isLeft() const;

    /* Set the current eye. */
    void leftImage() { isLeftChanged(m_isLeft = true);  }
    void rightImage() { isLeftChanged(m_isLeft = false); }

    /* The current draw mode. */
    DrawMode drawMode() const;
    void setDrawMode(DrawMode mode);

    bool anamorphicDualView() const;
    void setAnamorphicDualView(bool anamorphic);

    bool mirrorLeft();
    void setMirrorLeft(bool mirror);

    bool mirrorRight();
    void setMirrorRight(bool mirror);

    qreal greyFac();
    void setGreyFac(qreal fac);

signals:
    void isLeftChanged(bool isLeft);
    void drawModeChanged(DrawMode mode);
    void anamorphicDualViewChanged(bool anamorphicDualView);

    void mirrorLeftChanged(bool mirror);
    void mirrorRightChanged(bool mirror);

    void greyFacChanged(qreal fac);

    /* Used for setting the cursor position. */
    void mouseMoved(const QPointF& pos);

private:
    bool m_mirrorLeft;
    bool m_mirrorRight;

    qreal m_greyFac;

    bool m_isLeft;
    DrawMode m_drawMode;
    bool m_anamorphicDualView;
};
