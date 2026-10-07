#ifndef PCBCARDWIDGET_H
#define PCBCARDWIDGET_H

#include <QWidget>
#include "process.h"

class QHBoxLayout;

// Tarjeta compacta de un PCB dentro de una cola.
// El borde izquierdo lleva el color del estado: el color codifica estado
// y nada mas. Al hacer clic (fuera de los botones) se emite verDetalle().
class PcbCardWidget : public QWidget {
    Q_OBJECT
public:
    explicit PcbCardWidget(const Proceso &proc, QWidget *parent = nullptr);

signals:
    void verDetalle(int pid);
    void accionAdmitir(int pid);
    void accionDespachar(int pid);
    void accionBloquear(int pid);
    void accionDespertar(int pid);
    void accionExpropiar(int pid);
    void accionTerminar(int pid);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    int m_pid;
    void construirAcciones(QHBoxLayout *layout, ProcState estado);
};

#endif // PCBCARDWIDGET_H
