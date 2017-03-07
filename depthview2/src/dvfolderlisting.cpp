#include "dvfolderlisting.hpp"
#include <QStorageInfo>
#include <QSettings>
#include <QDateTime>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>

DVFolderListing::DVFolderListing(QObject *parent, QSettings& s) : QAbstractListModel(parent),
    settings(s), currentHistory(-1), driveTimer(this), m_fileBrowserOpen(false) {
    if (settings.contains("Bookmarks"))
        m_bookmarks = settings.value("Bookmarks").toStringList();

    /* Use the path of the settings file to get the path for the database. */
    QString path = settings.fileName();
    path.remove(path.lastIndexOf('.'), path.length()).append(".db");
    m_fileDataDB = QSqlDatabase::addDatabase("QSQLITE");
    m_fileDataDB.setDatabaseName(path);

    if (!m_fileDataDB.open())
       qWarning("Error opening database!");

    /* Check to see if the table exists. */
    if (m_fileDataDB.record("files").isEmpty()) {
        /* TODO - Handle potential changes/additions to database fields. */
        QSqlQuery query("create table files (path string, stereoMode integer, stereoSwap bool)");
        if (query.lastError().isValid()) qWarning("Error creating table! %s", qPrintable(query.lastError().text()));
    }

    initDir(QDir::currentPath());

    /* TODO - What other video types can we do? */
    stereoImageSuffixes << "jps" << "pns";
    imageSuffixes << "jpg" << "jpeg" << "png" << "bmp" << stereoImageSuffixes;
    videoSuffixes << "avi" << "mp4" << "m4v" << "mkv" << "ogv" << "ogg" << "webm" << "flv" << "3gp" << "wmv" << "mpg";

    QStringList nameFilters;

    for (const QString& suffix : imageSuffixes)
        nameFilters << "*." + suffix;
    for (const QString& suffix : videoSuffixes)
        nameFilters << "*." + suffix;

    /* These extensions are the supported stereo image/video formats. */
    m_currentDir.setNameFilters(nameFilters);

    m_currentDir.setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Files);
    m_currentDir.setSorting(QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);

    /* When the file changes, the stereo settings change. */
    connect(this, &DVFolderListing::currentFileChanged, this, &DVFolderListing::currentFileStereoModeChanged);
    connect(this, &DVFolderListing::currentFileChanged, this, &DVFolderListing::currentFileStereoSwapChanged);

    /* TODO - Figure out a way to detect when there is actually a change rather than just putting it on a timer. */
    connect(&driveTimer, &QTimer::timeout, this, &DVFolderListing::storageDevicePathsChanged);
    driveTimer.start(8000);
}

void DVFolderListing::postQmlInit() {
    if (!m_currentFile.exists())
        /* Must be done after QML is all set up so that the file browser popup gets the signal. */
        setFileBrowserOpen(settings.value("StartupFileBrowser").toBool());
}

void DVFolderListing::openNext() {
    /* Just files with the default name filter please. */
    QFileInfoList entryList = m_currentDir.entryInfoList(QDir::Files);

    if(!entryList.empty()){
        /* Try to find the current file in the list. */
        int index = entryList.indexOf(m_currentFile);
        ++index;

        /* Wrap the index value if it ends up outside the list bounds. */
        if(index >= entryList.count())
            index = 0;

        openFile(entryList[index]);
    }
}

void DVFolderListing::openPrevious() {
    /* Just files with the default name filter please. */
    QFileInfoList entryList = m_currentDir.entryInfoList(QDir::Files);

    if(!entryList.empty()){
        /* Try to find the current file in the list. */
        int index = entryList.indexOf(m_currentFile);

        --index;

        /* Wrap the index value if it ends up outside the list bounds. */
        if(index < 0)
            index = entryList.count() - 1;

        openFile(entryList[index]);
    }
}

QString DVFolderListing::bytesToString(qint64 bytes) const {
    int unit;
    const QString units[] = {tr("Bytes"), tr("kB"), tr("MB"), tr("GB")};

    /* Multiply by 10 to have one decimal point. */
    bytes *= 10;

    for (unit=-1; (++unit<3) && (bytes > 10239); bytes /= 1024);

    return QString::number(bytes * 0.1f, 'f', 1) + ' ' + units[unit];
}

QString DVFolderListing::currentFile() const {
    return m_currentFile.fileName();
}

QUrl DVFolderListing::currentURL() const {
    /* The URL is always the absolute path of the file. */
    return encodeURL(m_currentFile.absoluteFilePath());
}

bool DVFolderListing::openFile(QFileInfo fileInfo) {
    if (fileInfo != m_currentFile) {
        if (!isFileImage(fileInfo) && !isFileVideo(fileInfo))
            /* Not an image or a video, not something we can open. */
            return false;

        /* Check to see if the file is in the current dir, and if it isn't update the dir. Use a new QDir because
         * filters are part of QDir comparisons, and sometimes the QString path has variations that will break things. */
        if (fileInfo.dir() != QDir(m_currentDir.path()))
            setCurrentDir(fileInfo.absolutePath());

        /* Close the file browser if it was open. */
        setFileBrowserOpen(false);

        m_currentFile = fileInfo;
        emit currentFileChanged();
    }
    /* If the file was already open or was opened, we're good. */
    return true;
}

bool DVFolderListing::openFile(QUrl url) {
    return openFile(QFileInfo(decodeURL(url)));
}

QUrl DVFolderListing::currentDir() const {
    return encodeURL(m_currentDir.absolutePath());
}

void DVFolderListing::setCurrentDir(QUrl url) {
    setCurrentDir(url.toLocalFile());
}

void DVFolderListing::setCurrentDir(QString dir) {
    /* Tell the model system that we're going to be changing all the things. */
    beginResetModel();

    if (m_currentDir.cd(dir)) {
        pushHistory();
        emit currentDirChanged();
    }

    /* Tell the model system that we've finished changing all the things. */
    endResetModel();
}

bool DVFolderListing::canGoUp() const {
    /* Use a copy of currentDir to test whether or not cdUp() will work. */
    QDir tmp = m_currentDir;
    return tmp.cdUp();
}

void DVFolderListing::goUp() {
    /* Calling m_currentDir.cd("..") is equivalent to m_currentDir.cdUp() anyway... */
    setCurrentDir("..");
}

QStringList DVFolderListing::getStorageDevicePaths() const {
    QStringList paths;

    for (QStorageInfo info : QStorageInfo::mountedVolumes())
#ifdef Q_OS_ANDROID
        /* In my experience anything that doesn't have "storage" or "sdcard" in it on Android is useless. */
        if (info.rootPath().contains("storage") || info.rootPath().contains("sdcard"))
#endif
        paths.append(info.rootPath() + ';' + info.displayName() + ";" + QString::number(info.bytesTotal()));


    return paths;
}

bool DVFolderListing::fileExists(QString file) const {
    return QFile::exists(file);
}
bool DVFolderListing::dirExists(QString dir) const {
    return QDir(dir).exists();
}

QUrl DVFolderListing::encodeURL(QString url) const {
    return QUrl::fromLocalFile(url);
}
QString DVFolderListing::decodeURL(QUrl url) const {
    return url.toLocalFile();
}

void DVFolderListing::goBack() {
    setCurrentDir(browserHistory[--currentHistory]);
    emit historyChanged();
}

void DVFolderListing::goForward() {
    setCurrentDir(browserHistory[++currentHistory]);
    emit historyChanged();
}

void DVFolderListing::pushHistory() {
    QString value = m_currentDir.absolutePath();

    /* We ignore if value is empty or equal to the current history item. */
    if (!value.isEmpty() && (browserHistory.isEmpty() || browserHistory[currentHistory] != value)) {
        /* If the next item is equal to the passed value we keep the current history and just increment it. */
        if (canGoForward() && browserHistory[currentHistory + 1] == value)
            ++currentHistory;
        /* Same for back, just decrement instead of increment. */
        else if (canGoBack() && browserHistory[currentHistory - 1] == value)
            --currentHistory;
        else {
            ++currentHistory;

            /* If there are any forward entries, they must be cleared before adding the new entry. */
            if (currentHistory < browserHistory.size())
                browserHistory.erase(browserHistory.begin() + currentHistory, browserHistory.end());

            browserHistory.append(value);
        }

        /* At this point no matter which branch it went through it has changed. */
        emit historyChanged();
    }
}

bool DVFolderListing::canGoBack() const {
    /* We can go back if the list isn't empty and we aren't at the first item in the list. */
    return !browserHistory.isEmpty() && currentHistory > 0;
}

bool DVFolderListing::canGoForward() const {
    /* We can go forward if currentHistory isn't the last item in the list. */
    return !browserHistory.isEmpty() && currentHistory < (browserHistory.size() - 1);
}

void DVFolderListing::addBookmark(QString bookmark) {
    /* Don't add existing bookmarks, and don't add directories that don't exist. */
    if (!m_bookmarks.contains(bookmark) && dirExists(decodeURL(bookmark))) {
        m_bookmarks.append(bookmark);
        settings.setValue("Bookmarks", m_bookmarks);
        bookmarksChanged();
    }
}

void DVFolderListing::deleteBookmark(QString bookmark) {
    /* Only emit signal and update setting if something was actually deleted. */
    if (m_bookmarks.removeAll(bookmark) != 0) {
        settings.setValue("Bookmarks", m_bookmarks);
        bookmarksChanged();
    }
}

QStringList DVFolderListing::bookmarks() const {
    return m_bookmarks;
}

QSqlRecord DVFolderListing::getRecordForFile(const QFileInfo& file, bool create) const {
    if (file.exists()) {
        QSqlQuery query(m_fileDataDB);
        query.prepare("SELECT * FROM files WHERE path = (:path)");
        query.bindValue(":path", file.canonicalFilePath());

        if (query.exec() && query.next())
            return query.record();

        /* If it didn't exist, optionally create it. */
        if (create) {
            query.prepare("INSERT INTO files (path, stereoMode, stereoSwap) VALUES (:path, :mode, :swap)");
            query.bindValue(":path", file.canonicalFilePath());
            /* Get the default values for the stereoMode and stereoSwap variables. */
            query.bindValue(":mode", fileStereoMode(file));
            query.bindValue(":swap", fileStereoSwap(file));

            if (query.exec())
                return query.record();

            qWarning("Unable to create record for file! %s", qPrintable(query.lastError().text()));
        }
    }

    return QSqlRecord();
}

bool DVFolderListing::isCurrentFileStereoImage() const {
    return isFileStereoImage(m_currentFile);
}
bool DVFolderListing::isCurrentFileImage() const {
    return isFileImage(m_currentFile);
}
bool DVFolderListing::isCurrentFileVideo() const {
    return isFileVideo(m_currentFile);
}

DVSourceMode::Type DVFolderListing::currentFileStereoMode() const {
    return fileStereoMode(m_currentFile);
}
void DVFolderListing::setCurrentFileStereoMode(DVSourceMode::Type mode) {
    if (mode == currentFileStereoMode())
        return;

    /* Get or create the record for the selected file. */
    QSqlRecord record = getRecordForFile(m_currentFile, true);

    QSqlQuery query(m_fileDataDB);
    query.prepare("UPDATE files SET stereoMode = :mode WHERE path = :path");
    query.bindValue(":mode", mode);
    /* Use the record's path value. */
    query.bindValue(":path", record.value(0));

    if (!query.exec())
        qWarning("Unable to update record for file! %s", qPrintable(query.lastError().text()));

    emit currentFileStereoModeChanged();
}

bool DVFolderListing::currentFileStereoSwap() const {
    return fileStereoSwap(m_currentFile);
}
void DVFolderListing::setCurrentFileStereoSwap(bool swap) {
    if (swap == currentFileStereoSwap())
        return;

    /* Get or create the record for the selected file. */
    QSqlRecord record = getRecordForFile(m_currentFile, true);

    QSqlQuery query(m_fileDataDB);
    query.prepare("UPDATE files SET stereoSwap = :swap WHERE path = :path");
    query.bindValue(":swap", swap);
    /* Use the record's path value. */
    query.bindValue(":path", record.value(0));

    if (!query.exec())
        qWarning("Unable to update record for file! %s", qPrintable(query.lastError().text()));

    emit currentFileStereoSwapChanged();
}

qint64 DVFolderListing::currentFileSize() const {
    return m_currentFile.size();
}
QString DVFolderListing::currentFileInfo() const {
    QString info = tr("<h1>Media Info:</h1>%1<br>Type: %2<br>File Size: %3<br>Date Created: %4")
            .arg(m_currentFile.absoluteFilePath()).arg(fileTypeString(m_currentFile))
            .arg(bytesToString(m_currentFile.size())).arg(m_currentFile.created().toString());

    QString owner = m_currentFile.owner();
    if (!owner.isEmpty())
        info += tr("<br>Owner: ") + owner;

    return info;
}
QString DVFolderListing::fileTypeString(const QFileInfo& file) const {
    return file.isDir() ? tr("Folder") : isFileStereoImage(file) ? tr("Stereo Image") : isFileVideo(file) ? tr("Video") : tr("Image");
}

bool DVFolderListing::isFileStereoImage(const QFileInfo& info) const {
    return !info.isDir() && stereoImageSuffixes.contains(info.suffix(), Qt::CaseInsensitive);
}
bool DVFolderListing::isFileImage(const QFileInfo& info) const {
    return !info.isDir() && imageSuffixes.contains(info.suffix(), Qt::CaseInsensitive);
}
bool DVFolderListing::isFileVideo(const QFileInfo& info) const {
    return !info.isDir() && videoSuffixes.contains(info.suffix(), Qt::CaseInsensitive);
}
DVSourceMode::Type DVFolderListing::fileStereoMode(const QFileInfo& file) const {
    /* Directories are side-by-side because of their thumbnail. */
    if(file.isDir() || stereoImageSuffixes.contains(file.suffix(), Qt::CaseInsensitive))
        return DVSourceMode::SidebySide;

    QSqlRecord record = getRecordForFile(file);

    if (!record.isEmpty())
        return record.value("stereoMode").value<DVSourceMode::Type>();

    /* TODO - Try to find a way to detect the mode. */
    return DVSourceMode::Mono;
}
bool DVFolderListing::fileStereoSwap(const QFileInfo& file) const {
    QSqlRecord record = getRecordForFile(file);

    if (record.isEmpty())
        return isFileStereoImage(file);

    return record.value("stereoSwap").toBool();
}

QHash<int, QByteArray> DVFolderListing::roleNames() const {
    QHash<int, QByteArray> names;

    names[FileNameRole]         = "fileName";
    names[FilePathRole]         = "fileURL";
    names[IsDirRole]            = "fileIsDir";
    names[IsImageRole]          = "fileIsImage";
    names[IsVideoRole]          = "fileIsVideo";
    names[FileSizeRole]         = "fileSize";
    names[FileCreatedRole]      = "fileCreated";
    names[FileStereoModeRole]   = "fileStereoMode";
    names[FileStereoSwapRole]   = "fileStereoSwap";
    names[FileTypeStringRole]   = "fileTypeString";

    return names;
}

QVariant DVFolderListing::data(const QModelIndex& index, int role) const {
    QVariant data;

    if (int(m_currentDir.count()) > index.row()) {
        QFileInfo info = m_currentDir.entryInfoList()[index.row()];

        /* Set the return value based on the role. */
        switch (role) {
        case FileNameRole:
            data = info.fileName();
            break;
        case FilePathRole:
            data = QUrl::fromLocalFile(info.absoluteFilePath());
            break;
        case IsDirRole:
            data = info.isDir();
            break;
        case IsImageRole:
            data = isFileImage(info);
            break;
        case IsVideoRole:
            data = isFileVideo(info);
            break;
        case FileSizeRole:
            data = info.size();
            break;
        case FileCreatedRole:
            data = info.created().toString();
            break;
        case FileStereoModeRole:
            data = fileStereoMode(info);
            break;
        case FileStereoSwapRole:
            data = fileStereoSwap(info);
            break;
        case FileTypeStringRole:
            data = fileTypeString(info);
            break;
        }
    }
    return data;
}

int DVFolderListing::rowCount(const QModelIndex& parent) const {
    Q_UNUSED(parent)
    return m_currentDir.count();
}

bool DVFolderListing::initDir(const QString& dir) {
    /* Clear any stored history and reset the counter. */
    browserHistory.clear();
    currentHistory = -1;

    /* Return false if cd fails. */
    if (!m_currentDir.cd(dir))
        return false;

    pushHistory();

    /* It's all good. */
    return true;
}

bool DVFolderListing::fileBrowserOpen() const {
    return m_fileBrowserOpen;
}

void DVFolderListing::setFileBrowserOpen(bool open) {
    if (open != m_fileBrowserOpen) {
        m_fileBrowserOpen = open;
        emit fileBrowserOpenChanged();
    }
}
