#include "include/dvqmlcommunication.hpp"

DVQmlCommunication::DVQmlCommunication(QObject *parent) : QObject(parent), m_drawMode(AnglaphFull) {
    /* STUB */
}

bool DVQmlCommunication::isLeft() const {
    return m_isLeft;
}

DVQmlCommunication::DrawMode DVQmlCommunication::drawMode() const {
    return m_drawMode;
}

void DVQmlCommunication::setDrawMode(DrawMode mode) {
    if(m_drawMode != mode) {
        m_drawMode = mode;
        emit drawModeChanged(mode);
    }
}

bool DVQmlCommunication::anamorphicDualView() const {
    return m_anamorphicDualView;
}

void DVQmlCommunication::setAnamorphicDualView(bool anamorphic) {
    if(m_anamorphicDualView != anamorphic) {
        m_anamorphicDualView = anamorphic;
        emit anamorphicDualViewChanged(anamorphic);
    }
}
