#include "version.hpp"
#include "dvwindow.hpp"
#include "dvqmlcommunication.hpp"
#include "dvinputplugin.hpp"
#include "dvrenderplugin.hpp"
#include "dvfolderlisting.hpp"
#include "dvqmlcommunication.hpp"
#include <QQuickItem>
#include <QMetaObject>
#include <QApplication>
#include <QOpenGLContext>
#include <QPluginLoader>
#include <QDir>

void DVWindow::loadPlugins() {
    /* Start with the path the application is in. */
    QDir pluginsDir(qApp->applicationDirPath());

#if defined(Q_OS_WIN)
    /* If we're in a "debug" or "release" folder go up a level, because that's where plugins are copied by the build system. */
    if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release")
        pluginsDir.cdUp();
#elif defined(Q_OS_MAC)
    /* I haven't actually tried this on Mac. This is just what the Qt plugin example said to do... */
    if (pluginsDir.dirName() == "MacOS") {
        pluginsDir.cdUp();
        pluginsDir.cdUp();
        pluginsDir.cdUp();
    }
#endif

    /* Go into the "plugins" folder from there. */
    pluginsDir.cd("plugins");

#if defined(Q_OS_WIN)
    /* On Windows we can add the plugin dir to the DLL search path dynamically.
     * On Linux LD_LIBRARY_PATH must be set before running the program to get the same effect. */
    SetDllDirectory(pluginsDir.absolutePath().toStdWString().c_str());
#endif

    qDebug("Loading plugins from \"%s\"...", qPrintable(pluginsDir.absolutePath()));

    /* Try to load all files in the directory. */
    for (const QString& filename : pluginsDir.entryList(QDir::Files)) {
        /* If the file isn't a valid library for this platform, don't bother. */
        if (!QLibrary::isLibrary(filename)) continue;

        QPluginLoader loader(pluginsDir.absoluteFilePath(filename));
        QObject *obj = loader.instance();

        if (obj == nullptr) {
            qDebug("\"%s\" is not a plugin. %s", qPrintable(filename), qPrintable(loader.errorString()));
            continue;
        }

        /* If it can be cast to the plugin type it is a valid plugin, otherwise it will be null. */
        DVRenderPlugin* renderPlugin = qobject_cast<DVRenderPlugin*>(obj);
        DVInputPlugin* inputPlugin = qobject_cast<DVInputPlugin*>(obj);

        if (renderPlugin != nullptr) {
            qDebug("Found render plugin: \"%s\"", qPrintable(filename));

            if (renderPlugin->init(context()->extraFunctions(), qmlEngine)) {
                for (const QString& mode : renderPlugin->drawModeNames())
                    qmlCommunication->addPluginMode(mode, renderPlugin->getConfigMenuObject());

                renderPlugins.append(renderPlugin);

                qDebug("Loaded plugin: \"%s\"", qPrintable(filename));
            } else {
                qDebug("Plugin: \"%s\" failed to init.", qPrintable(filename));
            }
        } else if (inputPlugin != nullptr) {
            qDebug("Found input plugin: \"%s\"", qPrintable(filename));

            if (inputPlugin->init(qmlEngine)) {
                inputPlugins.append(inputPlugin);

                qDebug("Loaded plugin: \"%s\"", qPrintable(filename));
            } else {
                qDebug("Plugin: \"%s\" failed to init.", qPrintable(filename));
            }
        }
    }
    qDebug("Done loading plugins.");
}

void DVWindow::unloadPlugins() {
    /* Deinit any/all loaded plugins. */
    for (DVRenderPlugin* plugin : renderPlugins)
        plugin->deinit();
    for (DVInputPlugin* plugin : inputPlugins)
        plugin->deinit();

    /* Clear the list. Not that it should be used anymore... */
    renderPlugins.clear();
    inputPlugins.clear();
}

DVRenderPlugin* DVWindow::getCurrentRenderPlugin() {
    for (DVRenderPlugin* plugin : renderPlugins)
        /* Find the first plugin that contains the mode we want. */
        if (plugin->drawModeNames().contains(qmlCommunication->pluginMode()))
            return plugin;

    return nullptr;
}

bool DVWindow::doPluginRender() {
    DVRenderPlugin* plugin = getCurrentRenderPlugin();

    /* Return false if it wasn't found, otherwise let it do its thing. */
    return plugin != nullptr && plugin->render(qmlCommunication->pluginMode(), context()->extraFunctions());
}

void DVWindow::getPluginSize() {
    DVRenderPlugin* plugin = getCurrentRenderPlugin();

    if (plugin != nullptr)
        qmlSize = plugin->getRenderSize(qmlSize);
}

void DVWindow::pluginOnFrameSwapped() {
    DVRenderPlugin* plugin = getCurrentRenderPlugin();

    if (plugin != nullptr) {
        /* Let it do its thing. */
        plugin->frameSwapped(context()->extraFunctions());

        /* Do we hold the mouse? */
        holdMouse = plugin->shouldLockMouse();
    }
}

void DVWindow::doPluginInput() {
    /* Get input from ALL input plugins. */
    /* TODO - Maybe make a settings dialog where they can be disabled? */
    for (DVInputPlugin* plugin : inputPlugins)
        plugin->pollInput(this);

    /* Only get input from the current render plugin. */
    DVRenderPlugin* plugin = getCurrentRenderPlugin();

    if (plugin != nullptr)
        plugin->pollInput(this);
}

DVInputMode::Type DVWindow::inputMode() const {
    return qmlCommunication->fileBrowserOpen() ? DVInputMode::FileBrowser : folderListing->isCurrentFileVideo() ? DVInputMode::VideoPlayer : DVInputMode::ImageViewer;
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
    qmlCommunication->setFileBrowserOpen(true);
}

void DVWindow::goBack() {
    if (qmlCommunication->fileBrowserOpen() && folderListing->canGoBack())
        folderListing->goBack();
}

void DVWindow::goForward() {
    if (qmlCommunication->fileBrowserOpen() && folderListing->canGoForward())
        folderListing->goForward();
}

void DVWindow::goUp() {
    if (qmlCommunication->fileBrowserOpen() && folderListing->canGoUp())
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

//void DVWindow::setMousePosition(QPoint pos, bool relative = false) {
//}

//void DVWindow::sendMouseClick(Qt::MouseButton button) {
//}

void DVWindow::playVideo() {
    emit qmlCommunication->playVideo();
}

void DVWindow::pauseVideo() {
    emit qmlCommunication->pauseVideo();
}

void DVWindow::playPauseVideo() {
    emit qmlCommunication->playPauseVideo();
}

void DVWindow::seekBack() {
    emit qmlCommunication->seekBack();
}

void DVWindow::seekForward() {
    emit qmlCommunication->seekForward();
}

void DVWindow::seekAmount(int msec) {
    emit qmlCommunication->seekAmount(msec);
}
