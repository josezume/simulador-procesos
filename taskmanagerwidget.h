#ifndef TASKMANAGERWIDGET_H
#define TASKMANAGERWIDGET_H

#include <QWidget>
#include <QVector>
#include "process.h"

class QLabel;
class QTableWidget;
class QPushButton;
class UsageGraphWidget;
class MemoryBarWidget;

// Panel de rendimiento: metricas globales, graficas de CPU/memoria en vivo,
// mapa de la memoria fisica por segmentos y tabla de procesos con las
// metricas clasicas de planificacion (espera, retorno, respuesta, E/S...).
class TaskManagerWidget : public QWidget {
    Q_OBJECT
public:
    explicit TaskManagerWidget(QWidget *parent = nullptr);

    // Refresca metricas y tabla (tras cualquier cambio de estado).
    void actualizar(const QVector<Proceso> &procesos, int reloj, double cpuGlobal);

    // Anade un punto a las graficas (una vez por tick de reloj).
    void muestrear(const QVector<Proceso> &procesos, double cpuGlobal);

    void limpiar();

signals:
    void verDetalle(int pid);          // doble clic sobre una fila
    void solicitudTerminar(int pid);   // boton "Finalizar proceso"

private:
    UsageGraphWidget *m_graficaCpu;
    UsageGraphWidget *m_graficaMem;
    MemoryBarWidget *m_barraMemoria;
    QTableWidget *m_tabla;
    QPushButton *m_btnFinalizar;

    QLabel *m_valCpu,  *m_subCpu;
    QLabel *m_valMem,  *m_subMem;
    QLabel *m_valProc, *m_subProc;
    QLabel *m_valEspera, *m_subEspera;
    QLabel *m_barraEstado;

    int pidSeleccionado() const;
    QWidget* crearMetrica(const QString &titulo, QLabel *&valorSalida, QLabel *&subSalida);
};

#endif // TASKMANAGERWIDGET_H
