#include "include/dvqmlcommunication.hpp"

DVQmlCommunication::DVQmlCommunication(QObject *parent) : QObject(parent), m_mirrorLeft(false), m_mirrorRight(false), m_greyFac(0.0), m_drawMode(Anglaph), m_anamorphicDualView(false) {
    /* STUB */
}

bool DVQmlCommunication::isLeft() const {
    return m_isLeft;
}

DVQmlCommunication::DrawMode DVQmlCommunication::drawMode() const {
    return m_drawMode;
}

void DVQmlCommunication::setDrawMode(DrawMode mode) {
    /* Only emit if changed. */
    if(m_drawMode != mode) {
        m_drawMode = mode;
        emit drawModeChanged(mode);
    }
}

bool DVQmlCommunication::anamorphicDualView() const {
    return m_anamorphicDualView;
}

void DVQmlCommunication::setAnamorphicDualView(bool anamorphic) {
    /* Only emit if changed. */
    if(m_anamorphicDualView != anamorphic) {
        m_anamorphicDualView = anamorphic;
        emit anamorphicDualViewChanged(anamorphic);
    }
}

bool DVQmlCommunication::mirrorLeft() {
    return m_mirrorLeft;
}

void DVQmlCommunication::setMirrorLeft(bool mirror) {
    /* Only emit if changed. */
    if (m_mirrorLeft != mirror) {
        m_mirrorLeft = mirror;
        emit mirrorLeftChanged(mirror);
    }
}

bool DVQmlCommunication::mirrorRight() {
    return m_mirrorRight;
}

void DVQmlCommunication::setMirrorRight(bool mirror) {
    /* Only emit if changed. */
    if (m_mirrorRight != mirror) {
        m_mirrorRight = mirror;
        emit mirrorRightChanged(mirror);
    }
}

qreal DVQmlCommunication::greyFac() {
    return m_greyFac;
}

void DVQmlCommunication::setGreyFac(qreal fac) {
    /* Limit to [0, 1] range. */
    fac = qBound(0.0, fac, 1.0);

    /* Only emit if changed. */
    if (fac != m_greyFac) {
        m_greyFac = fac;
        emit greyFacChanged(fac);
    }
}
