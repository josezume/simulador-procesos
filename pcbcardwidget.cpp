#include "pcbcardwidget.h"
#include "theme.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QMouseEvent>

// Etiqueta pequena tipo "chip" con fondo tenue.
static QLabel *chip(const QString &texto, const QString &color = QString()) {
    auto *l = new QLabel(texto);
    l->setStyleSheet(QString("font-family:%1; font-size:9.5px; font-weight:500;"
                             "color:%2; background:rgba(233,233,237,18);"
                             "border-radius:3px; padding:3px 5px;")
                         .arg(Tema::FamiliaMono)
                         .arg(color.isEmpty() ? Tema::Neutro300 : color));
    return l;
}

PcbCardWidget::PcbCardWidget(const Proceso &proc, QWidget *parent)
    : QWidget(parent), m_pid(proc.pid)
{
    setCursor(Qt::PointingHandCursor);
    const QString col = Tema::colorEstadoHex(proc.estado);

    // El borde izquierdo de 3px es la unica marca de color de la tarjeta.
    setStyleSheet(QString(
        "PcbCardWidget { background:%1; border:1px solid %2;"
        " border-left:3px solid %3; border-radius:3px 7px 7px 3px; }"
        "PcbCardWidget:hover { border-color:%4; border-left:3px solid %3; }"
        "QLabel { background:transparent; border:none; }")
        .arg(Tema::Superficie)
        .arg(Tema::Borde)
        .arg(col)
        .arg(Tema::Neutro600));

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(9, 8, 9, 8);
    outer->setSpacing(6);

    // --- Nombre + PID ---
    auto *top = new QHBoxLayout();
    top->setSpacing(6);
    auto *nombre = new QLabel(proc.nombre);
    nombre->setStyleSheet(QString("font-size:12.5px; font-weight:500; color:%1;").arg(Tema::Texto));
    auto *pid = new QLabel(QString("#%1").arg(proc.pid));
    pid->setStyleSheet(QString("font-family:%1; font-size:10px; font-weight:600; color:%2;")
                           .arg(Tema::FamiliaMono).arg(Tema::Neutro500));
    top->addWidget(nombre, 1);
    top->addWidget(pid, 0, Qt::AlignRight);
    outer->addLayout(top);

    // --- Chips de datos del PCB ---
    const bool terminado = (proc.estado == ProcState::Terminado);
    const double cpuPct = cpuPorcentaje(proc);
    auto *chips = new QHBoxLayout();
    chips->setSpacing(5);
    chips->addWidget(chip(QString("prio %1").arg(proc.prioridad)));
    if (terminado) {
        chips->addWidget(chip(QString("ret %1t").arg(tiempoRetorno(proc))));
        chips->addWidget(chip(QString("ctx %1").arg(proc.cambiosContexto)));
    } else {
        chips->addWidget(chip(QString("CPU %1%").arg(cpuPct, 0, 'f', 0),
                              cpuPct > 0.1 ? Tema::EstEjecutando : QString()));
        chips->addWidget(chip(formatoMemoria(proc.memoriaMB)));
        if (!proc.recursosAsignados.isEmpty())
            chips->addWidget(chip(QString("rec %1").arg(proc.recursosAsignados.size())));
        if (!proc.recursosSolicitados.isEmpty())
            chips->addWidget(chip(QString("pide %1").arg(proc.recursosSolicitados.first()),
                                  Tema::EstBloqueado));
    }
    chips->addStretch();
    outer->addLayout(chips);

    // --- Progreso de la rafaga de CPU ---
    const int hechos = proc.rafagaTotal - proc.rafagaRestante;
    const int pct = proc.rafagaTotal > 0 ? hechos * 100 / proc.rafagaTotal : 0;

    auto *fila = new QHBoxLayout();
    fila->setSpacing(6);
    auto *pista = new QWidget();
    pista->setFixedHeight(4);
    pista->setStyleSheet(QString("background:%1; border-radius:2px;").arg(Tema::Neutro900));
    auto *pistaLay = new QHBoxLayout(pista);
    pistaLay->setContentsMargins(0, 0, 0, 0);
    pistaLay->setSpacing(0);
    auto *relleno = new QWidget();
    relleno->setStyleSheet(QString("background:%1; border-radius:2px;").arg(col));
    pistaLay->addWidget(relleno, qMax(pct, 0));
    pistaLay->addStretch(qMax(100 - pct, 0));
    fila->addWidget(pista, 1);
    auto *rafaga = new QLabel(QString("%1/%2").arg(hechos).arg(proc.rafagaTotal));
    rafaga->setStyleSheet(QString("font-family:%1; font-size:9.5px; color:%2;")
                              .arg(Tema::FamiliaMono).arg(Tema::Neutro600));
    fila->addWidget(rafaga);
    outer->addLayout(fila);

    // --- Acciones contextuales ---
    if (!terminado) {
        auto *acciones = new QHBoxLayout();
        acciones->setSpacing(5);
        construirAcciones(acciones, proc.estado);
        outer->addLayout(acciones);
    }
}

void PcbCardWidget::construirAcciones(QHBoxLayout *layout, ProcState estado) {
    auto mkBtn = [&](const QString &texto) {
        auto *b = new QPushButton(texto);
        b->setObjectName("chip");
        b->setCursor(Qt::PointingHandCursor);
        return b;
    };

    switch (estado) {
    case ProcState::Nuevo: {
        auto *b = mkBtn("Admitir");
        connect(b, &QPushButton::clicked, this, [this]{ emit accionAdmitir(m_pid); });
        layout->addWidget(b);
        break;
    }
    case ProcState::Listo: {
        auto *b1 = mkBtn("Despachar");
        auto *b2 = mkBtn("Terminar");
        connect(b1, &QPushButton::clicked, this, [this]{ emit accionDespachar(m_pid); });
        connect(b2, &QPushButton::clicked, this, [this]{ emit accionTerminar(m_pid); });
        layout->addWidget(b1);
        layout->addWidget(b2);
        break;
    }
    case ProcState::Ejecutando: {
        auto *b1 = mkBtn("Bloquear E/S");
        auto *b2 = mkBtn("Expropiar");
        auto *b3 = mkBtn("Terminar");
        connect(b1, &QPushButton::clicked, this, [this]{ emit accionBloquear(m_pid); });
        connect(b2, &QPushButton::clicked, this, [this]{ emit accionExpropiar(m_pid); });
        connect(b3, &QPushButton::clicked, this, [this]{ emit accionTerminar(m_pid); });
        layout->addWidget(b1);
        layout->addWidget(b2);
        layout->addWidget(b3);
        break;
    }
    case ProcState::Bloqueado: {
        auto *b = mkBtn("E/S completa");
        connect(b, &QPushButton::clicked, this, [this]{ emit accionDespertar(m_pid); });
        layout->addWidget(b);
        break;
    }
    case ProcState::Terminado:
        break;
    }
    layout->addStretch();
}

void PcbCardWidget::mousePressEvent(QMouseEvent *event) {
    emit verDetalle(m_pid);
    QWidget::mousePressEvent(event);
}
