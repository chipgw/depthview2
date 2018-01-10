import qbs 1.0

Module {
    property path openVRDir: "../openvr"
    Depends { name: "cpp" }
    cpp.includePaths: [openVRDir + "/headers"]
    Properties {
        condition: qbs.targetOS.contains("windows") && qbs.architecture === "x86"
        cpp.dynamicLibraries: [openVRDir + "/lib/win32/openvr_api.lib"]
    }
    Properties {
        condition: qbs.targetOS.contains("windows") && qbs.architecture === "x86_64"
        cpp.dynamicLibraries: [openVRDir + "/lib/win64/openvr_api.lib"]
    }
    /* TODO - Linux & macOS. */
}
