#pragma once

#include <QDir>
#include <QTimer>
#include <QUrl>
#include <QAbstractListModel>

class QSettings;

class DVFolderListing : public QAbstractListModel {
    Q_OBJECT

    QDir m_currentDir;
    QFileInfo m_currentFile;

    QStringList browserHistory;
    int currentHistory;

    QStringList m_bookmarks;

    QTimer driveTimer;

    QSettings& settings;

    Q_PROPERTY(QString currentFile READ currentFile NOTIFY currentFileChanged)
    Q_PROPERTY(QUrl currentURL READ currentURL NOTIFY currentFileChanged)
    Q_PROPERTY(QUrl currentDir READ currentDir WRITE setCurrentDir NOTIFY currentDirChanged)

    /* By making this a property we can emit a signal when the list needs to be updated. */
    Q_PROPERTY(QStringList storageDevicePaths READ getStorageDevicePaths NOTIFY storageDevicePathsChanged)

    /* Basically the same situation as with storageDevicePaths, we want it to automagically update things that reference it. */
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY historyChanged)
    Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY historyChanged)
    Q_PROPERTY(bool canGoUp READ canGoUp NOTIFY currentDirChanged)

    /* There is no WRITE function because you add or remove via addBookmark() and deleteBookmark(). */
    Q_PROPERTY(QStringList bookmarks READ bookmarks NOTIFY bookmarksChanged)

public:
    explicit DVFolderListing(QObject* parent, QSettings& s);

    QString currentFile();
    QUrl currentURL();

    void openFile(QFileInfo fileInfo);
    Q_INVOKABLE void openFile(QUrl url);

    /* Used by the property system. (Calls the QString overload with url.toLocalFile()) */
    void setCurrentDir(QUrl url);
    /* Used by everything else. */
    void setCurrentDir(QString dir);
    QUrl currentDir();

    /* Is there a directory above the current dir? */
    bool canGoUp() const;
    /* Go up a dir from the current dir. */
    Q_INVOKABLE void goUp();

    /* Returns a list of strings describing mounted drives in the format: "mount path;display name". */
    QStringList getStorageDevicePaths() const;

    /* Seriously, why doesn't QML have these capabilities somewhere? */
    Q_INVOKABLE bool fileExists(QString file) const;
    Q_INVOKABLE bool dirExists(QString dir) const;

    /* Used by QML to convert local file strings to URLs and visa versa. */
    Q_INVOKABLE QUrl encodeURL(QString url) const;
    Q_INVOKABLE QString decodeURL(QUrl url) const;

    /* Decrement the current history item and update the currentDir. Only call when canGoBack is true. */
    Q_INVOKABLE void goBack();
    /* Increment the current history item and update the currentDir. Only call when canGoForward is true. */
    Q_INVOKABLE void goForward();

    /* Called whenever the file browser changes directories. If value is empty or the current history item nothing happens.
     * If it's the next one we go forward. If it's the previous one we go back. Otherwise we clear any forward history and add it at the end of the list. */
    void pushHistory();

    /* Is there a value to go back to? */
    bool canGoBack() const;
    /* Is there a value to go forward to? */
    bool canGoForward() const;

    Q_INVOKABLE void addBookmark(QString bookmark);
    Q_INVOKABLE void deleteBookmark(QString bookmark);

    QStringList bookmarks() const;

    Q_INVOKABLE void openNext();
    Q_INVOKABLE void openPrevious();

    /* Begin Model stuff... */
    enum Roles {
        FileNameRole = Qt::UserRole+1,
        FilePathRole,
        IsDirRole,
        IsImageRole,
        IsVideoRole,
    };

    QHash<int, QByteArray> roleNames() const;

    QVariant data(const QModelIndex &index, int role) const;
    int rowCount(const QModelIndex &parent) const;

signals:
    void currentFileChanged(QString file);

    /* Don't include the dir because this also triggers the listing update. */
    void currentDirChanged();

    void storageDevicePathsChanged();

    void historyChanged();

    void bookmarksChanged();
};
