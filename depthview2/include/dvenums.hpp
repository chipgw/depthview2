#pragma once

#include <QMetaEnum>

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
};

DV_ENUM(DVDrawMode,
        Anaglyph,
        SidebySide,
        TopBottom,
        InterlacedV,
        InterlacedH,
        Checkerboard,
        Mono)

DV_ENUM(DVSourceMode,
        SidebySide,
        SidebySideAnamorphic,
        TopBottom,
        TopBottomAnamorphic,
        Mono)

DV_ENUM(DVInputMode,
        ImageViewer,
        VideoPlayer,
        FileBrowser)

DV_ENUM(DVPluginType,
        InvalidPlugin,
        InputPlugin)
