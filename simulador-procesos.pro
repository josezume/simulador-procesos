QT += core gui widgets

CONFIG += c++17

# MinGW (Windows) trae su propio <process.h>, que los encabezados estándar
# (<memory>, <mutex>, <thread>) incluyen por dentro. Si la carpeta del proyecto
# está en la ruta de búsqueda, el compilador toma NUESTRO process.h en su lugar
# y aparecen cientos de errores dentro de los encabezados de Qt. Con esta opción
# la carpeta del proyecto no se agrega con -I; nuestros #include "..." siguen
# funcionando porque están todos en la misma carpeta.
CONFIG += no_include_pwd

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
