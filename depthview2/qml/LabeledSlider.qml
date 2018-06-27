import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

RowLayout {
    property alias text: label.text
    property alias from: slider.from
    property alias to: slider.to
    property alias value: slider.value

    width: parent.width

    Label {
        id: label
    }
    Slider {
        id: slider

        Layout.fillWidth: true
    }
}
