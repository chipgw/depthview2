DepthView
=========
is a viewer for stereoscopic 3D .jps and .pns images and also some formats of 3D video. I run it on Windows, Linux, and Android.

This is a WIP complete rewrite using QML and OpenGL. The original project is located here: [DepthView]

Supported Draw Modes
--------------------
* Red/Cyan Anglaph
* Side by Side (with mirror)
* Top/Bottom (also with mirror)
* Interlaced (horizontal and vertical)
* Checkerboard
* Single Image (Mono)

Dependencies
------------
* [Qt] 5.7 or newer.
* [QtAV]
* [OpenVR] \(Optional for VR support\)

Building
--------
The simplest way to build is to open `depthview2.pro` in Qt Creator. Configuration options can then be added in the Projects->Build Steps->qmake->Additional Arguments.

For information on running qmake directly, see the [qmake docs].

Configuration options:
* "CONFIG+=portable" - Create a build that will store settings in the exe directory rather than in the OS defined user settings folder.
* "PLUGINS=<plugins>" - Build with the specified plugins. (Supported plugin names are in parenthesis below.)

Supported plugins:
* Test plugin (testplugin) - A minimal plugin automatically enabled in debug builds for testing purposes.
* OpenVR plugin (openvrplugin) - A plugin to add support for rendering to a virtual screen in VR. (Looks for OpenVR development files in ../openvr relative to the project root.)

License
-------
[MIT License]

[DepthView]:https://github.com/chipgw/depthview
[Qt]:http://www.qt.io
[QtAV]:http://www.qtav.org/
[OpenVR]:https://github.com/ValveSoftware/openvr
[qmake docs]:http://doc.qt.io/qt-5/qmake-running.html
[MIT License]:LICENSE
