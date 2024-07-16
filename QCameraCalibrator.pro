QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    calibrationthread.cpp \
    camerathread.cpp \
    graphicsviewcontainer.cpp \
    main.cpp \
    mainwindow.cpp \
    markerthread.cpp \
    workspace.cpp \
    yamlhandler.cpp

HEADERS += \
    calibrationthread.h \
    camerathread.h \
    graphicsviewcontainer.h \
    mainwindow.h \
    markerthread.h \
    workspace.h \
    yamlhandler.h

FORMS += \
    mainwindow.ui

# Specify the OpenCV version you are using
OPENCV_VERSION = 490

# Specify the path to the OpenCV include directory
INCLUDEPATH += C:/lib/install/opencv/include

# Specify the path to the OpenCV library directory
LIBS += -LC:/lib/install/opencv/x64/vc17/lib

# Link the correct version of the opencv_world library depending on the build configuration
CONFIG(debug, debug|release) {
    # Debug configuration
    LIBS += -lopencv_world$$OPENCV_VERSION"d"
} else {
    # Release configuration
    LIBS += -lopencv_world$$OPENCV_VERSION
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
