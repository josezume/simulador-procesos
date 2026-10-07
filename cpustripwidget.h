#ifndef CPUSTRIPWIDGET_H
#define CPUSTRIPWIDGET_H

#include <QWidget>
#include <QVector>
#include "process.h"

class QLayout;
class QHBoxLayout;
class QVBoxLayout;
class QLabel;

// ============================================================
//  Franja de CPU + cola de listos
// ------------------------------------------------------------
//  La CPU no es una columna mas: es un recurso unico. Ocupa una
//  franja horizontal donde a la izquierda se ve quien la tiene y
//  a la derecha la cola de listos ordenada por prioridad, que es
//  exactamente lo que decide el planificador.
// ============================================================
class CpuStripWidget : public QWidget {
    Q_OBJECT
public:
    explicit CpuStripWidget(QWidget *parent = nullptr);

    void actualizar(const QVector<Proceso> &procesos);

signals:
    void accionDespachar(int pid);
    void accionBloquear(int pid);
    void accionExpropiar(int pid);
    void accionTerminar(int pid);
    void verDetalle(int pid);

private:
    QWidget *m_cajaCpu;
    QVBoxLayout *m_layCpu;
    QWidget *m_cajaCola;
    QHBoxLayout *m_layCola;
    QLabel *m_tituloCola;

    void pintarCpuVacia();
    void pintarCpu(const Proceso &p);
    static void vaciar(QLayout *layout);
};

#endif // CPUSTRIPWIDGET_H
