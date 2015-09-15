#include "dvshortcut.hpp"
#include <QCoreApplication>
#include <QKeyEvent>
#include <QQuickItem>

DVShortcut::DVShortcut(QObject* parent) : QObject(parent) { /* STUB */ }

void DVShortcut::setKey(const QVariantList& values) {
    QList<QKeySequence> newKeys;

    for (const QVariant& key : values) {
        /* Convert the Variant to a KeySequence. */
        if (key.type() == QVariant::Int)
            newKeys += QKeySequence::keyBindings(static_cast<QKeySequence::StandardKey>(key.toInt()));
        else
            newKeys += QKeySequence::fromString(key.toString());
    }

    /* Remove any empty keys. */
    newKeys.removeAll(QKeySequence());

    /* Don't emit the changed signal if it's the same sequence. */
    if(keys != newKeys) {
        /* If the new key is empty remove any event filter that may have been installed. */
        if(newKeys.isEmpty())
            QCoreApplication::instance()->removeEventFilter(this);
        /* Otherwise if the old key was empty install the event filter. */
        else if (keys.isEmpty())
            QCoreApplication::instance()->installEventFilter(this);

        /* Set sequence and emit the signal for QML. */
        keys = newKeys;
        emit keyChanged();
    }
}

QVariantList DVShortcut::key() const {
    QVariantList keyStrings;

    for (const QKeySequence& key : keys)
        keyStrings += key.toString();

    return keyStrings;
}

bool DVShortcut::eventFilter(QObject* obj, QEvent* e) {
    if(e->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);

        /* Is this the right key combo? */
        if(keys.contains(QKeySequence(keyEvent->modifiers() + keyEvent->key()))) {
            /* If nothing is bound to triggered() try doing something with the parent. */
            if (receivers(SIGNAL(triggered())))
                emit triggered();
            else {
                QString parentClassName(parent()->metaObject()->className());
                /* If the parent is a Button, click it. If the parent is a CheckBox, toggle it. */
                if(!(parentClassName.contains("Button_QMLTYPE_") && QMetaObject::invokeMethod(parent(), "clicked")) &&
                   !(parentClassName.contains("CheckBox_QMLTYPE_") && parent()->setProperty("checked", !parent()->property("checked").toBool())))
                    /* TODO - There might be more parent types that could be automagically activated... */
                    qDebug("Nothing connected to shortcut!");
            }

            /* Nobody else gets to see this event. Nobody. */
            return true;
        }
    }

    /* Not a key press with the right sequence, do what QObject would do. */
    return QObject::eventFilter(obj, e);
}

