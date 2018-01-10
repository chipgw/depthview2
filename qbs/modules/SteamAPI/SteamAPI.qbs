Module {
//    win32: {
//        contains(QMAKE_TARGET.arch, x86_64): LIBS += -L$$PWD/../../../steamsdk/
//        else: LIBS += -L$$PWD/../../../steamsdk/
//    }else{
//        LIBS += -L$$PWD/../../../steamsdk/redistributable_bin/linux64/ -L$$PWD/../../../steamsdk/redistributable_bin/linux32/ -lsteam_api
//    }
    property path steamSDKDir: "../steamsdk"
    Depends { name: "cpp" }
    cpp.includePaths: [steamSDKDir + "/public/steam"]
    Properties {
        condition: qbs.targetOS.contains("windows") && qbs.architecture === "x86"
        cpp.dynamicLibraries: [steamSDKDir + "/redistributable_bin/steam_api.lib"]
    }
    Properties {
        condition: qbs.targetOS.contains("windows") && qbs.architecture === "x86_64"
        cpp.dynamicLibraries: [steamSDKDir + "/redistributable_bin/win64/steam_api64.lib"]
    }
    /* TODO - Linux & macOS. */
}
