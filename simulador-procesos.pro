QT += core gui widgets

CONFIG += c++17

TARGET = simulador-procesos
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    statediagramwidget.cpp \
    pcbcardwidget.cpp \
    cpustripwidget.cpp \
    ganttwidget.cpp \
    memorybarwidget.cpp \
    usagegraphwidget.cpp \
    taskmanagerwidget.cpp

HEADERS += \
    mainwindow.h \
    theme.h \
    statediagramwidget.h \
    pcbcardwidget.h \
    cpustripwidget.h \
    ganttwidget.h \
    memorybarwidget.h \
    usagegraphwidget.h \
    taskmanagerwidget.h \
    process.h \
    memorymodel.h
