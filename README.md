DepthView
=========
is a viewer for stereoscopic 3D images and video. It runs on Windows and Linux, with experimental macOS and Android support.

This is a complete rewrite of [DepthView] using QML and OpenGL.

Supported Display Modes
-----------------------
* Red/Cyan Anglaph
* Side by Side (with mirror)
* Top/Bottom (also with mirror)
* Interlaced (horizontal and vertical)
* Checkerboard
* Single Image (Mono)
* Experimental VR plugin (using SteamVR/OpenVR)

Supported File Formats
----------------------
* Stereo 3D jps & pns files
* 2D or 3D jpg, png, & bmp images (3D can be top/bottom or side-by-side).
* 2D or 3D avi, mp4, m4v, mkv, ogv, ogg, webm, flv, 3gp, wmv, & mpg videos (3D can be top/bottom or side-by-side).

Dependencies
------------
* [Qt] 5.8 or newer.
* [QtAV]
* [OpenVR] v1.0.16 \(Optional for VR support\)

Building
--------
The simplest way to build is to open `depthview2.pro` in Qt Creator. Configuration options can then be added in the Projects->Build Steps->qmake->Additional Arguments.

For information on running qmake directly, see the [qmake docs].

Configuration options:
* `CONFIG+=portable` - Create a build that will store settings in the exe directory rather than in the OS defined user settings folder.
* `CONFIG+=openvr` - Adds support for rendering to a virtual screen in VR. (Looks for OpenVR development files in `../openvr` relative to the project root.)
* `PLUGINS=<plugins>` - Build with the specified plugins. (Supported plugin names are in parenthesis below.)

Supported plugins:
* Gamepad plugin (`gamepadplugin`) - Adds support for input from game controllers using the Qt Gamepad module.
* Steam Controller plugin (`steamcontrollerplugin`) - Adds support for input from the Steam Controller & other gamepads supported through the Steam API. (Looks for Steam SDK in `../steamsdk` relative to the project root.)

License
-------
[MIT License]

[DepthView]:https://github.com/chipgw/depthview
[Qt]:http://www.qt.io
[QtAV]:http://www.qtav.org/
[OpenVR]:https://github.com/ValveSoftware/openvr
[qmake docs]:http://doc.qt.io/qt-5/qmake-running.html
[MIT License]:LICENSE
