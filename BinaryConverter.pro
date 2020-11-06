QT       += core gui
QT       += printsupport


greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    imageviewer.cpp \
    main.cpp

HEADERS += \
    imageviewer.h

FORMS += \
#    imageviewer.ui \
    imageviewer1366.ui \
    imageviewer1680.ui

QMAKE_CXXFLAGS += -std=c++17 \
                  -std=c++1z

unix:mac{
    QMAKE_CXXFLAGS += -std=c++17

    INCLUDEPATH += /Users/yuki/source/opencv-4.4.0/include
    LIBS += -L/Users/yuki/source/opencv/lib \
    -lopencv_world
}

win32{
    QMAKE_CXXFLAGS += std:c++17

    INCLUDEPATH += C:/opencv/release/install/include
    Debug:{
    LIBS += -lc:/opencv/release/install/x64/vc16/lib/opencv_world440d
    }
    Release:{
    LIBS += -lc:/opencv/release/install/x64/vc16/lib/opencv_world440
    }
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
