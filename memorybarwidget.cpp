#include "memorybarwidget.h"
#include "theme.h"
#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>
#include <algorithm>

MemoryBarWidget::MemoryBarWidget(QWidget *parent) : QWidget(parent) {
    setMinimumHeight(AltoBarra + AltoLeyenda + 20);
}

void MemoryBarWidget::actualizar(const QVector<Proceso> &procesos) {
    m_segmentos.clear();

    // El SO siempre ocupa su bloque, al principio de la memoria fisica.
    m_segmentos.append({QStringLiteral("Sistema"), MEMORIA_SO_MB, QColor(Tema::Neutro700)});

    QVector<const Proceso *> vivos;
    for (const Proceso &p : procesos)
        if (p.estado != ProcState::Terminado && p.memoriaMB > 0.0) vivos.append(&p);
    std::stable_sort(vivos.begin(), vivos.end(),
                     [](const Proceso *a, const Proceso *b) { return a->memoriaMB > b->memoriaMB; });

    for (const Proceso *p : vivos)
        m_segmentos.append({p->nombre, p->memoriaMB, Tema::colorEstado(p->estado)});

    m_usadaMB = memoriaUsadaMB(procesos);
    const double libre = qMax(0.0, MEMORIA_TOTAL_MB - m_usadaMB);
    m_segmentos.append({QStringLiteral("Libre"), libre, QColor(Tema::Neutro900)});

    update();
}

void MemoryBarWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QFont fMono(QStringLiteral("DejaVu Sans Mono"));
    fMono.setStyleHint(QFont::Monospace);

    // --- Cabecera: total usado / total fisico ---
    fMono.setPixelSize(9);
    fMono.setWeight(QFont::DemiBold);
    fMono.setLetterSpacing(QFont::AbsoluteSpacing, 1.4);
    p.setFont(fMono);
    p.setPen(QColor(Tema::Neutro600));
    p.drawText(QRect(0, 0, width(), 12), Qt::AlignLeft | Qt::AlignVCenter,
               QStringLiteral("MEMORIA FISICA"));

    fMono.setLetterSpacing(QFont::AbsoluteSpacing, 0);
    fMono.setPixelSize(11);
    fMono.setWeight(QFont::Normal);
    p.setFont(fMono);
    p.setPen(QColor(Tema::Neutro500));
    p.drawText(QRect(0, 0, width(), 12), Qt::AlignRight | Qt::AlignVCenter,
               QString("%1 / %2").arg(formatoMemoria(m_usadaMB), formatoMemoria(MEMORIA_TOTAL_MB)));

    if (m_segmentos.isEmpty()) return;

    // --- Barra segmentada ---
    const QRect barra(0, 20, width(), AltoBarra);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(Tema::Neutro900));
    p.drawRoundedRect(barra, 4, 4);

    p.save();
    QPainterPath recorte;
    recorte.addRoundedRect(QRectF(barra), 4, 4);
    p.setClipPath(recorte);

    fMono.setPixelSize(10);
    QFontMetrics fm(fMono);
    p.setFont(fMono);

    double x = 0.0;
    for (const Segmento &s : m_segmentos) {
        const double ancho = width() * (s.mb / MEMORIA_TOTAL_MB);
        const QRectF celda(barra.x() + x, barra.y(), ancho, barra.height());
        if (s.etiqueta != QStringLiteral("Libre")) {
            p.setPen(Qt::NoPen);
            QColor c = s.color;
            c.setAlpha(s.etiqueta == QStringLiteral("Sistema") ? 200 : 170);
            p.setBrush(c);
            p.drawRect(celda);
            // Separador de 1px entre segmentos
            p.setPen(QPen(QColor(Tema::Fondo), 1));
            p.drawLine(QPointF(celda.right(), celda.top()), QPointF(celda.right(), celda.bottom()));
        }
        // Solo rotulamos donde de verdad cabe el texto.
        const QString texto = fm.elidedText(s.etiqueta, Qt::ElideRight, int(ancho) - 10);
        if (ancho > 62 && !texto.isEmpty()) {
            p.setPen(QColor(s.etiqueta == QStringLiteral("Libre") ? Tema::Neutro600 : Tema::Fondo));
            p.drawText(celda.adjusted(5, 0, -5, 0), Qt::AlignVCenter | Qt::AlignLeft, texto);
        }
        x += ancho;
    }
    p.restore();

    // --- Leyenda: dos filas de puntos con nombre y tamano ---
    fMono.setPixelSize(10);
    p.setFont(fMono);
    QFontMetrics fmL(fMono);

    double lx = 0.0;
    int ly = barra.bottom() + 9;
    for (const Segmento &s : m_segmentos) {
        if (s.etiqueta == QStringLiteral("Libre")) continue;
        const QString texto = QString("%1 %2").arg(s.etiqueta, formatoMemoria(s.mb));
        const double ancho = fmL.horizontalAdvance(texto) + 16;
        if (lx + ancho > width()) {
            lx = 0.0;
            ly += 13;
            if (ly > height() - 6) break;
        }
        p.setPen(Qt::NoPen);
        p.setBrush(s.color);
        p.drawEllipse(QRectF(lx, ly + 3, 5, 5));
        p.setPen(QColor(Tema::Neutro500));
        p.drawText(QRectF(lx + 9, ly, ancho - 9, 11), Qt::AlignLeft | Qt::AlignVCenter, texto);
        lx += ancho;
    }
}
