import qbs

DynamicLibrary {
    Depends { name: "cpp" }
    Depends { name: "Qt"; submodules: ["qml", "quick"] }

    cpp.includePaths: ["depthview2/include/"]

    Group {
        fileTagsFilter: "dynamiclibrary"
        qbs.install: true
        qbs.installDir: "bin/plugins"
    }
}
