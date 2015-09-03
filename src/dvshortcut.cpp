#include "dvshortcut.hpp"
#include <QCoreApplication>
#include <QKeyEvent>

DVShortcut::DVShortcut(QObject* parent) : QObject(parent) { /* STUB */ }

void DVShortcut::setKey(const QVariant& key) {
    /* Convert the variant to a KeySequence. */
    QKeySequence newKey = key.value<QKeySequence>();

    /* Don't emit the changed signal if it's the same sequence. */
    if(m_key != newKey) {
        /* If the new key is empty remove any event filter that may have been installed. */
        if(newKey.isEmpty())
            QCoreApplication::instance()->removeEventFilter(this);
        /* Otherwise if the old key was empty install the event filter. */
        else if (m_key.isEmpty())
            QCoreApplication::instance()->installEventFilter(this);

        /* Set sequence and emit the signal for QML. */
        m_key = newKey;
        emit keyChanged();
    }
}

bool DVShortcut::eventFilter(QObject* obj, QEvent* e) {
    if(e->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);

        /* Is this the right key combo? */
        if(QKeySequence(keyEvent->modifiers() + keyEvent->key()) == m_key) {
            emit triggered();

            /* Nobody else gets to see this event. Nobody. */
            return true;
        }
    }

    /* Not a key press with the right sequence, do what QObject would do. */
    return QObject::eventFilter(obj, e);
}

