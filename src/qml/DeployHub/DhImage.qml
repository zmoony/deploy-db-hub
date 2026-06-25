import QtQuick

Image {
    id: root

    property int maxSourceWidth: 0
    property int maxSourceHeight: 0

    asynchronous: true
    cache: true
    mipmap: true
    smooth: true
    antialiasing: true

    sourceSize.width: maxSourceWidth > 0 ? maxSourceWidth : -1
    sourceSize.height: maxSourceHeight > 0 ? maxSourceHeight : -1
}
