#pragma once

#include <QUrl>
#include "dvenums.hpp"

class QWindow;
class QSettings;
class QQuickItem;

class DVQmlCommunication : public QObject {
    Q_OBJECT

    /* Can be read, written, and notifies when changed. */
    Q_PROPERTY(DVDrawMode::Type drawMode MEMBER m_drawMode READ drawMode WRITE setDrawMode NOTIFY drawModeChanged)
    Q_PROPERTY(bool anamorphicDualView MEMBER m_anamorphicDualView READ anamorphicDualView WRITE setAnamorphicDualView NOTIFY anamorphicDualViewChanged)

    Q_PROPERTY(bool mirrorLeft READ mirrorLeft WRITE setMirrorLeft NOTIFY mirrorLeftChanged)
    Q_PROPERTY(bool mirrorRight READ mirrorRight WRITE setMirrorRight NOTIFY mirrorRightChanged)

    Q_PROPERTY(qreal greyFac READ greyFac WRITE setGreyFac NOTIFY greyFacChanged)

    Q_PROPERTY(bool fullscreen READ fullscreen WRITE setFullscreen NOTIFY fullscreenChanged)

    Q_PROPERTY(QString pluginMode READ pluginMode WRITE setPluginMode NOTIFY pluginModeChanged)
    Q_PROPERTY(QStringList pluginModes READ getPluginModes NOTIFY pluginModesChanged)
    Q_PROPERTY(QStringList modes READ getModes NOTIFY pluginModesChanged)

    Q_PROPERTY(QQuickItem* pluginConfigMenu READ getPluginConfigMenu NOTIFY pluginModeChanged)

    Q_PROPERTY(bool swapEyes READ swapEyes WRITE setSwapEyes NOTIFY swapEyesChanged)

    Q_PROPERTY(bool fileBrowserOpen READ fileBrowserOpen WRITE setFileBrowserOpen NOTIFY fileBrowserOpenChanged)

public:
    /* Settings can be set from DVWindow. */
    QSettings& settings;

    explicit DVQmlCommunication(QWindow* parent, QSettings& s);

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

    void addPluginMode(const QString& mode, QQuickItem* config);
    QStringList getPluginModes() const;
    QStringList getModes() const;
    QQuickItem* getPluginConfigMenu() const;

    bool swapEyes() const;
    void setSwapEyes(bool swap);

    Q_INVOKABLE QString versionString();
    Q_INVOKABLE QString buildType();
    Q_INVOKABLE QString buildCompiler();

    bool fileBrowserOpen() const;
    void setFileBrowserOpen(bool open);

signals:
    void isLeftChanged(bool isLeft);
    void drawModeChanged(DVDrawMode::Type mode);
    void anamorphicDualViewChanged(bool anamorphicDualView);

    void mirrorLeftChanged(bool mirror);
    void mirrorRightChanged(bool mirror);

    void greyFacChanged(qreal fac);

    void fullscreenChanged(bool fullscreen);

    void pluginModeChanged(QString mode);
    void pluginModesChanged();

    void swapEyesChanged();

    void fileBrowserOpenChanged();

    /* Used for setting the cursor position. */
    void mouseMoved(const QPointF& pos);

    /* Used to show/hide ui based on touchscreen input. */
    void touchEvent();

public slots:
    void ownerWindowStateChanged(Qt::WindowState windowState);

private:
    bool m_mirrorLeft;
    bool m_mirrorRight;

    qreal m_greyFac;

    DVDrawMode::Type m_drawMode;
    bool m_anamorphicDualView;

    QWindow* owner;

    QString m_pluginMode;
    QMap<QString, QQuickItem*> pluginModes;

    bool m_swapEyes;

    bool m_fileBrowserOpen;
};
