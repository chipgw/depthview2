#pragma once

#include <QObject>
#include <QKeySequence>
#include <QVariant>

class DVShortcut : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList key READ key WRITE setKey NOTIFY keyChanged)

public:
    explicit DVShortcut(QObject* parent = 0);

    /* This is where we catch the shortcut keypress event. */
    bool eventFilter(QObject* obj, QEvent* e);

    void setKey(const QVariantList& values);
    QVariantList key() const;

signals:
    void keyChanged();

    /* Signal that's sent when the key sequence is pressed. */
    void triggered();

private:
    QList<QKeySequence> keys;
};
