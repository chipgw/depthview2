#pragma once

#include <QUrl>
#include <QPoint>
#include "dvenums.hpp"

#if defined(Q_OS_WIN32) && !defined(DV_PORTABLE)
#define DV_FILE_ASSOCIATION
#endif

class DVFolderListing;
class QWindow;
class QSettings;
class QSGTextureProvider;
class QQuickItem;

class DVQmlCommunication : public QObject {
    Q_OBJECT

    /* Can be read, written, and notifies when changed. */
    Q_PROPERTY(DVDrawMode::Type drawMode MEMBER m_drawMode READ drawMode WRITE setDrawMode NOTIFY drawModeChanged)
    Q_PROPERTY(bool anamorphicDualView MEMBER m_anamorphicDualView READ anamorphicDualView WRITE setAnamorphicDualView NOTIFY anamorphicDualViewChanged)

    Q_PROPERTY(bool mirrorLeft READ mirrorLeft WRITE setMirrorLeft NOTIFY mirrorLeftChanged)
    Q_PROPERTY(bool mirrorRight READ mirrorRight WRITE setMirrorRight NOTIFY mirrorRightChanged)

    Q_PROPERTY(qreal greyFacL READ greyFacL WRITE setGreyFacL NOTIFY greyFacLChanged)
    Q_PROPERTY(qreal greyFacR READ greyFacR WRITE setGreyFacR NOTIFY greyFacRChanged)

    Q_PROPERTY(bool fullscreen READ fullscreen WRITE setFullscreen NOTIFY fullscreenChanged)

    Q_PROPERTY(bool swapEyes READ swapEyes WRITE setSwapEyes NOTIFY swapEyesChanged)

    Q_PROPERTY(bool saveWindowState READ saveWindowState WRITE setSaveWindowState NOTIFY saveWindowStateChanged)
    Q_PROPERTY(bool startupFileBrowser READ startupFileBrowser WRITE setStartupFileBrowser NOTIFY startupFileBrowserChanged)

    Q_PROPERTY(QString uiTheme READ uiTheme WRITE setUiTheme NOTIFY uiThemeChanged)
    Q_PROPERTY(QStringList uiThemes READ uiThemes CONSTANT)

    Q_PROPERTY(QQuickItem* openImageTarget READ openImageTarget WRITE setOpenImageTarget NOTIFY openImageTargetChanged)

    Q_PROPERTY(QPointF surroundPan READ surroundPan WRITE setSurroundPan NOTIFY surroundPanChanged)
    Q_PROPERTY(qreal surroundFOV READ surroundFOV WRITE setSurroundFOV NOTIFY surroundFOVChanged)

public:
    /* Settings can be set from DVWindow. */
    QSettings& settings;

    explicit DVQmlCommunication(QWindow* parent, QSettings& s);

    /* The current draw mode. */
    DVDrawMode::Type drawMode() const;
    void setDrawMode(DVDrawMode::Type mode);
    void initDrawMode(DVDrawMode::Type mode);

    bool anamorphicDualView() const;
    void setAnamorphicDualView(bool anamorphic);

    bool mirrorLeft() const;
    void setMirrorLeft(bool mirror);

    bool mirrorRight() const;
    void setMirrorRight(bool mirror);

    qreal greyFacL() const;
    void setGreyFacL(qreal fac);
    qreal greyFacR() const;
    void setGreyFacR(qreal fac);

    bool fullscreen() const;
    void setFullscreen(bool fullscreen);

    bool swapEyes() const;
    void setSwapEyes(bool swap);

    Q_INVOKABLE QString versionString();
    Q_INVOKABLE QString gitVersion();
    Q_INVOKABLE QString buildType();
    Q_INVOKABLE QString buildCompiler();

    bool saveWindowState() const;
    void setSaveWindowState(bool save);

    bool startupFileBrowser() const;
    void setStartupFileBrowser(bool open);

    QString uiTheme() const;
    QStringList uiThemes() const;
    void setUiTheme(QString theme);

    QQuickItem* openImageTarget();
    QSGTextureProvider* openImageTexture();
    void setOpenImageTarget(QQuickItem* target);

    QPointF surroundPan() const;
    void setSurroundPan(QPointF val);
    qreal surroundFOV() const;
    void setSurroundFOV(qreal val);

    DVFolderListing* folderListing;

#ifdef DV_FILE_ASSOCIATION
    Q_INVOKABLE static void registerFileTypes();
#endif

signals:
    void isLeftChanged(bool isLeft);
    void drawModeChanged();
    void anamorphicDualViewChanged(bool anamorphicDualView);

    void mirrorLeftChanged(bool mirror);
    void mirrorRightChanged(bool mirror);

    void greyFacLChanged(qreal fac);
    void greyFacRChanged(qreal fac);

    void fullscreenChanged(bool fullscreen);

    void swapEyesChanged();

    void openImageTargetChanged();

    void surroundPanChanged();
    void surroundFOVChanged();

    /* Settings. */
    void saveWindowStateChanged();
    void startupFileBrowserChanged();
    void uiThemeChanged();

    /* Used for setting the cursor position. */
    void mouseMoved(const QPointF& pos, bool synthesized);

    /* Used to show/hide ui based on touchscreen input. */
    void touchEvent();

    /* --------------------------------------------- *
     * Begin signals for DVInputInterface functions. *
     * (Only the ones that need to be used in QML.)  *
     * --------------------------------------------- */

    /* Navigation controls, used primarily in the file browser. */
    void left();
    void right();
    void up();
    void down();

    /* Accept the currently highlighted item. */
    void accept();

    /* Closes any and all popups, basically the same as pressing the escape key. */
    void cancel();

    /* Show the file info popup. */
    void fileInfo();

    void zoomActual();
    void zoomFit();

    /* ------------------------------------------- *
     * End signals for DVInputInterface functions. *
     * ------------------------------------------- */

public slots:
    void ownerWindowStateChanged(Qt::WindowState windowState);

private:
    bool m_mirrorLeft;
    bool m_mirrorRight;

    qreal m_greyFacL;
    qreal m_greyFacR;

    DVDrawMode::Type m_drawMode;
    bool m_anamorphicDualView;

    QWindow* owner;

    Qt::WindowState lastWindowState;

    bool m_swapEyes;

    QQuickItem* imageTarget;

    QPointF m_surroundPan;
    qreal m_surroundFOV;
};
