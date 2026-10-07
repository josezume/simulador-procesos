#ifndef USAGEGRAPHWIDGET_H
#define USAGEGRAPHWIDGET_H

#include <QWidget>
#include <QColor>
#include <QVector>

// Grafica de area con historial deslizante (0..100 %). Las muestras nuevas
// entran por la derecha y desplazan a las antiguas.
class UsageGraphWidget : public QWidget {
    Q_OBJECT
public:
    explicit UsageGraphWidget(const QString &titulo, const QColor &acento,
                              QWidget *parent = nullptr);

    void agregarMuestra(double porcentaje);      // valor 0..100
    void setSubtitulo(const QString &texto);     // linea auxiliar (ej. "2.9 / 8.0 GB")
    void limpiar();

    QSize sizeHint() const override { return QSize(340, 150); }
    QSize minimumSizeHint() const override { return QSize(220, 120); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_titulo;
    QString m_subtitulo;
    QColor m_acento;
    QVector<double> m_muestras;
    int m_maxMuestras = 60;   // ancho del historial visible
};

#endif // USAGEGRAPHWIDGET_H
