import qbs 1.0
import "qbs/imports/DVPlugin.qbs" as DVPlugin

Project {
    qbsSearchPaths: "qbs"

    QtApplication {
        name: "DepthView"
        targetName: "depthview2"

        cpp.includePaths: ["depthview2/include/"]
        files: [
            "depthview2/src/main.cpp",
            "depthview2/src/dvqmlcommunication.cpp",
            "depthview2/src/version.cpp",
            "depthview2/src/dvfolderlisting.cpp",
            "depthview2/src/dvthumbnailprovider.cpp",
            "depthview2/src/dvpluginmanager.cpp",
            "depthview2/src/dvfilevalidator.cpp",
            "depthview2/src/dvvirtualscreenmanager.cpp",
            "depthview2/src/dvwindowhook.cpp",
            "depthview2/src/dvrenderer.cpp",
            "depthview2/include/dvenums.hpp",
            "depthview2/include/dvqmlcommunication.hpp",
            "depthview2/include/dvinputplugin.hpp",
            "depthview2/include/version.hpp",
            "depthview2/include/dvfolderlisting.hpp",
            "depthview2/include/dvinputinterface.hpp",
            "depthview2/include/dvthumbnailprovider.hpp",
            "depthview2/include/dvpluginmanager.hpp",
            "depthview2/include/dvfilevalidator.hpp",
            "depthview2/include/dvconfig.hpp",
            "depthview2/include/dvvirtualscreenmanager.hpp",
            "depthview2/include/dv_vrdriver.hpp",
            "depthview2/include/dvwindowhook.hpp",
            "depthview2/include/dvrenderer.hpp",
            "depthview2/qml.qrc",
            "depthview2/depthview2.rc"
        ]
        Depends { name: "cpp" }
        Depends { name: "Qt"; submodules: ["qml", "quick", "widgets", "sql", "av", "quickcontrols2"] }
        cpp.dynamicLibraries: [ (qbs.buildVariant == "debug") ? "QtAVd1.lib" : "QtAV1.lib"]

        /* TODO - Get git version. */
        cpp.defines: ["GIT_VERSION=\"\""]

        Depends {
            id: openVR
            name: "OpenVR"
            required: false
        }
        Properties {
            condition: openVR.present

            files: outer.concat(["depthview2/src/dv_vrdriver_openvr.cpp",
                                 "depthview2/include/dv_vrdriver_openvr.hpp"])

            cpp.defines: outer.concat("DV_OPENVR")
        }

        Group {
            fileTagsFilter: "application"
            qbs.install: true
            qbs.installDir: "bin"
        }
    }

    DVPlugin {
        name: "Steam Controller Plugin"
        targetName: "dv2_steamcontrollerplugin"

        Depends { name: "SteamAPI" }

        files: [
            "plugins/steamcontrollerplugin/steamcontrollerplugin.cpp",
            "plugins/steamcontrollerplugin/steamcontrollerplugin.hpp",
            "plugins/steamcontrollerplugin/steamcontrollerplugin.json"
        ]
    }

    DVPlugin {
        name: "Qt Gamepad Plugin"
        targetName: "dv2_gamepadplugin"

        Depends {
            name: "Qt"; submodules: ["gamepad"]
        }

        files: [
            "plugins/gamepadplugin/gamepadplugin.cpp",
            "plugins/gamepadplugin/gamepadplugin.hpp",
            "plugins/gamepadplugin/gamepadplugin.json"
        ]
    }
}
