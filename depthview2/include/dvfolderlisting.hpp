#pragma once

#include <QDir>
#include <QTimer>
#include <QUrl>
#include <QAbstractListModel>
#include <QSqlRecord>
#include <QMutex>
#include "dvenums.hpp"

class QSettings;
class DVQmlCommunication;

class DVFolderListing : public QAbstractListModel {
    Q_OBJECT

    QSettings& settings;

    QDir m_currentDir;
    QFileInfo m_currentFile;

    QStringList stereoImageSuffixes;
    QStringList imageSuffixes;
    QStringList videoSuffixes;

    QStringList browserHistory;
    int currentHistory;

    QStringList m_bookmarks;

    QTimer driveTimer;

    bool m_fileBrowserOpen;

    /* Only one database operation can be running at once. */
    mutable QMutex dbOpMutex;

    Q_PROPERTY(QString currentFile READ currentFile NOTIFY currentFileChanged)
    Q_PROPERTY(QUrl currentURL READ currentURL NOTIFY currentFileChanged)

    /* Allow QML to easily know the type of the current file. */
    Q_PROPERTY(bool currentFileIsStereoImage READ isCurrentFileStereoImage NOTIFY currentFileChanged)
    Q_PROPERTY(bool currentFileIsImage READ isCurrentFileImage NOTIFY currentFileChanged)
    Q_PROPERTY(bool currentFileIsVideo READ isCurrentFileVideo NOTIFY currentFileChanged)
    Q_PROPERTY(int currentFileAudioTrack READ currentFileAudioTrack WRITE setCurrentFileAudioTrack NOTIFY currentFileAudioTrackChanged)
    Q_PROPERTY(bool currentFileIsSurround READ isCurrentFileSurround WRITE setCurrentFileSurround NOTIFY currentFileSurroundChanged)
    Q_PROPERTY(DVSourceMode::Type currentFileStereoMode READ currentFileStereoMode WRITE setCurrentFileStereoMode NOTIFY currentFileStereoModeChanged)
    Q_PROPERTY(bool currentFileStereoSwap READ currentFileStereoSwap WRITE setCurrentFileStereoSwap NOTIFY currentFileStereoSwapChanged)
    Q_PROPERTY(qint64 currentFileSize READ currentFileSize NOTIFY currentFileChanged)
    Q_PROPERTY(QString currentFileInfo READ currentFileInfo NOTIFY currentFileChanged)

    Q_PROPERTY(QUrl currentDir READ currentDir WRITE setCurrentDir NOTIFY currentDirChanged)

    /* By making this a property we can emit a signal when the list needs to be updated. */
    Q_PROPERTY(QStringList storageDevicePaths READ getStorageDevicePaths NOTIFY storageDevicePathsChanged)

    /* Basically the same situation as with storageDevicePaths, we want it to automagically update things that reference it. */
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY historyChanged)
    Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY historyChanged)
    Q_PROPERTY(bool canGoUp READ canGoUp NOTIFY currentDirChanged)

    /* There is no WRITE function because you add or remove via addBookmark() and deleteBookmark(). */
    Q_PROPERTY(QStringList bookmarks READ bookmarks NOTIFY bookmarksChanged)

    Q_PROPERTY(bool fileBrowserOpen READ fileBrowserOpen WRITE setFileBrowserOpen NOTIFY fileBrowserOpenChanged)

    Q_PROPERTY(QString startDir READ startDir WRITE setStartDir NOTIFY startDirChanged)
    Q_PROPERTY(QString snapshotDir READ snapshotDir WRITE setSnapshotDir NOTIFY snapshotDirChanged)

public:
    explicit DVFolderListing(QObject* parent, QSettings& s);

    /* Just the name of the current file, no path. */
    QString currentFile() const;
    /* The absolute URl of the current file. */
    QUrl currentURL() const;

    bool openFile(QFileInfo fileInfo);
    Q_INVOKABLE bool openFile(QUrl url);

    /* Used by the property system. (Calls the QString overload with url.toLocalFile()) */
    void setCurrentDir(QUrl url);
    /* Used by everything else. */
    void setCurrentDir(QString dir);
    QUrl currentDir() const;

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

    Q_INVOKABLE QString bytesToString(qint64 bytes) const;

    /* Begin Model stuff... */
    enum Roles {
        FileNameRole = Qt::UserRole,
        FilePathRole,
        IsDirRole,
        IsImageRole,
        IsVideoRole,
        IsSurroundRole,
        FileSizeRole,
        FileCreatedRole,
        FileStereoModeRole,
        FileStereoSwapRole,
        FileTypeStringRole
    };

    QSqlRecord getRecordForFile(const QFileInfo& file) const;

    bool isCurrentFileStereoImage() const;
    bool isCurrentFileImage() const;
    bool isCurrentFileVideo() const;

    int currentFileAudioTrack() const;
    void setCurrentFileAudioTrack(int track);

    bool isCurrentFileSurround() const;
    void setCurrentFileSurround(bool surround);

    DVSourceMode::Type currentFileStereoMode() const;
    void setCurrentFileStereoMode(DVSourceMode::Type mode);

    bool currentFileStereoSwap() const;
    void setCurrentFileStereoSwap(bool swap);

    void updateRecordForFile(const QFileInfo& file, const QString& propertyName, QVariant value);
    void updateRecordForFile(const QFileInfo& file, const QString& propertyName, QVariant value, Roles role);

    qint64 currentFileSize() const;
    QString currentFileInfo() const;
    QString fileTypeString(const QFileInfo& file) const;

    bool isFileStereoImage(const QFileInfo& file) const;
    bool isFileImage(const QFileInfo& file) const;
    bool isFileVideo(const QFileInfo& file) const;
    bool isFileSurround(const QFileInfo& file) const;

    DVSourceMode::Type fileStereoMode(const QFileInfo& file) const;
    bool fileStereoSwap(const QFileInfo& file) const;

    QHash<int, QByteArray> roleNames() const;

    /* The function that gives QML the different properties for a given file in the current dir. */
    QVariant data(const QModelIndex &index, int role) const;

    /* How many files are in the current dir? */
    int rowCount(const QModelIndex& parent) const;

    bool initDir(const QString& dir);

    QString startDir();
    void setStartDir(QString path);

    QString snapshotDir();
    void setSnapshotDir(QString path);

    bool fileBrowserOpen() const;
    void setFileBrowserOpen(bool open);

    void setupFileDatabase();
    Q_INVOKABLE void resetFileDatabase();

    DVQmlCommunication* qmlCommunication;

signals:
    void currentFileChanged();
    void currentDirChanged();

    void storageDevicePathsChanged();

    void historyChanged();
    void bookmarksChanged();
    void startDirChanged();
    void snapshotDirChanged(QString dir);

    void fileBrowserOpenChanged();

    void currentFileStereoModeChanged();
    void currentFileStereoSwapChanged();
    void currentFileSurroundChanged();
    void currentFileAudioTrackChanged();
};
