#pragma once

#include <QObject>
#include <QSettings>
#include <QMetaEnum>

class QWindow;

/* Macro to easily define QML friendly enums.
 * First argument is enum name, followed by enum values. */
#define DV_ENUM(Name, ...) \
class Name { \
    Q_GADGET \
    /* No instances of this class. */ \
    Name() = delete; \
    ~Name() = delete; \
public: \
    enum Type { \
        __VA_ARGS__ \
    }; \
    /* So that the enum is usable in QML. */ \
    Q_ENUM(Type) \
    \
    static QMetaEnum metaEnum() { return QMetaEnum::fromType<Type>(); } \
    \
    static Type fromString(const char* str) { return Type(metaEnum().keyToValue(str)); } \
    \
    static const char* toString(Type val) { return metaEnum().valueToKey(val); } \
} /* Put the ';' after declarations cuz the Qt Creator parser likes it better for some reason... */

DV_ENUM(DVDrawMode,
        Anaglyph,
        SidebySide,
        TopBottom,
        InterlacedV,
        InterlacedH,
        Checkerboard,
        MonoLeft,
        MonoRight,
        Plugin);

DV_ENUM(DVSourceMode,
        SidebySide,
        SidebySideAnamorphic,
        TopBottom,
        TopBottomAnamorphic,
        Mono);

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

    QString pluginMode();
    void setPluginMode(QString mode);

    void addPluginModes(const QStringList& modes);
    Q_INVOKABLE QStringList getPluginModes();

    Q_INVOKABLE void addBookmark(QString bookmark);
    Q_INVOKABLE void deleteBookmark(QString bookmark);

    QStringList bookmarks();

    Q_INVOKABLE QStringList getStorageDevicePaths();

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
};
