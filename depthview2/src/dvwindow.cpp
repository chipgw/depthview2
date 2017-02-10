#include "version.hpp"
#include "dvwindow.hpp"
#include "dvqmlcommunication.hpp"
#include "dvfolderlisting.hpp"
#include "dvthumbnailprovider.hpp"
#include <QApplication>
#include <QQuickRenderControl>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlContext>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QMimeData>
#include <QOpenGLFramebufferObject>

/* This class is needed for making forwarded keyboard events be recognized by QML. */
class RenderControl : public QQuickRenderControl {
public:
    RenderControl(DVWindow* win) : QQuickRenderControl(win), window(win) { }

    QWindow* window;

    /* Apparently it has something to do with QML making sure the window has focus... */
    QWindow* renderWindow(QPoint* offset) {
        return window == nullptr ? QQuickRenderControl::renderWindow(offset) : window;
    }
};

#ifdef DV_PORTABLE
/* Portable builds store settings in a "DepthView.conf" next to the application executable. */
#define SETTINGS_ARGS QApplication::applicationDirPath() + "/DepthView.conf", QSettings::IniFormat
#else
/* Non-portable builds use an ini file in "%APPDATA%/chipgw" or "~/.config/chipgw". */
#define SETTINGS_ARGS QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName()
#endif

DVWindow::DVWindow() : QOpenGLWindow(), settings(SETTINGS_ARGS), renderFBO(nullptr) {
    qmlCommunication = new DVQmlCommunication(this, settings);
    folderListing = new DVFolderListing(this, settings);

    /* Use the class defined above. */
    qmlRenderControl = new RenderControl(this);
    qmlWindow = new QQuickWindow(qmlRenderControl);

    qmlEngine = new QQmlEngine(this);

    if (qmlEngine->incubationController() == nullptr)
        qmlEngine->setIncubationController(qmlWindow->incubationController());

    qmlEngine->rootContext()->setContextProperty("DepthView", qmlCommunication);
    qmlEngine->rootContext()->setContextProperty("FolderListing", folderListing);

    /* When the Qt.quit() function is called in QML, close this window. */
    connect(qmlEngine, &QQmlEngine::quit, this, &DVWindow::close);

    qmlEngine->addImageProvider("video", new DVThumbnailProvider);

    qmlRegisterUncreatableType<DVDrawMode>(DV_URI_VERSION, "DrawMode", "Only for enum values.");
    qmlRegisterUncreatableType<DVSourceMode>(DV_URI_VERSION, "SourceMode", "Only for enum values.");

    /* Update QML size whenever draw mode or anamorphic are changed. */
    connect(qmlCommunication, &DVQmlCommunication::drawModeChanged, this, &DVWindow::updateQmlSize);
    connect(qmlCommunication, &DVQmlCommunication::anamorphicDualViewChanged, this, &DVWindow::updateQmlSize);

    /* Update window title whenever file changes. */
    connect(folderListing, &DVFolderListing::currentFileChanged, [this](){setTitle(folderListing->currentFile() + " - DepthView");});

    connect(this, &DVWindow::frameSwapped, this, &DVWindow::onFrameSwapped);

    /* We render a cursor inside QML so it is shown for both eyes. */
    setCursor(Qt::BlankCursor);

    setMinimumSize(QSize(1000, 500));
    setGeometry(0, 0, 1000, 600);
}

DVWindow::~DVWindow() {
    unloadPlugins();

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
        getPluginSize();

    /* Don't recreate fbo unless it's null or its size is wrong. */
    if(renderFBO == nullptr || renderFBO->size() != qmlSize)
        createFBO();

    qmlRoot->setSize(qmlSize);

    qmlWindow->setGeometry(QRect(QPoint(), qmlSize));
}

/* Most events need only be passed on to the qmlWindow. */
bool DVWindow::event(QEvent* e) {
    switch (e->type()) {
    case QEvent::Leave:
        /* TODO - This still doesn't always work right, but it's better than using setMouseGrabEnabled()... */
        if (holdMouse) {
            QPoint pos = mapFromGlobal(QCursor::pos());

            /* Generate a new coordinate on screen. */
            pos.setX(qBound(1, pos.x(), width()-1));
            pos.setY(qBound(1, pos.y(), height()-1));

            /* Will generate a new event. */
            QCursor::setPos(mapToGlobal(pos));
        }
        break;
    case QEvent::MouseMove:
        /* We also emit a special signal for this one so that the fake cursor
         * can be set to the right position without having a MouseArea that absorbs events. */
        emit qmlCommunication->mouseMoved(static_cast<QMouseEvent*>(e)->localPos());
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::Wheel:
        QCoreApplication::sendEvent(qmlWindow, e);

        setCursor(Qt::BlankCursor);
        return true;
    case QEvent::TouchBegin:
    case QEvent::TouchEnd:
    case QEvent::TouchUpdate:
    case QEvent::TouchCancel:
        /* TODO - Remap touch location into the modified screen coordinates,
         * in particular for Side by Side & Top/Bottom modes. */
        QCoreApplication::sendEvent(qmlWindow, e);

        emit qmlCommunication->touchEvent();
        return true;
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
        QCoreApplication::sendEvent(qmlWindow, e);
        return true;
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
        /* Don't relay the event to QOpenGLWindow. */
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

    return QOpenGLWindow::event(e);
}

void DVWindow::doCommandLine(QCommandLineParser& parser) {
    /* We use one string to hold all warning messages, so we only have to show one dialog. */
    QString warning;

    if(parser.isSet("f"))
        setWindowState(Qt::WindowFullScreen);

    if(parser.isSet("d") && !folderListing->initDir(parser.value("d")))
        warning += tr("<p>Invalid directory \"%1\" passed to \"--startdir\" argument!</p>").arg(parser.value("d"));

    if(parser.isSet("r")){
        const QString& renderer = parser.value("r");

        int mode = qmlCommunication->getModes().indexOf(renderer);

        if(mode == -1)
            warning += tr("<p>Invalid renderer \"%1\" passed to \"--renderer\" argument!</p>").arg(renderer);

        if (mode >= DVDrawMode::Plugin) {
            qmlCommunication->setPluginMode(renderer);
            qmlCommunication->initDrawMode(DVDrawMode::Plugin);
        } else {
            qmlCommunication->initDrawMode(DVDrawMode::Type(mode));
        }
    }

    for (const QString& arg : parser.positionalArguments()) {
        QFileInfo file(arg);

        /* The file extension is checked by openFile(). */
        if (file.exists() && folderListing->openFile(file))
            break;
    }

    /* If there weren't any warnings we don't show the dialog. */
    if(!warning.isEmpty())
        QMessageBox::warning(nullptr, tr("Invalid Command Line!"), warning);
}
