#include "dvfolderlisting.hpp"
#include <QStorageInfo>
#include <QSettings>

DVFolderListing::DVFolderListing(QObject *parent, QSettings& s) : QObject(parent), settings(s), currentHistory(-1), driveTimer(this) {
    if (settings.contains("Bookmarks"))
        m_bookmarks = settings.value("Bookmarks").toStringList();

    /* TODO - Figure out a way to detect when there is actually a change rather than just putting it on a timer. */
    connect(&driveTimer, &QTimer::timeout, this, &DVFolderListing::storageDevicePathsChanged);
    driveTimer.start(8000);
}

QStringList DVFolderListing::getStorageDevicePaths() const {
    QStringList paths;

    for (QStorageInfo info : QStorageInfo::mountedVolumes())
#ifdef Q_OS_ANDROID
        /* In my experience anything that doesn't have "storage" or "sdcard" in it on Android is useless. */
        if (info.rootPath().contains("storage") || info.rootPath().contains("sdcard"))
#endif
        paths.append(info.rootPath() + ';' + info.displayName());


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

QString DVFolderListing::goBack() {
    --currentHistory;
    emit historyChanged();
    return browserHistory[currentHistory];
}

QString DVFolderListing::goForward() {
    ++currentHistory;
    emit historyChanged();
    return browserHistory[currentHistory];
}

void DVFolderListing::pushHistory(QString value) {
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
            if (canGoForward())
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
        bookmarksChanged(m_bookmarks);
        settings.setValue("Bookmarks", m_bookmarks);
    }
}

void DVFolderListing::deleteBookmark(QString bookmark) {
    /* Only emit signal and update setting if something was actually deleted. */
    if (m_bookmarks.removeAll(bookmark) != 0) {
        bookmarksChanged(m_bookmarks);
        settings.setValue("Bookmarks", m_bookmarks);
    }
}

QStringList DVFolderListing::bookmarks() const {
    return m_bookmarks;
}

