#include "version.hpp"
#include "dvwindow.hpp"
#include "dvqmlcommunication.hpp"
#include "dvfolderlisting.hpp"
#include "dvthumbnailprovider.hpp"
#include "dvpluginmanager.hpp"
#include "dvfilevalidator.hpp"
#include "dvconfig.hpp"
#include <QApplication>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlContext>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QMimeData>
#include <QOpenGLFramebufferObject>
#include <QSqlDatabase>
#include <AVPlayer.h>

#ifdef DV_PORTABLE
/* Portable builds store settings in a "DepthView.conf" next to the application executable. */
#define SETTINGS_ARGS QApplication::applicationDirPath() + "/DepthView.conf", QSettings::IniFormat
#else
/* Non-portable builds use an ini file in "%APPDATA%/chipgw" or "~/.config/chipgw". */
#define SETTINGS_ARGS QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName()
#endif

DVWindow::DVWindow() : QQuickWindow(), settings(SETTINGS_ARGS), renderFBO(nullptr), sphereTris(QOpenGLBuffer::IndexBuffer) {
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

    /* Let these classes see each other. */
    qmlCommunication->folderListing = folderListing;
    folderListing->qmlCommunication = qmlCommunication;
    qmlEngine = new QQmlEngine(this);

    if (qmlEngine->incubationController() == nullptr)
        qmlEngine->setIncubationController(incubationController());

    qmlEngine->rootContext()->setContextProperty("DepthView", qmlCommunication);
    qmlEngine->rootContext()->setContextProperty("FolderListing", folderListing);
    qmlEngine->rootContext()->setContextProperty("PluginManager", pluginManager);

    /* When the Qt.quit() function is called in QML, close this window. */
    connect(qmlEngine, &QQmlEngine::quit, this, &DVWindow::close);

    qmlEngine->addImageProvider("video", new DVThumbnailProvider);

    qmlRegisterUncreatableType<DVDrawMode>(DV_URI_VERSION, "DrawMode", "Only for enum values.");
    qmlRegisterUncreatableType<DVSourceMode>(DV_URI_VERSION, "SourceMode", "Only for enum values.");
    qmlRegisterType<DVFileValidator>(DV_URI_VERSION, "FileValidator");
    qRegisterMetaType<DVFolderListing*>();

    /* Update window title whenever file changes. */
    connect(folderListing, &DVFolderListing::currentDirChanged, this, &DVWindow::updateTitle);
    connect(folderListing, &DVFolderListing::currentFileChanged, this, &DVWindow::updateTitle);
    connect(folderListing, &DVFolderListing::fileBrowserOpenChanged, this, &DVWindow::updateTitle);

    connect(this, &QQuickWindow::frameSwapped, this, &DVWindow::onFrameSwapped, Qt::DirectConnection);
    connect(this, &QQuickWindow::sceneGraphInitialized, this, &DVWindow::initializeGL, Qt::DirectConnection);
    connect(this, &QQuickWindow::afterRendering, this, &DVWindow::paintGL, Qt::DirectConnection);
    connect(this, &QQuickWindow::beforeRendering, this, &DVWindow::preSync, Qt::DirectConnection);

    /* We render a cursor inside QML so it is shown for both eyes. */
    setCursor(Qt::BlankCursor);

    setMinimumSize(QSize(1000, 500));
    setGeometry(0, 0, 1000, 600);

    pluginManager->loadPlugins(qmlEngine);

    QQmlComponent rootComponent(qmlEngine);

    rootComponent.loadUrl(QUrl(QStringLiteral("qrc:/qml/Window.qml")));

    /* Wait for it to load... */
    while (rootComponent.isLoading());

    /* The program can't run if there was an error. */
    if (rootComponent.isError())
        qFatal(qPrintable(rootComponent.errorString()));

    qmlRoot = qobject_cast<QQuickItem*>(rootComponent.create());

    /* Critical error! abort! abort! */
    if (qmlRoot == nullptr)
        qFatal(qPrintable(rootComponent.errorString()));

    /* This is the root item, make it so. */
    qmlRoot->setParentItem(contentItem());
    setColor(QColor(0, 0, 0, 0));

    player = qmlRoot->findChild<QtAV::AVPlayer*>();
    if (player == nullptr)
        qFatal("Unable to find AVPlayer!");

    /* The setGeometry() and setState() calls may try to set the qmlRoot geometry,
     * which means this needs to be done after QML is all set up. */
    if (settings.childGroups().contains("Window")) {
        /* Restore window state from the stored geometry. */
        settings.beginGroup("Window");
        setGeometry(settings.value("Geometry").toRect());
        setWindowState(Qt::WindowState(settings.value("State").toInt()));
        settings.endGroup();
    }
}

DVWindow::~DVWindow() {
    pluginManager->unloadPlugins();

    delete renderFBO;

    if (qmlCommunication->saveWindowState()) {
        /* Save the window geometry so that it can be restored next run. */
        settings.beginGroup("Window");
        settings.setValue("Geometry", geometry());
        settings.setValue("State", windowState());
        settings.endGroup();
    }
}

void DVWindow::updateQmlSize() {
    qmlSize = size();

    /* If Side-by-Side and not anamorphic we only render QML at half of the window size (horizontally). */
    if(qmlCommunication->drawMode() == DVDrawMode::SidebySide && !qmlCommunication->anamorphicDualView())
        qmlSize.setWidth(qmlSize.width() / 2);

    /* If Top/Bottom and not anamorphic we only render QML at half of the window size (vertically). */
    if(qmlCommunication->drawMode() == DVDrawMode::TopBottom && !qmlCommunication->anamorphicDualView())
        qmlSize.setHeight(qmlSize.height() / 2);

    if (qmlCommunication->drawMode() == DVDrawMode::Plugin)
        qmlSize = pluginManager->getPluginSize(qmlSize);

    /* Don't recreate fbo unless it's null or its size is wrong. */
    if (renderFBO == nullptr || renderFBO->size() != qmlSize)
        createFBO();

    qmlRoot->setSize(qmlSize);
}

void DVWindow::updateTitle() {
   setTitle((folderListing->fileBrowserOpen() ? folderListing->currentDir().toLocalFile() : folderListing->currentFile()) + " - DepthView");
}

bool DVWindow::event(QEvent* e) {
    switch (e->type()) {
    case QEvent::Leave:
        /* TODO - This still doesn't always work right, but it's better than using setMouseGrabEnabled()... */
        if (holdMouse) {
            QPoint pos = mapFromGlobal(QCursor::pos());

            /* Generate a new coordinate on screen. */
            pos.setX(qBound(1, pos.x(), qmlSize.width()-1));
            pos.setY(qBound(1, pos.y(), qmlSize.height()-1));

            /* Will generate a new event. */
            QCursor::setPos(mapToGlobal(pos));

            return true;
        }
        break;
    case QEvent::MouseMove:
        /* If holding the mouse, make sure it's inside the QML render area. */
        if (holdMouse && !QRect(QPoint(), qmlSize).contains(mapFromGlobal(QCursor::pos()), true)) {
            QPoint pos = mapFromGlobal(QCursor::pos());

            /* Generate a new coordinate on screen. */
            pos.setX(qBound(1, pos.x(), qmlSize.width()-1));
            pos.setY(qBound(1, pos.y(), qmlSize.height()-1));

            /* Will generate a new event. */
            QCursor::setPos(mapToGlobal(pos));

            return true;
        }
        /* We also emit a special signal for this one so that the fake cursor
         * can be set to the right position without having a MouseArea that absorbs events. */
        emit qmlCommunication->mouseMoved(static_cast<QMouseEvent*>(e)->localPos());
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
        /* Don't relay the event to QOpenGLWindow. */
        return true;
    }
    default:
        break;
    }

    return QQuickWindow::event(e);
}

void DVWindow::doCommandLine(QCommandLineParser& parser) {
    /* We use one string to hold all warning messages, so we only have to show one dialog. */
    QString warning;

    /* Print the valid modes to the console.
     * Still inits & shows the window because to get plugin modes requires the plugin system to be inited. */
    if (parser.isSet("l"))
        qDebug() << "Valid render modes:" << pluginManager->getModes();

    if (parser.isSet("f"))
        setWindowState(Qt::WindowFullScreen);

    if (parser.isSet("d") && !folderListing->initDir(parser.value("d")))
        warning += tr("<p>Invalid directory \"%1\" passed to \"--startdir\" argument!</p>").arg(parser.value("d"));

    if (parser.isSet("r")) {
        const QString& renderer = parser.value("r");

        int mode = pluginManager->getModes().indexOf(renderer);

        if(mode == -1)
            warning += tr("<p>Invalid renderer \"%1\" passed to \"--renderer\" argument!</p>").arg(renderer);

        if (mode >= DVDrawMode::Plugin) {
            pluginManager->setPluginMode(renderer);
            qmlCommunication->initDrawMode(DVDrawMode::Plugin);
        } else {
            qmlCommunication->initDrawMode(DVDrawMode::Type(mode));
        }
    }

    for (const QString& arg : parser.positionalArguments()) {
        QFileInfo file(arg);

        /* The file extension is checked by openFile(). */
        if (file.exists() && folderListing->openFile(file)) break;
    }

    /* If there weren't any warnings we don't show the dialog. */
    if(!warning.isEmpty())
        QMessageBox::warning(nullptr, tr("Invalid Command Line!"), warning);
}

DVInputMode::Type DVWindow::inputMode() const {
    return folderListing->fileBrowserOpen() ? DVInputMode::FileBrowser : folderListing->isCurrentFileVideo() ? DVInputMode::VideoPlayer : DVInputMode::ImageViewer;
}

void DVWindow::up() {
    emit qmlCommunication->up();
}
void DVWindow::down() {
    emit qmlCommunication->down();
}
void DVWindow::left() {
    emit qmlCommunication->left();
}
void DVWindow::right() {
    emit qmlCommunication->right();
}

void DVWindow::accept() {
    emit qmlCommunication->accept();
}

void DVWindow::cancel() {
    emit qmlCommunication->cancel();
}

void DVWindow::openFileBrowser() {
    folderListing->setFileBrowserOpen(true);
}

void DVWindow::goBack() {
    if (folderListing->fileBrowserOpen() && folderListing->canGoBack())
        folderListing->goBack();
}

void DVWindow::goForward() {
    if (folderListing->fileBrowserOpen() && folderListing->canGoForward())
        folderListing->goForward();
}

void DVWindow::goUp() {
    if (folderListing->fileBrowserOpen() && folderListing->canGoUp())
        folderListing->goUp();
}

void DVWindow::fileInfo() {
    emit qmlCommunication->fileInfo();
}

void DVWindow::nextFile() {
    folderListing->openNext();
}

void DVWindow::previousFile() {
    folderListing->openPrevious();
}

void DVWindow::zoomActual() {
    emit qmlCommunication->zoomActual();
}

void DVWindow::zoomFit() {
    emit qmlCommunication->zoomFit();
}

void DVWindow::playVideo() {
    player->play();
}

void DVWindow::pauseVideo() {
    player->pause();
}

void DVWindow::playPauseVideo() {
    player->togglePause();
}

void DVWindow::seekBack() {
    player->seekBackward();
}

void DVWindow::seekForward() {
    player->seekForward();
}

void DVWindow::seekAmount(qint64 msec) {
    player->seek(msec);
}

void DVWindow::volumeUp() {
    player->audio()->setVolume(qMin(player->audio()->volume() + 0.1, 1.0));
}

void DVWindow::volumeDown() {
    player->audio()->setVolume(qMax(player->audio()->volume() - 0.1, 0.0));
}

void DVWindow::mute() {
    player->audio()->setMute(!player->audio()->isMute());
}

void DVWindow::setVolume(qreal volume) {
    player->audio()->setVolume(volume);
}
