#pragma once

#include <QDir>
#include <QTimer>
#include <QUrl>

class QSettings;

class DVFolderListing : public QObject {
    Q_OBJECT

    QStringList browserHistory;
    int currentHistory;

    QStringList m_bookmarks;

    QTimer driveTimer;

    QSettings& settings;

    /* By making this a property we can emit a signal when the list needs to be updated. */
    Q_PROPERTY(QStringList storageDevicePaths READ getStorageDevicePaths NOTIFY storageDevicePathsChanged)

    /* Basically the same situation as with storageDevicePaths, we want it to automagically update things that reference it. */
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY historyChanged)
    Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY historyChanged)

    /* There is no WRITE function because you add or remove via addBookmark() and deleteBookmark(). */
    Q_PROPERTY(QStringList bookmarks READ bookmarks NOTIFY bookmarksChanged)

public:
    explicit DVFolderListing(QObject* parent, QSettings& s);

    /* Returns a list of strings describing mounted drives in the format: "mount path;display name". */
    QStringList getStorageDevicePaths() const;

    /* Seriously, why doesn't QML have these capabilities somewhere? */
    Q_INVOKABLE bool fileExists(QString file) const;
    Q_INVOKABLE bool dirExists(QString dir) const;

    /* Used by QML to convert local file strings to URLs and visa versa. */
    Q_INVOKABLE QUrl encodeURL(QString url) const;
    Q_INVOKABLE QString decodeURL(QUrl url) const;

    /* Decrement the current history item and return its value. Only call when canGoBack is true. */
    Q_INVOKABLE QString goBack();
    /* Increment the current history item and return its value. Only call when canGoBack is true. */
    Q_INVOKABLE QString goForward();

    /* Called whenever the file browser changes directories. If value is empty or the current history item nothing happens.
     * If it's the next one we go forward. If it's the previous one we go back. Otherwise we clear any forward history and add it at the end of the list. */
    Q_INVOKABLE void pushHistory(QString value);

    /* Is there a value to go back to? */
    bool canGoBack() const;
    /* Is there a value to go forward to? */
    bool canGoForward() const;

    Q_INVOKABLE void addBookmark(QString bookmark);
    Q_INVOKABLE void deleteBookmark(QString bookmark);

    QStringList bookmarks() const;

signals:
    void storageDevicePathsChanged();

    void historyChanged();

    void bookmarksChanged(QStringList bookmarks);
};
