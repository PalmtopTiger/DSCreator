#-------------------------------------------------
#
# Project created by QtCreator 2014-01-29T18:46:53
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = DSCreator
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    script.cpp \
    table.cpp

HEADERS  += mainwindow.h \
    script.h \
    table.h

FORMS    += mainwindow.ui

RESOURCES += DSCreator.qrc

RC_FILE = DSCreator.rc

ICON = main.icns
