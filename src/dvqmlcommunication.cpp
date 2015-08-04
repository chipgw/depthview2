#include "include/dvqmlcommunication.hpp"

DVQmlCommunication::DVQmlCommunication(QObject *parent) : QObject(parent) {
    setDrawMode(AnglaphFull);
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
        emit drawModeChanged();
    }
}
