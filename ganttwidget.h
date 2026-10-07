#ifndef GANTTWIDGET_H
#define GANTTWIDGET_H

#include <QWidget>
#include <QVector>
#include "process.h"

// ============================================================
//  Linea de tiempo (diagrama de Gantt) de los estados
// ------------------------------------------------------------
//  Una fila por proceso, una celda por tick. El color de la celda
//  es el color del estado en que estaba el proceso en ese tick, asi
//  que el historial de transiciones del PCB se vuelve visible: se
//  leen de un golpe las esperas, las rafagas y los bloqueos por E/S.
// ============================================================
class GanttWidget : public QWidget {
    Q_OBJECT
public:
    explicit GanttWidget(QWidget *parent = nullptr);

    void actualizar(const QVector<Proceso> &procesos, int reloj);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<Proceso> m_procesos;
    int m_reloj = 0;

    static constexpr int AnchoEtiqueta = 108;  // columna de nombres
    static constexpr int AltoFila      = 15;
    static constexpr int EspacioFila   = 3;
    static constexpr int AltoEjeTicks  = 13;
    static constexpr int AnchoCeldaMin = 4;
    static constexpr int AnchoCeldaMax = 16;
};

#endif // GANTTWIDGET_H
