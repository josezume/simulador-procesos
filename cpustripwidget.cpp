#include "cpustripwidget.h"
#include "theme.h"
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLayoutItem>
#include <algorithm>

static QLabel *rotulo(const QString &texto) {
    auto *l = new QLabel(texto);
    l->setObjectName("sectionLabel");
    return l;
}

CpuStripWidget::CpuStripWidget(QWidget *parent) : QWidget(parent) {
    auto *raiz = new QHBoxLayout(this);
    raiz->setContentsMargins(0, 0, 0, 0);
    raiz->setSpacing(10);

    // --- Izquierda: el nucleo ---
    m_cajaCpu = new QWidget();
    m_cajaCpu->setObjectName("cajaCpu");
    m_cajaCpu->setMinimumWidth(300);
    m_cajaCpu->setStyleSheet(QString(
        "QWidget#cajaCpu { background:%1; border:1px solid %2; border-radius:8px; }"
        "QLabel { background:transparent; }")
        .arg(Tema::Panel).arg(Tema::Borde));
    m_layCpu = new QVBoxLayout(m_cajaCpu);
    m_layCpu->setContentsMargins(12, 10, 12, 11);
    m_layCpu->setSpacing(7);
    raiz->addWidget(m_cajaCpu, 5);

    // --- Derecha: la cola de listos ---
    m_cajaCola = new QWidget();
    m_cajaCola->setObjectName("cajaCola");
    m_cajaCola->setMinimumWidth(220);
    m_cajaCola->setStyleSheet(QString(
        "QWidget#cajaCola { background:%1; border:1px solid %2; border-radius:8px; }"
        "QLabel { background:transparent; }")
        .arg(Tema::Panel).arg(Tema::Borde));
    auto *layCajaCola = new QVBoxLayout(m_cajaCola);
    layCajaCola->setContentsMargins(12, 10, 12, 11);
    layCajaCola->setSpacing(8);

    auto *cabCola = new QHBoxLayout();
    cabCola->setSpacing(8);
    cabCola->addWidget(rotulo(QStringLiteral("COLA DE LISTOS")));
    m_tituloCola = new QLabel(QStringLiteral("ordenada por prioridad"));
    m_tituloCola->setObjectName("sub");
    cabCola->addWidget(m_tituloCola);
    cabCola->addStretch();
    layCajaCola->addLayout(cabCola);

    auto *envolturaCola = new QWidget();
    m_layCola = new QHBoxLayout(envolturaCola);
    m_layCola->setContentsMargins(0, 0, 0, 0);
    m_layCola->setSpacing(7);
    layCajaCola->addWidget(envolturaCola);
    layCajaCola->addStretch();
    raiz->addWidget(m_cajaCola, 6);
}

void CpuStripWidget::vaciar(QLayout *layout) {
    while (QLayoutItem *item = layout->takeAt(0)) {
        if (QWidget *w = item->widget()) w->deleteLater();
        if (QLayout *hijo = item->layout()) vaciar(hijo);
        delete item;
    }
}

void CpuStripWidget::actualizar(const QVector<Proceso> &procesos) {
    vaciar(m_layCpu);
    vaciar(m_layCola);

    // --- Quien tiene la CPU ---
    const Proceso *enCpu = nullptr;
    for (const Proceso &p : procesos)
        if (p.estado == ProcState::Ejecutando) { enCpu = &p; break; }

    if (enCpu) pintarCpu(*enCpu);
    else pintarCpuVacia();

    // --- Cola de listos, ordenada como la ve el planificador ---
    QVector<const Proceso *> listos;
    for (const Proceso &p : procesos)
        if (p.estado == ProcState::Listo) listos.append(&p);
    std::stable_sort(listos.begin(), listos.end(),
                     [](const Proceso *a, const Proceso *b) {
                         if (a->prioridad != b->prioridad) return a->prioridad > b->prioridad;
                         return a->tickLlegada < b->tickLlegada;
                     });

    m_tituloCola->setText(listos.isEmpty()
        ? QStringLiteral("vacia")
        : QString("%1 en espera · prioridad, luego FCFS").arg(listos.size()));

    if (listos.isEmpty()) {
        auto *vacio = new QLabel(QStringLiteral("ningun proceso listo para despachar"));
        vacio->setStyleSheet(QString("font-family:%1; font-size:10.5px; color:%2;")
                                 .arg(Tema::FamiliaMono).arg(Tema::Neutro700));
        m_layCola->addWidget(vacio);
    }

    for (int i = 0; i < listos.size(); ++i) {
        const Proceso *p = listos[i];
        const bool primero = (i == 0);
        auto *btn = new QPushButton();
        btn->setCursor(Qt::PointingHandCursor);
        btn->setToolTip(QString("Despachar %1 a la CPU").arg(p->nombre));
        btn->setText(QString("p%1  ·  %2  #%3").arg(p->prioridad).arg(p->nombre).arg(p->pid));
        // El primero de la cola es el siguiente en entrar: contorno de acento.
        btn->setStyleSheet(QString(
            "QPushButton { font-family:%1; font-size:10.5px; font-weight:500;"
            " color:%2; background:%3; border:1px solid %4;"
            " border-radius:6px; padding:6px 10px; }"
            "QPushButton:hover { border-color:%5; }")
            .arg(Tema::FamiliaMono)
            .arg(primero ? Tema::Texto : Tema::Neutro400)
            .arg(Tema::Superficie)
            .arg(primero ? "rgba(145,132,217,140)" : Tema::Borde)
            .arg(Tema::Acento));
        const int pid = p->pid;
        connect(btn, &QPushButton::clicked, this, [this, pid]{ emit accionDespachar(pid); });
        m_layCola->addWidget(btn);
    }
    m_layCola->addStretch();
}

void CpuStripWidget::pintarCpuVacia() {
    auto *cab = new QHBoxLayout();
    cab->setSpacing(8);
    cab->addWidget(rotulo(QStringLiteral("CPU")));
    auto *estado = new QLabel(QStringLiteral("inactiva"));
    estado->setObjectName("sub");
    cab->addWidget(estado);
    cab->addStretch();
    m_layCpu->addLayout(cab);

    auto *nota = new QLabel(QStringLiteral("Ningun proceso en ejecucion.\nDespacha uno desde la cola de listos."));
    nota->setStyleSheet(QString("font-size:11.5px; line-height:1.5; color:%1;").arg(Tema::Neutro600));
    m_layCpu->addWidget(nota);
    m_layCpu->addStretch();
}

void CpuStripWidget::pintarCpu(const Proceso &p) {
    const QString col = Tema::colorEstadoHex(ProcState::Ejecutando);

    // Cabecera: rotulo + tick de entrada
    auto *cab = new QHBoxLayout();
    cab->setSpacing(8);
    cab->addWidget(rotulo(QStringLiteral("CPU")));
    auto *estado = new QLabel(QString("ocupada · %1 cambios de contexto").arg(p.cambiosContexto));
    estado->setObjectName("sub");
    cab->addWidget(estado);
    cab->addStretch();
    m_layCpu->addLayout(cab);

    // Nombre del proceso que la ocupa
    auto *fila = new QHBoxLayout();
    fila->setSpacing(8);
    auto *nombre = new QLabel(p.nombre);
    nombre->setStyleSheet(QString("font-size:14px; font-weight:500; color:%1;").arg(Tema::Texto));
    auto *meta = new QLabel(QString("PID %1 · prio %2").arg(p.pid).arg(p.prioridad));
    meta->setStyleSheet(QString("font-family:%1; font-size:11px; color:%2;")
                            .arg(Tema::FamiliaMono).arg(Tema::Neutro600));
    fila->addWidget(nombre);
    fila->addWidget(meta);
    fila->addStretch();
    m_layCpu->addLayout(fila);

    // Progreso de la rafaga
    const int hechos = p.rafagaTotal - p.rafagaRestante;
    const int pct = p.rafagaTotal > 0 ? hechos * 100 / p.rafagaTotal : 0;
    auto *pista = new QWidget();
    pista->setFixedHeight(6);
    pista->setStyleSheet(QString("background:%1; border-radius:3px;").arg(Tema::Neutro900));
    auto *pistaLay = new QHBoxLayout(pista);
    pistaLay->setContentsMargins(0, 0, 0, 0);
    pistaLay->setSpacing(0);
    auto *relleno = new QWidget();
    relleno->setStyleSheet(QString("background:%1; border-radius:3px;").arg(col));
    pistaLay->addWidget(relleno, qMax(pct, 0));
    pistaLay->addStretch(qMax(100 - pct, 0));
    m_layCpu->addWidget(pista);

    const QString pc = QString("0x%1").arg(p.contadorPrograma, 4, 16, QChar('0')).toUpper();
    auto *detalle = new QLabel(QString("rafaga %1 / %2 ticks · quedan %3 · PC %4")
                                   .arg(hechos).arg(p.rafagaTotal).arg(p.rafagaRestante).arg(pc));
    detalle->setStyleSheet(QString("font-family:%1; font-size:10.5px; color:%2;")
                               .arg(Tema::FamiliaMono).arg(Tema::Neutro500));
    m_layCpu->addWidget(detalle);

    // Acciones sobre el proceso en CPU
    auto *acciones = new QHBoxLayout();
    acciones->setSpacing(6);
    auto mk = [&](const QString &texto) {
        auto *b = new QPushButton(texto);
        b->setObjectName("chip");
        b->setCursor(Qt::PointingHandCursor);
        acciones->addWidget(b);
        return b;
    };
    const int pid = p.pid;
    connect(mk(QStringLiteral("Bloquear E/S")), &QPushButton::clicked, this,
            [this, pid]{ emit accionBloquear(pid); });
    connect(mk(QStringLiteral("Expropiar")), &QPushButton::clicked, this,
            [this, pid]{ emit accionExpropiar(pid); });
    connect(mk(QStringLiteral("Terminar")), &QPushButton::clicked, this,
            [this, pid]{ emit accionTerminar(pid); });
    connect(mk(QStringLiteral("Ver PCB")), &QPushButton::clicked, this,
            [this, pid]{ emit verDetalle(pid); });
    acciones->addStretch();
    m_layCpu->addLayout(acciones);
}
