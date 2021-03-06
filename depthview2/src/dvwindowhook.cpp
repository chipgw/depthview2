#include "version.hpp"
#include "dvwindowhook.hpp"
#include "dvqmlcommunication.hpp"
#include "dvfolderlisting.hpp"
#include "dvrenderer.hpp"
#include "dvthumbnailprovider.hpp"
#include "dvpluginmanager.hpp"
#include "dvfilevalidator.hpp"
#include "dvconfig.hpp"
#include "dvvirtualscreenmanager.hpp"
#include <QApplication>
#include <QQuickWindow>
#include <QQmlContext>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QMimeData>
#include <QSqlDatabase>
#include <AVPlayer.h>
#include <VideoCapture.h>

#ifdef DV_PORTABLE
/* Portable builds store settings in a "DepthView.conf" next to the application executable. */
#define SETTINGS_ARGS QApplication::applicationDirPath() + "/DepthView.conf", QSettings::IniFormat
#else
/* Non-portable builds use an ini file in "%APPDATA%/chipgw" or "~/.config/chipgw". */
#define SETTINGS_ARGS QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName()
#endif

DVWindowHook::DVWindowHook(QQmlApplicationEngine* engine) : QObject(engine), settings(SETTINGS_ARGS) {
    /* Use the path of the settings file to get the path for the database. */
    QString path = settings.fileName();
    path.remove(path.lastIndexOf('.'), path.length()).append(".db");
    QSqlDatabase dataDB = QSqlDatabase::addDatabase("QSQLITE");
    dataDB.setDatabaseName(path);

    if (!dataDB.open())
       qWarning("Error opening database!");

    qmlCommunication = new DVQmlCommunication(this, settings);
    folderListing = new DVFolderListing(this, settings);
    pluginManager = new DVPluginManager(this, settings);
    renderer = new DVRenderer(this, settings, *qmlCommunication, *folderListing);

    /* Let these classes see each other. */
    qmlCommunication->folderListing = folderListing;
    folderListing->qmlCommunication = qmlCommunication;

    engine->rootContext()->setContextProperty("DepthView", qmlCommunication);
    engine->rootContext()->setContextProperty("FolderListing", folderListing);
    engine->rootContext()->setContextProperty("PluginManager", pluginManager);
    engine->rootContext()->setContextProperty("VRManager", renderer->vrManager);

    engine->addImageProvider("video", new DVThumbnailProvider);

    qmlRegisterUncreatableType<DVDrawMode>(DV_URI_VERSION, "DrawMode", "Only for enum values.");
    qmlRegisterUncreatableType<DVSourceMode>(DV_URI_VERSION, "SourceMode", "Only for enum values.");
    qmlRegisterType<DVFileValidator>(DV_URI_VERSION, "FileValidator");
    qRegisterMetaType<DVFolderListing*>();

    /* Update window title whenever file changes. */
    connect(folderListing, &DVFolderListing::currentDirChanged, this, &DVWindowHook::updateTitle);
    connect(folderListing, &DVFolderListing::currentFileChanged, this, &DVWindowHook::updateTitle);
    connect(folderListing, &DVFolderListing::fileBrowserOpenChanged, this, &DVWindowHook::updateTitle);

    connect(qmlCommunication, &DVQmlCommunication::takeSnapshot, this, &DVWindowHook::takeSnapshot);

    pluginManager->loadPlugins(engine);

    engine->load("qrc:/qml/Window.qml");

    window = qobject_cast<QQuickWindow*>(engine->rootObjects().first());

    if (window == nullptr)
        qFatal("Unable to get window from QML!");

    renderer->setWindow(window);

    /* We render a cursor inside QML so it is shown for both eyes. */
    window->setCursor(Qt::BlankCursor);

    window->setMinimumSize(QSize(1000, 500));
    window->setGeometry(0, 0, 1000, 600);

    connect(window, &QQuickWindow::beforeSynchronizing, this, &DVWindowHook::preSync, Qt::DirectConnection);

    /* This is the root item, make it so. */
    window->setColor(QColor(0, 0, 0, 0));

    player = window->findChild<QtAV::AVPlayer*>();
    if (player == nullptr)
        qFatal("Unable to find AVPlayer!");

    if (settings.contains("SnapshotDir"))
        player->videoCapture()->setCaptureDir(settings.value("SnapshotDir").toString());

    connect(player->videoCapture(), &QtAV::VideoCapture::saved, this, &DVWindowHook::imageCaptured, Qt::DirectConnection);
    connect(folderListing, &DVFolderListing::snapshotDirChanged, player->videoCapture(), &QtAV::VideoCapture::setCaptureDir);

    window->installEventFilter(this);

    window->show();

    /* The setGeometry() and setState() calls may try to set the qmlRoot geometry,
     * which means this needs to be done after QML is all set up. */
    if (settings.childGroups().contains("Window")) {
        /* Restore window state from the stored geometry. */
        settings.beginGroup("Window");
        window->setGeometry(settings.value("Geometry").toRect());
        window->setWindowState(Qt::WindowState(settings.value("State").toInt()));
        settings.endGroup();
    }
}

DVWindowHook::~DVWindowHook() {
    /* If the GL thread is currently rendering, wait for it to finish. */
    deleteLock.lock();

    pluginManager->unloadPlugins();

    if (qmlCommunication->saveWindowState()) {
        /* Save the window geometry so that it can be restored next run. */
        settings.beginGroup("Window");
        settings.setValue("Geometry", window->geometry());
        settings.setValue("State", window->windowState());
        settings.endGroup();
    }
}

void DVWindowHook::preSync() {
    pluginManager->doPluginInput(this);
}

void DVWindowHook::updateTitle() {
   window->setTitle((folderListing->fileBrowserOpen() ? folderListing->currentDir().toLocalFile() : folderListing->currentFile()) + " - DepthView");
}

void DVWindowHook::imageCaptured(const QString& filename) {
    QFileInfo info(filename);
    /* Copy the database info for the current file to the new file (excluding audio track because this is an image). */
    folderListing->updateRecordForFile(info, "stereoMode", folderListing->currentFileStereoMode());
    folderListing->updateRecordForFile(info, "stereoSwap", folderListing->currentFileStereoSwap());
    folderListing->updateRecordForFile(info, "surround", folderListing->isCurrentFileSurround());
}

bool DVWindowHook::eventFilter(QObject*, QEvent* e) {
    switch (e->type()) {
    case QEvent::MouseMove:
        /* If holding the mouse, make sure it's inside the QML render area. */
        if (renderer->lockMouse() && !QRect(QPoint(), renderer->qmlSize).contains(window->mapFromGlobal(QCursor::pos()), true)) {
            QPoint pos = window->mapFromGlobal(QCursor::pos());

            /* Generate a new coordinate on screen. */
            pos.setX(qBound(1, pos.x(), renderer->qmlSize.width()-1));
            pos.setY(qBound(1, pos.y(), renderer->qmlSize.height()-1));

            /* Will generate a new event. */
            QCursor::setPos(window->mapToGlobal(pos));

            return true;
        }
        /* We also emit a special signal for this one so that the fake cursor
         * can be set to the right position without having a MouseArea that absorbs events. */
        emit qmlCommunication->mouseMoved(static_cast<QMouseEvent*>(e)->localPos(),
                                          static_cast<QMouseEvent*>(e)->source() == Qt::MouseEventSynthesizedByApplication);
        break;
    case QEvent::TouchBegin:
    case QEvent::TouchEnd:
    case QEvent::TouchUpdate:
    case QEvent::TouchCancel:
        /* TODO - Remap touch location into the modified screen coordinates,
         * in particular for Side by Side & Top/Bottom modes. */

        emit qmlCommunication->touchEvent();
        break;
    case QEvent::DragEnter: {
        QDragEnterEvent* drag = static_cast<QDragEnterEvent*>(e);
        if (drag->mimeData()->hasUrls()) {
            for (const QUrl& url : drag->mimeData()->urls()) {
                QFileInfo info(url.toLocalFile());
                /* If there are any local URLs that exist and match either the video or image filters, accept the drop. */
                if (info.exists() && (folderListing->isFileImage(info) || folderListing->isFileVideo(info))) {
                    drag->acceptProposedAction();
                    return true;
                }
            }
        }
        /* Don't relay the event to QQuickWindow. */
        return true;
    }
    case QEvent::Drop: {
        QDropEvent* drop = static_cast<QDropEvent*>(e);
        if (drop->mimeData()->hasUrls()) {
            for (const QUrl& url : drop->mimeData()->urls()) {
                /* If the file opened properly, accept the drop. */
                if (folderListing->openFile(url)) {
                    drop->acceptProposedAction();
                    return true;
                }
            }
        }
        /* Don't relay the event to QQuickWindow. */
        return true;
    }
    default:
        break;
    }

    return false;
}

void DVWindowHook::doCommandLine(QCommandLineParser& parser) {
    /* We use one string to hold all warning messages, so we only have to show one dialog. */
    QString warning;

    if (parser.isSet("f"))
        window->setWindowState(Qt::WindowFullScreen);

    if (parser.isSet("d") && !folderListing->initDir(parser.value("d")))
        warning += tr("<p>Invalid directory \"%1\" passed to \"--startdir\" argument!</p>").arg(parser.value("d"));

    if (parser.isSet("r")) {
        const QString& renderer = parser.value("r");

        int mode = DVDrawMode::fromString(renderer.toLocal8Bit().data());

        if (mode == -1)
            warning += tr("<p>Invalid renderer \"%1\" passed to \"--renderer\" argument!</p>").arg(renderer);

        qmlCommunication->initDrawMode(DVDrawMode::Type(mode));
    }

    for (const QString& arg : parser.positionalArguments()) {
        QFileInfo file(arg);

        /* The file extension is checked by openFile(). */
        if (file.exists() && folderListing->openFile(file)) break;
    }

    /* If there weren't any warnings we don't show the dialog. */
    if (!warning.isEmpty())
        QMessageBox::warning(nullptr, tr("Invalid Command Line!"), warning);
}

DVInputMode::Type DVWindowHook::inputMode() const {
    return folderListing->fileBrowserOpen() ? DVInputMode::FileBrowser : folderListing->isCurrentFileVideo() ? DVInputMode::VideoPlayer : DVInputMode::ImageViewer;
}

void DVWindowHook::up() {
    emit qmlCommunication->up();
}
void DVWindowHook::down() {
    emit qmlCommunication->down();
}
void DVWindowHook::left() {
    emit qmlCommunication->left();
}
void DVWindowHook::right() {
    emit qmlCommunication->right();
}

void DVWindowHook::accept() {
    emit qmlCommunication->accept();
}

void DVWindowHook::cancel() {
    emit qmlCommunication->cancel();
}

void DVWindowHook::openFileBrowser() {
    folderListing->setFileBrowserOpen(true);
}

void DVWindowHook::goBack() {
    if (folderListing->fileBrowserOpen() && folderListing->canGoBack())
        /* Call as a queued connection in case this is called from the OpenGL thread,
         * because resetting the model must be done in the QML thread. */
        QMetaObject::invokeMethod(folderListing, "goBack", Qt::QueuedConnection);
}

void DVWindowHook::goForward() {
    if (folderListing->fileBrowserOpen() && folderListing->canGoForward())
        /* Call as a queued connection in case this is called from the OpenGL thread,
         * because resetting the model must be done in the QML thread. */
        QMetaObject::invokeMethod(folderListing, "goForward", Qt::QueuedConnection);
}

void DVWindowHook::goUp() {
    if (folderListing->fileBrowserOpen() && folderListing->canGoUp())
        /* Call as a queued connection in case this is called from the OpenGL thread,
         * because resetting the model must be done in the QML thread. */
        QMetaObject::invokeMethod(folderListing, "goUp", Qt::QueuedConnection);
}

void DVWindowHook::fileInfo() {
    emit qmlCommunication->fileInfo();
}

void DVWindowHook::nextFile() {
    folderListing->openNext();
}

void DVWindowHook::previousFile() {
    folderListing->openPrevious();
}

void DVWindowHook::zoomActual() {
    emit qmlCommunication->zoomActual();
}

void DVWindowHook::zoomFit() {
    emit qmlCommunication->zoomFit();
}

void DVWindowHook::playVideo() {
    player->play();
}

void DVWindowHook::pauseVideo() {
    player->pause();
}

void DVWindowHook::playPauseVideo() {
    player->togglePause();
}

void DVWindowHook::seekBack() {
    player->seekBackward();
}

void DVWindowHook::seekForward() {
    player->seekForward();
}

void DVWindowHook::seekAmount(qint64 msec) {
    player->seek(msec);
}

void DVWindowHook::volumeUp() {
    /* Call as a queued connection in case this is called from the OpenGL thread. */
    staticMetaObject.invokeMethod(this, "setVolumeImpl", Qt::QueuedConnection, Q_ARG(qreal, qMin(player->audio()->volume() + 0.1, 1.0)));
}

void DVWindowHook::volumeDown() {
    /* Call as a queued connection in case this is called from the OpenGL thread. */
    staticMetaObject.invokeMethod(this, "setVolumeImpl", Qt::QueuedConnection, Q_ARG(qreal, qMax(player->audio()->volume() - 0.1, 0.0)));
}

void DVWindowHook::mute() {
    /* Call as a queued connection in case this is called from the OpenGL thread. */
    staticMetaObject.invokeMethod(this, "muteImpl", Qt::QueuedConnection);
}

void DVWindowHook::setVolume(qreal volume) {
    /* Call as a queued connection in case this is called from the OpenGL thread. */
    staticMetaObject.invokeMethod(this, "setVolumeImpl", Qt::QueuedConnection, Q_ARG(qreal, volume));
}

void DVWindowHook::takeSnapshot() {
    if (folderListing->isCurrentFileVideo())
        player->videoCapture()->capture();
}

void DVWindowHook::muteImpl() {
    player->audio()->setMute(!player->audio()->isMute());
}

void DVWindowHook::setVolumeImpl(qreal volume) {
    player->audio()->setVolume(volume);
}

QObject* DVWindowHook::inputEventObject() {
    return window;
}
