#pragma once

#include <QTimer>
#include <QSettings>
#include <QUrl>
#include "dvenums.hpp"

class QWindow;

class DVQmlCommunication : public QObject {
    Q_OBJECT

    /* Can only be read. */
    Q_PROPERTY(bool isLeft READ isLeft NOTIFY isLeftChanged)

    /* Can be read, written, and notifies when changed. */
    Q_PROPERTY(DVDrawMode::Type drawMode MEMBER m_drawMode READ drawMode WRITE setDrawMode NOTIFY drawModeChanged)
    Q_PROPERTY(bool anamorphicDualView MEMBER m_anamorphicDualView READ anamorphicDualView WRITE setAnamorphicDualView NOTIFY anamorphicDualViewChanged)

    Q_PROPERTY(bool mirrorLeft READ mirrorLeft WRITE setMirrorLeft NOTIFY mirrorLeftChanged)
    Q_PROPERTY(bool mirrorRight READ mirrorRight WRITE setMirrorRight NOTIFY mirrorRightChanged)

    Q_PROPERTY(qreal greyFac READ greyFac WRITE setGreyFac NOTIFY greyFacChanged)

    Q_PROPERTY(bool fullscreen READ fullscreen WRITE setFullscreen NOTIFY fullscreenChanged)

    Q_PROPERTY(QString pluginMode READ pluginMode WRITE setPluginMode NOTIFY pluginModeChanged)

    /* There is no WRITE function because you add or remove via addBookmark() and deleteBookmark(). */
    Q_PROPERTY(QStringList bookmarks READ bookmarks NOTIFY bookmarksChanged)

    /* By making this a property we can emit a signal when the list needs to be updated. */
    Q_PROPERTY(QStringList storageDevicePaths READ getStorageDevicePaths NOTIFY storageDevicePathsChanged)


    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY historyChanged)
    Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY historyChanged)

public:
    /* Settings can be set from DVWindow. */
    QSettings settings;

    explicit DVQmlCommunication(QWindow* parent);

    /* Where QML reads the value of the current eye. */
    bool isLeft() const;

    /* Set the current eye. */
    void leftImage() { isLeftChanged(m_isLeft = true);  }
    void rightImage() { isLeftChanged(m_isLeft = false); }

    /* The current draw mode. */
    DVDrawMode::Type drawMode() const;
    void setDrawMode(DVDrawMode::Type mode);

    bool anamorphicDualView() const;
    void setAnamorphicDualView(bool anamorphic);

    bool mirrorLeft() const;
    void setMirrorLeft(bool mirror);

    bool mirrorRight() const;
    void setMirrorRight(bool mirror);

    qreal greyFac() const;
    void setGreyFac(qreal fac);

    bool fullscreen() const;
    void setFullscreen(bool fullscreen);

    QString pluginMode() const;
    void setPluginMode(QString mode);

    void addPluginModes(const QStringList& modes);
    Q_INVOKABLE QStringList getPluginModes() const;

    Q_INVOKABLE void addBookmark(QString bookmark);
    Q_INVOKABLE void deleteBookmark(QString bookmark);

    QStringList bookmarks() const;

    QStringList getStorageDevicePaths() const;

    Q_INVOKABLE bool fileExists(QString file) const;
    Q_INVOKABLE bool dirExists(QString dir) const;

    Q_INVOKABLE QUrl encodeURL(QString url) const;
    Q_INVOKABLE QString decodeURL(QUrl url) const;

    Q_INVOKABLE QString goBack();
    Q_INVOKABLE QString goForward();
    Q_INVOKABLE void pushHistory(QString value);
    bool canGoBack() const;
    bool canGoForward() const;

signals:
    void isLeftChanged(bool isLeft);
    void drawModeChanged(DVDrawMode::Type mode);
    void anamorphicDualViewChanged(bool anamorphicDualView);

    void mirrorLeftChanged(bool mirror);
    void mirrorRightChanged(bool mirror);

    void greyFacChanged(qreal fac);

    void fullscreenChanged(bool fullscreen);

    void pluginModeChanged(QString mode);

    void bookmarksChanged(QStringList bookmarks);

    void storageDevicePathsChanged();

    void historyChanged();

    /* Used for setting the cursor position. */
    void mouseMoved(const QPointF& pos);

    /* Used to show/hide ui based on touchscreen input. */
    /* TODO - Mabe some args would be useful? */
    void touchEvent();

public slots:
    void ownerWindowStateChanged(Qt::WindowState windowState);

private:
    bool m_mirrorLeft;
    bool m_mirrorRight;

    qreal m_greyFac;

    bool m_isLeft;
    DVDrawMode::Type m_drawMode;
    bool m_anamorphicDualView;

    QWindow* owner;

    QString m_pluginMode;
    QStringList pluginModes;

    QStringList m_bookmarks;

    QStringList browserHistory;
    int currentHistory;

    QTimer driveTimer;
};
