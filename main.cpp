#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Simulador de Procesos");
    app.setOrganizationName("Curso de Sistemas Operativos");

    MainWindow w;
    w.showMaximized();
    return app.exec();
}
