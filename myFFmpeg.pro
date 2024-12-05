#-------------------------------------------------
#
# Project created by QtCreator 2024-11-12T09:23:12
#
#-------------------------------------------------
win32: {
    FFMPEG_HOME=D:\dependentFile\ffmpeg-6.1.1-full_build-shared
    SDL_HOME = D:\dependentFile\SDL
    #设置 ffmpeg 的头文件
    INCLUDEPATH += $$FFMPEG_HOME/include \
                                        $$SDL_HOME/include

    #设置导入库的目录一边程序可以找到导入库
    # -L ：指定导入库的目录
    # -l ：指定要导入的 库名称
    LIBS +=  -L$$FFMPEG_HOME/lib \
             -lavcodec \
             -lavdevice \
             -lavfilter \
            -lavformat \
            -lavutil \
            -lpostproc \
            -lswresample \
            -lswscale \

    # 添加 SDL2 和 SDL2main 库
    LIBS += -L$$SDL_HOME/lib/x64 \
            -lSDL2
}

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = FFmpegTest3
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11



SOURCES += \
        audioplay.cpp \
        avpacketqueue.cpp \
        decoder.cpp \
        main.cpp \
        mainwindow.cpp

HEADERS += \
        audioplay.h \
        avpacketqueue.h \
        decoder.h \
        mainwindow.h

FORMS += \
        mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resouce.qrc


