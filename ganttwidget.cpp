#include "ganttwidget.h"
#include "theme.h"
#include <QPainter>
#include <QPaintEvent>
#include <QFontMetrics>
#include <algorithm>

GanttWidget::GanttWidget(QWidget *parent) : QWidget(parent) {
    setMinimumHeight(60);
}

void GanttWidget::actualizar(const QVector<Proceso> &procesos, int reloj) {
    m_procesos = procesos;
    m_reloj = reloj;
    updateGeometry();
    update();
}

QSize GanttWidget::sizeHint() const {
    const int filas = qMax(1, m_procesos.size());
    return QSize(360, AltoEjeTicks + filas * (AltoFila + EspacioFila) + 4);
}

void GanttWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    QFont fMono(QStringLiteral("DejaVu Sans Mono"));
    fMono.setStyleHint(QFont::Monospace);

    // Sin procesos: una nota discreta en lugar de una rejilla vacia.
    if (m_procesos.isEmpty() || m_reloj <= 0) {
        p.setPen(QColor(Tema::Neutro700));
        fMono.setPixelSize(10);
        p.setFont(fMono);
        p.drawText(rect(), Qt::AlignVCenter | Qt::AlignLeft,
                   QStringLiteral("  la linea de tiempo se dibuja al avanzar el reloj"));
        return;
    }

    const int ticks = m_reloj;
    const int anchoPista = width() - AnchoEtiqueta - 4;
    // Celdas anchas cuando hay pocos ticks; se estrechan al crecer la historia.
    int anchoCelda = anchoPista / qMax(1, ticks);
    anchoCelda = std::clamp(anchoCelda, AnchoCeldaMin, AnchoCeldaMax);
    // Si ya no cabe todo, mostramos la cola reciente (los ultimos N ticks).
    const int visibles = qMin(ticks, qMax(1, anchoPista / anchoCelda));
    const int primerTick = ticks - visibles;

    // --- Eje de ticks: una marca cada 5 ---
    fMono.setPixelSize(9);
    p.setFont(fMono);
    p.setPen(QColor(Tema::Neutro700));
    for (int t = primerTick; t < ticks; ++t) {
        if (t % 5 != 0) continue;
        const int x = AnchoEtiqueta + (t - primerTick) * anchoCelda;
        p.drawText(QRect(x, 0, 34, AltoEjeTicks - 2),
                   Qt::AlignLeft | Qt::AlignVCenter, QString("t%1").arg(t));
    }

    // --- Una fila por proceso ---
    int y = AltoEjeTicks;
    QFont fUI = font();
    fUI.setPixelSize(10);

    for (const Proceso &proc : m_procesos) {
        // Etiqueta: nombre recortado + PID
        p.setFont(fUI);
        p.setPen(QColor(Tema::Neutro400));
        const QString etiqueta = QFontMetrics(fUI).elidedText(
            proc.nombre, Qt::ElideRight, AnchoEtiqueta - 32);
        p.drawText(QRect(0, y, AnchoEtiqueta - 30, AltoFila),
                   Qt::AlignLeft | Qt::AlignVCenter, etiqueta);
        p.setFont(fMono);
        p.setPen(QColor(Tema::Neutro700));
        p.drawText(QRect(AnchoEtiqueta - 28, y, 22, AltoFila),
                   Qt::AlignRight | Qt::AlignVCenter, QString("#%1").arg(proc.pid));

        // Celdas
        for (int t = primerTick; t < ticks; ++t) {
            const QRect celda(AnchoEtiqueta + (t - primerTick) * anchoCelda, y,
                              anchoCelda - 1, AltoFila);
            ProcState est;
            if (!estadoEnTick(proc, t, est)) {
                // Antes de existir: nada, solo la pista vacia.
                p.fillRect(celda, QColor(Tema::BordeSuave));
                continue;
            }
            QColor c = Tema::colorEstado(est);
            // Ejecutando en pleno; los demas estados en tinte, para que la
            // rafaga de CPU sea lo que salta a la vista.
            if (est != ProcState::Ejecutando) c.setAlpha(est == ProcState::Terminado ? 60 : 120);
            p.fillRect(celda, c);
        }

        y += AltoFila + EspacioFila;
    }
}
