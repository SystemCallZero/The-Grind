QT += core widgets

CONFIG += c++17

TARGET = TheGrind
TEMPLATE = app

SOURCES += \
    main.cpp \
    grindmanager.cpp \
    mainwindow.cpp

HEADERS += \
    grindmanager.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

RC_ICONS = icon.ico