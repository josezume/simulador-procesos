#ifndef MEMORYBARWIDGET_H
#define MEMORYBARWIDGET_H

#include <QWidget>
#include <QVector>
#include "process.h"

// ============================================================
//  Barra de memoria fisica por segmentos
// ------------------------------------------------------------
//  Un segmento por proceso vivo, mas el bloque del sistema
//  operativo y el hueco libre. Solo se rotulan los segmentos que
//  tienen sitio; el resto se identifica en la leyenda de abajo.
// ============================================================
class MemoryBarWidget : public QWidget {
    Q_OBJECT
public:
    explicit MemoryBarWidget(QWidget *parent = nullptr);

    void actualizar(const QVector<Proceso> &procesos);

    QSize sizeHint() const override { return QSize(360, 74); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    struct Segmento {
        QString etiqueta;
        double mb = 0.0;
        QColor color;
    };
    QVector<Segmento> m_segmentos;
    double m_usadaMB = 0.0;

    static constexpr int AltoBarra   = 22;
    static constexpr int AltoLeyenda = 30;
};

#endif // MEMORYBARWIDGET_H
