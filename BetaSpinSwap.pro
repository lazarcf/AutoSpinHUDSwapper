#-------------------------------------------------
#
# Project created by QtCreator 2016-06-01T21:16:39
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = BetaSpinSwap
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    winhacks.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

LIBS += -luser32
LIBS += -lkernel32
LIBS += -lpsapi

RESOURCES += \
    resources.qrc
