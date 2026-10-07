#include "usagegraphwidget.h"
#include "theme.h"

#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QFont>

UsageGraphWidget::UsageGraphWidget(const QString &titulo, const QColor &acento, QWidget *parent)
    : QWidget(parent), m_titulo(titulo), m_acento(acento)
{
    setMinimumHeight(120);
}

void UsageGraphWidget::agregarMuestra(double porcentaje) {
    m_muestras.append(qBound(0.0, porcentaje, 100.0));
    while (m_muestras.size() > m_maxMuestras)
        m_muestras.removeFirst();
    update();
}

void UsageGraphWidget::setSubtitulo(const QString &texto) {
    if (m_subtitulo == texto) return;
    m_subtitulo = texto;
    update();
}

void UsageGraphWidget::limpiar() {
    m_muestras.clear();
    m_subtitulo.clear();
    update();
}

void UsageGraphWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF marco = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);

    // --- Fondo del panel ---
    p.setPen(QPen(QColor(Tema::Borde), 1));
    p.setBrush(QColor(Tema::Panel));
    p.drawRoundedRect(marco, 8, 8);

    const double actual = m_muestras.isEmpty() ? 0.0 : m_muestras.last();

    QFont fMono(QStringLiteral("DejaVu Sans Mono"));
    fMono.setStyleHint(QFont::Monospace);

    // --- Encabezado: rotulo, valor actual y subtitulo ---
    fMono.setPixelSize(9);
    fMono.setWeight(QFont::DemiBold);
    fMono.setLetterSpacing(QFont::AbsoluteSpacing, 1.4);
    p.setFont(fMono);
    p.setPen(QColor(Tema::Neutro600));
    p.drawText(QRectF(marco.left() + 12, marco.top() + 9, marco.width() - 24, 14),
               Qt::AlignLeft | Qt::AlignVCenter, m_titulo.toUpper());

    fMono.setLetterSpacing(QFont::AbsoluteSpacing, 0);
    fMono.setPixelSize(17);
    p.setFont(fMono);
    p.setPen(m_acento);
    p.drawText(QRectF(marco.left() + 12, marco.top() + 5, marco.width() - 24, 22),
               Qt::AlignRight | Qt::AlignVCenter, QString::number(actual, 'f', 0) + " %");

    if (!m_subtitulo.isEmpty()) {
        fMono.setPixelSize(10);
        fMono.setWeight(QFont::Normal);
        p.setFont(fMono);
        p.setPen(QColor(Tema::Neutro600));
        p.drawText(QRectF(marco.left() + 12, marco.top() + 26, marco.width() - 24, 14),
                   Qt::AlignRight | Qt::AlignVCenter, m_subtitulo);
    }

    // --- Area de la curva ---
    QRectF area = marco.adjusted(12, 44, -12, -12);
    if (area.width() < 10 || area.height() < 10) return;

    p.setPen(QPen(QColor(Tema::BordeSuave), 1));
    for (int i = 1; i < 10; ++i) {
        qreal x = area.left() + area.width() * i / 10.0;
        p.drawLine(QPointF(x, area.top()), QPointF(x, area.bottom()));
    }
    for (int i = 1; i < 6; ++i) {
        qreal y = area.top() + area.height() * i / 6.0;
        p.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
    }
    p.setPen(QPen(QColor(Tema::Borde), 1));
    p.drawLine(QPointF(area.left(), area.bottom()), QPointF(area.right(), area.bottom()));

    if (m_muestras.isEmpty()) {
        fMono.setPixelSize(10);
        p.setFont(fMono);
        p.setPen(QColor(Tema::Neutro700));
        p.drawText(area, Qt::AlignCenter, QStringLiteral("sin muestras — avanza el reloj"));
        return;
    }

    // La muestra mas reciente se dibuja pegada al borde derecho.
    const int n = m_muestras.size();
    const qreal paso = area.width() / qreal(m_maxMuestras - 1);
    auto punto = [&](int i) {
        qreal x = area.right() - (n - 1 - i) * paso;
        qreal y = area.bottom() - (m_muestras[i] / 100.0) * area.height();
        return QPointF(x, y);
    };

    QPainterPath curva(punto(0));
    for (int i = 1; i < n; ++i) curva.lineTo(punto(i));

    QPainterPath relleno = curva;
    relleno.lineTo(QPointF(punto(n - 1).x(), area.bottom()));
    relleno.lineTo(QPointF(punto(0).x(), area.bottom()));
    relleno.closeSubpath();

    QLinearGradient grad(0, area.top(), 0, area.bottom());
    QColor c1 = m_acento; c1.setAlpha(96);
    QColor c2 = m_acento; c2.setAlpha(8);
    grad.setColorAt(0.0, c1);
    grad.setColorAt(1.0, c2);
    p.setPen(Qt::NoPen);
    p.setBrush(grad);
    p.drawPath(relleno);

    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(m_acento, 1.6));
    p.drawPath(curva);

    p.setPen(Qt::NoPen);
    p.setBrush(m_acento);
    p.drawEllipse(punto(n - 1), 2.4, 2.4);
}
