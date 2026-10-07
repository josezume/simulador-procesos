#ifndef STATEDIAGRAMWIDGET_H
#define STATEDIAGRAMWIDGET_H

#include <QWidget>
#include <QMap>
#include "process.h"

class QTimer;

// Dibuja el ciclo de vida del proceso como un grafo de 5 nodos, con el
// conteo de procesos en cada estado actualizándose en vivo. La última
// transición ocurrida se resalta y el nodo Ejecutando late mientras la
// CPU está ocupada.
class StateDiagramWidget : public QWidget {
    Q_OBJECT
public:
    explicit StateDiagramWidget(QWidget *parent = nullptr);

    void setCounts(const QMap<ProcState, int> &counts);

    // Resalta la arista de la última transición (se llama desde
    // MainWindow::cambiarEstado) y reinicia su desvanecimiento.
    void setUltimaTransicion(ProcState desde, ProcState hasta);
    void limpiarUltimaTransicion();

    QSize sizeHint() const override { return QSize(264, 330); }
    QSize minimumSizeHint() const override { return QSize(230, 300); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QMap<ProcState, int> m_counts;
    QMap<ProcState, QPointF> m_pos;

    // Última transición resaltada
    ProcState m_desde = ProcState::Nuevo;
    ProcState m_hasta = ProcState::Nuevo;
    bool m_hayTransicion = false;

    // Fase de la animación del halo (0..1), avanzada por el temporizador
    double m_fase = 0.0;
    QTimer *m_animacion;

    void dibujarArista(QPainter &p, const QPointF &a, const QPointF &b,
                       double radio, bool resaltada, bool punteada,
                       const QString &etiqueta) const;
};

#endif // STATEDIAGRAMWIDGET_H
