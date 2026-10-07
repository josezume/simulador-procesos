#include "statediagramwidget.h"
#include "theme.h"

#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QFont>
#include <QtMath>

StateDiagramWidget::StateDiagramWidget(QWidget *parent) : QWidget(parent) {
    // Posiciones relativas (0..1) de cada nodo dentro del widget
    m_pos[ProcState::Nuevo]      = QPointF(0.14, 0.13);
    m_pos[ProcState::Listo]      = QPointF(0.55, 0.36);
    m_pos[ProcState::Ejecutando] = QPointF(0.18, 0.63);
    m_pos[ProcState::Bloqueado]  = QPointF(0.80, 0.63);
    m_pos[ProcState::Terminado]  = QPointF(0.50, 0.90);

    // Late a 25 fps solo mientras haya un proceso en la CPU.
    m_animacion = new QTimer(this);
    m_animacion->setInterval(40);
    connect(m_animacion, &QTimer::timeout, this, [this] {
        m_fase += 0.02;
        if (m_fase > 1.0) m_fase -= 1.0;
        update();
    });
}

void StateDiagramWidget::setCounts(const QMap<ProcState, int> &counts) {
    m_counts = counts;
    const bool cpuOcupada = counts.value(ProcState::Ejecutando, 0) > 0;
    if (cpuOcupada && !m_animacion->isActive()) m_animacion->start();
    if (!cpuOcupada && m_animacion->isActive()) m_animacion->stop();
    update();
}

void StateDiagramWidget::setUltimaTransicion(ProcState desde, ProcState hasta) {
    m_desde = desde;
    m_hasta = hasta;
    m_hayTransicion = true;
    update();
}

void StateDiagramWidget::limpiarUltimaTransicion() {
    m_hayTransicion = false;
    update();
}

// Traza la arista entre dos nodos, recortada para no meterse bajo los
// círculos, con punta de flecha y etiqueta en el punto medio.
void StateDiagramWidget::dibujarArista(QPainter &p, const QPointF &a, const QPointF &b,
                                       double radio, bool resaltada, bool punteada,
                                       const QString &etiqueta) const {
    QLineF linea(a, b);
    const double largo = linea.length();
    if (largo <= 2 * radio + 10) return;

    // Recorte simétrico en ambos extremos
    QLineF desdeA(a, b); desdeA.setLength(radio + 5);
    QLineF desdeB(b, a); desdeB.setLength(radio + 7);
    const QPointF p1 = desdeA.p2();
    const QPointF p2 = desdeB.p2();

    QPen pluma(resaltada ? QColor(Tema::Acento) : QColor(Tema::Neutro700));
    pluma.setWidthF(resaltada ? 2.0 : 1.3);
    if (punteada) pluma.setDashPattern({3.0, 3.0});
    p.setPen(pluma);
    p.setBrush(Qt::NoBrush);
    p.drawLine(p1, p2);

    // Punta de flecha
    QLineF eje(p1, p2);
    const double ang = qDegreesToRadians(eje.angle());
    const QPointF dir(qCos(ang), -qSin(ang));
    const QPointF perp(-dir.y(), dir.x());
    const double L = 7.0, W = 3.4;
    QPainterPath punta;
    punta.moveTo(p2);
    punta.lineTo(p2 - dir * L + perp * W);
    punta.lineTo(p2 - dir * L - perp * W);
    punta.closeSubpath();
    p.setPen(Qt::NoPen);
    p.setBrush(resaltada ? QColor(Tema::Acento) : QColor(Tema::Neutro700));
    p.drawPath(punta);

    if (!etiqueta.isEmpty()) {
        QFont f = p.font();
        f.setPointSizeF(7.0);
        f.setFamily("DejaVu Sans Mono");
        p.setFont(f);
        p.setPen(resaltada ? QColor(Tema::Acento300) : QColor(Tema::Neutro700));
        const QPointF medio = (p1 + p2) / 2.0 + perp * 9.0;
        p.drawText(QRectF(medio.x() - 40, medio.y() - 8, 80, 16),
                   Qt::AlignCenter, etiqueta);
    }
}

void StateDiagramWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const qreal w = width(), h = height();
    const qreal r = qMin(24.0, qMin(w, h) / 11.0 + 12.0);

    // Las posiciones relativas se mapean dentro de una franja util que reserva
    // el radio del circulo arriba y, abajo, el radio mas la altura del rotulo.
    // Sin esta reserva, la etiqueta del nodo mas bajo (TERMINADO) quedaba
    // cortada contra el borde inferior del widget.
    const qreal yMin = r + 2.0;
    const qreal yMax = qMax(yMin + 1.0, h - (r + 19.0));

    auto pt = [&](ProcState s) {
        const QPointF f = m_pos[s];
        return QPointF(f.x() * w, yMin + f.y() * (yMax - yMin));
    };

    // --- Aristas: el ciclo de vida completo ---
    struct Arista { ProcState a, b; const char *etq; bool punteada; };
    // Las etiquetas se dejan vacias para un diagrama mas limpio. Basta con
    // volver a escribir el texto ("admitir", "despachar", ...) para mostrarlas.
    const Arista aristas[] = {
        { ProcState::Nuevo,      ProcState::Listo,      "", false },
        { ProcState::Listo,      ProcState::Ejecutando, "", false },
        { ProcState::Ejecutando, ProcState::Listo,      "", true  },
        { ProcState::Ejecutando, ProcState::Bloqueado,  "", false },
        { ProcState::Bloqueado,  ProcState::Listo,      "", false },
        { ProcState::Ejecutando, ProcState::Terminado,  "", false },
    };
    for (const auto &e : aristas) {
        const bool resaltada = m_hayTransicion && m_desde == e.a && m_hasta == e.b;
        dibujarArista(p, pt(e.a), pt(e.b), r, resaltada, e.punteada, e.etq);
    }

    // --- Nodos ---
    const ProcState orden[] = { ProcState::Nuevo, ProcState::Listo, ProcState::Ejecutando,
                                ProcState::Bloqueado, ProcState::Terminado };
    for (ProcState s : orden) {
        const QPointF c = pt(s);
        const QColor col = Tema::colorEstado(s);
        const int n = m_counts.value(s, 0);
        const bool activo = n > 0;

        // Halo latiente: solo en Ejecutando y solo si la CPU está ocupada
        if (s == ProcState::Ejecutando && activo) {
            const double t = qSin(m_fase * 2 * M_PI) * 0.5 + 0.5;   // 0..1
            QColor halo = col;
            halo.setAlpha(int(46 - 34 * t));
            p.setPen(Qt::NoPen);
            p.setBrush(halo);
            p.drawEllipse(c, r + 6 + 8 * t, r + 6 + 8 * t);
        }

        p.setPen(QPen(activo ? col : QColor(Tema::Neutro800), activo ? 1.6 : 1.2));
        p.setBrush(Tema::tinte(Tema::colorEstadoHex(s), activo ? 40 : 16));
        p.drawEllipse(c, r, r);

        QFont fn = p.font();
        fn.setFamily("DejaVu Sans Mono");
        fn.setPointSizeF(11.5);
        fn.setWeight(QFont::DemiBold);
        p.setFont(fn);
        p.setPen(activo ? col : QColor(Tema::Neutro700));
        p.drawText(QRectF(c.x() - r, c.y() - r, 2 * r, 2 * r), Qt::AlignCenter,
                   QString::number(n));

        QFont fl = p.font();
        fl.setFamily("DejaVu Sans Mono");
        fl.setPointSizeF(7.0);
        fl.setWeight(QFont::Normal);
        p.setFont(fl);
        p.setPen(activo ? QColor(Tema::Neutro400) : QColor(Tema::Neutro700));
        p.drawText(QRectF(c.x() - 52, c.y() + r + 3, 104, 14), Qt::AlignCenter,
                   estadoATexto(s).toUpper());
    }
}
