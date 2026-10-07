#include "taskmanagerwidget.h"
#include "usagegraphwidget.h"
#include "memorybarwidget.h"
#include "theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QTableWidgetItem>

namespace {

// Item de tabla que ordena por valor numerico y no alfabeticamente
// (para que "10 %" quede despues de "9 %").
class ItemNumerico : public QTableWidgetItem {
public:
    ItemNumerico(const QString &texto, double valor) : QTableWidgetItem(texto) {
        setData(Qt::UserRole, valor);
    }
    bool operator<(const QTableWidgetItem &otro) const override {
        return data(Qt::UserRole).toDouble() < otro.data(Qt::UserRole).toDouble();
    }
};

// Sombreado de celda proporcional al valor: la columna se lee como
// un histograma sin dejar de ser texto.
QColor sombreado(const QString &hex, double valor01) {
    QColor c(hex);
    c.setAlpha(int(10 + qBound(0.0, valor01, 1.0) * 70));
    return c;
}

} // namespace

TaskManagerWidget::TaskManagerWidget(QWidget *parent) : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(14, 12, 14, 12);
    root->setSpacing(11);

    // ---------- Metricas globales ----------
    auto *metricas = new QHBoxLayout();
    metricas->setSpacing(9);
    metricas->addWidget(crearMetrica(QStringLiteral("USO DE CPU"),   m_valCpu,    m_subCpu));
    metricas->addWidget(crearMetrica(QStringLiteral("MEMORIA"),      m_valMem,    m_subMem));
    metricas->addWidget(crearMetrica(QStringLiteral("PROCESOS"),     m_valProc,   m_subProc));
    metricas->addWidget(crearMetrica(QStringLiteral("ESPERA MEDIA"), m_valEspera, m_subEspera));
    root->addLayout(metricas);

    // ---------- Graficas ----------
    auto *graficas = new QHBoxLayout();
    graficas->setSpacing(9);
    m_graficaCpu = new UsageGraphWidget(QStringLiteral("Uso de CPU"), QColor(Tema::Acento));
    m_graficaMem = new UsageGraphWidget(QStringLiteral("Memoria en uso"), QColor(Tema::Neutro500));
    graficas->addWidget(m_graficaCpu);
    graficas->addWidget(m_graficaMem);
    root->addLayout(graficas);

    // ---------- Mapa de memoria fisica ----------
    auto *marcoMem = new QFrame();
    marcoMem->setStyleSheet(QString("QFrame { background:%1; border:1px solid %2;"
                                    " border-radius:8px; }")
                                .arg(Tema::Panel).arg(Tema::Borde));
    auto *layMem = new QVBoxLayout(marcoMem);
    layMem->setContentsMargins(12, 10, 12, 10);
    m_barraMemoria = new MemoryBarWidget();
    layMem->addWidget(m_barraMemoria);
    root->addWidget(marcoMem);

    // ---------- Tabla de procesos ----------
    const QStringList encabezados = {
        "PID", "NOMBRE", "ESTADO", "CPU %", "MEMORIA", "T. CPU",
        "ESPERA", "BLOQ.", "E/S", "CTX", "HILOS", "PRIO."
    };
    const QStringList ayudas = {
        "Identificador del proceso",
        "Nombre del programa",
        "Estado actual en el ciclo de vida",
        "Porcentaje de CPU en la ventana reciente",
        "Huella de memoria actual",
        "Ticks ejecutados / rafaga total",
        "Ticks acumulados en la cola de listos",
        "Ticks acumulados esperando E/S",
        "Operaciones de E/S solicitadas",
        "Cambios de contexto (entradas a la CPU)",
        "Hilos del proceso",
        "Prioridad (9 = mas alta)"
    };
    m_tabla = new QTableWidget(0, encabezados.size());
    m_tabla->setHorizontalHeaderLabels(encabezados);
    for (int c = 0; c < encabezados.size(); ++c)
        m_tabla->horizontalHeaderItem(c)->setToolTip(ayudas.at(c));
    m_tabla->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tabla->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tabla->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tabla->setShowGrid(false);
    m_tabla->verticalHeader()->setVisible(false);
    m_tabla->verticalHeader()->setDefaultSectionSize(26);
    m_tabla->setSortingEnabled(true);
    m_tabla->sortByColumn(3, Qt::DescendingOrder);   // por defecto, por CPU %

    // Anchos compactos y fijos: con 12 columnas, ResizeToContents desborda
    // el ancho disponible y obliga a desplazarse en horizontal.
    const int anchos[] = {46, 0, 92, 74, 88, 74, 68, 60, 48, 46, 52, 52};
    auto *cab = m_tabla->horizontalHeader();
    cab->setMinimumSectionSize(38);
    for (int c = 0; c < m_tabla->columnCount(); ++c) {
        cab->setSectionResizeMode(c, c == 1 ? QHeaderView::Stretch : QHeaderView::Interactive);
        if (c != 1) m_tabla->setColumnWidth(c, anchos[c]);
    }
    cab->setHighlightSections(false);
    cab->setFixedHeight(28);
    m_tabla->setMinimumHeight(170);
    root->addWidget(m_tabla, 1);

    connect(m_tabla, &QTableWidget::itemDoubleClicked, this, [this](QTableWidgetItem *it) {
        QTableWidgetItem *celdaPid = m_tabla->item(it->row(), 0);
        if (celdaPid) emit verDetalle(celdaPid->data(Qt::UserRole).toInt());
    });

    // ---------- Pie ----------
    auto *pie = new QHBoxLayout();
    pie->setSpacing(10);
    m_barraEstado = new QLabel(QStringLiteral("sin procesos"));
    m_barraEstado->setStyleSheet(QString("color:%1; font-family:%2; font-size:10.5px;")
                                     .arg(Tema::Neutro600).arg(Tema::FamiliaMono));
    pie->addWidget(m_barraEstado);
    pie->addStretch();

    auto *pista = new QLabel(QStringLiteral("doble clic en una fila = ver PCB"));
    pista->setObjectName("sub");
    pie->addWidget(pista);

    m_btnFinalizar = new QPushButton(QStringLiteral("Finalizar proceso"));
    m_btnFinalizar->setObjectName("danger");
    m_btnFinalizar->setCursor(Qt::PointingHandCursor);
    connect(m_btnFinalizar, &QPushButton::clicked, this, [this] {
        int pid = pidSeleccionado();
        if (pid > 0) emit solicitudTerminar(pid);
    });
    pie->addWidget(m_btnFinalizar);
    root->addLayout(pie);
}

QWidget* TaskManagerWidget::crearMetrica(const QString &titulo,
                                         QLabel *&valorSalida, QLabel *&subSalida) {
    auto *frame = new QFrame();
    frame->setStyleSheet(QString("QFrame { background:%1; border:1px solid %2;"
                                 " border-radius:8px; }"
                                 "QLabel { border:none; background:transparent; }")
                             .arg(Tema::Panel).arg(Tema::Borde));
    auto *lay = new QVBoxLayout(frame);
    lay->setContentsMargins(12, 9, 12, 10);
    lay->setSpacing(4);

    auto *tit = new QLabel(titulo);
    tit->setObjectName("metricCap");
    valorSalida = new QLabel(QStringLiteral("—"));
    valorSalida->setObjectName("metric");
    subSalida = new QLabel(QStringLiteral(" "));
    subSalida->setObjectName("sub");
    subSalida->setWordWrap(true);

    lay->addWidget(tit);
    lay->addWidget(valorSalida);
    lay->addWidget(subSalida);
    return frame;
}

int TaskManagerWidget::pidSeleccionado() const {
    const auto seleccion = m_tabla->selectedItems();
    if (seleccion.isEmpty()) return -1;
    QTableWidgetItem *celdaPid = m_tabla->item(seleccion.first()->row(), 0);
    return celdaPid ? celdaPid->data(Qt::UserRole).toInt() : -1;
}

void TaskManagerWidget::muestrear(const QVector<Proceso> &procesos, double cpuGlobal) {
    const double usada = memoriaUsadaMB(procesos);
    m_graficaCpu->agregarMuestra(cpuGlobal);
    m_graficaMem->agregarMuestra(100.0 * usada / MEMORIA_TOTAL_MB);
}

void TaskManagerWidget::actualizar(const QVector<Proceso> &procesos, int reloj, double cpuGlobal) {
    // ---------- Metricas globales ----------
    int vivos = 0, terminados = 0, ctxTotal = 0, esTotal = 0, hilosTotal = 0;
    int esperaTotal = 0, retornoTotal = 0, conRetorno = 0;
    QString enCpu = QStringLiteral("CPU inactiva");
    for (const auto &p : procesos) {
        if (p.estado == ProcState::Terminado) terminados++;
        else { vivos++; hilosTotal += p.hilos; }
        ctxTotal += p.cambiosContexto;
        esTotal  += p.operacionesES;
        esperaTotal += p.ticksEspera;
        if (tiempoRetorno(p) >= 0) { retornoTotal += tiempoRetorno(p); conRetorno++; }
        if (p.estado == ProcState::Ejecutando)
            enCpu = QString("en CPU: %1 · PID %2").arg(p.nombre).arg(p.pid);
    }
    const double usadaMB = memoriaUsadaMB(procesos);
    const double memPct  = 100.0 * usadaMB / MEMORIA_TOTAL_MB;
    const double esperaMedia = procesos.isEmpty() ? 0.0 : double(esperaTotal) / procesos.size();

    m_valCpu->setText(QString::number(cpuGlobal, 'f', 0) + " %");
    m_subCpu->setText(enCpu);
    m_valMem->setText(QString("%1 GB").arg(usadaMB / 1024.0, 0, 'f', 2));
    m_subMem->setText(QString("%1 % de %2 GB · %3 hilos")
                          .arg(memPct, 0, 'f', 0)
                          .arg(MEMORIA_TOTAL_MB / 1024.0, 0, 'f', 0)
                          .arg(hilosTotal));
    m_valProc->setText(QString::number(procesos.size()));
    m_subProc->setText(QString("%1 activos · %2 terminados").arg(vivos).arg(terminados));
    m_valEspera->setText(QString("%1 t").arg(esperaMedia, 0, 'f', 1));
    m_subEspera->setText(conRetorno > 0
        ? QString("retorno medio %1 t · %2 ctx")
              .arg(double(retornoTotal) / conRetorno, 0, 'f', 1).arg(ctxTotal)
        : QString("%1 cambios de contexto · %2 op. E/S").arg(ctxTotal).arg(esTotal));

    m_graficaCpu->setSubtitulo(QString("ventana de %1 ticks").arg(VENTANA_CPU));
    m_graficaMem->setSubtitulo(QString("%1 de %2")
                                   .arg(formatoMemoria(usadaMB), formatoMemoria(MEMORIA_TOTAL_MB)));
    m_barraMemoria->actualizar(procesos);

    m_barraEstado->setText(QString("t%1  ·  %2 procesos  ·  CPU %3 %  ·  memoria %4 %  ·  %5 op. E/S")
                               .arg(reloj)
                               .arg(procesos.size())
                               .arg(cpuGlobal, 0, 'f', 0)
                               .arg(memPct, 0, 'f', 0)
                               .arg(esTotal));

    // ---------- Tabla ----------
    const int pidPrevio = pidSeleccionado();
    const int columnaOrden = m_tabla->horizontalHeader()->sortIndicatorSection();
    const Qt::SortOrder ordenPrevio = m_tabla->horizontalHeader()->sortIndicatorOrder();

    m_tabla->setSortingEnabled(false);
    m_tabla->setRowCount(procesos.size());

    int fila = 0;
    for (const auto &p : procesos) {
        const double cpu = cpuPorcentaje(p);
        const bool terminado = (p.estado == ProcState::Terminado);
        const double memProc = terminado ? 0.0 : p.memoriaMB;

        auto poner = [&](int col, QTableWidgetItem *item,
                         Qt::Alignment align = Qt::AlignRight | Qt::AlignVCenter) {
            item->setTextAlignment(align);
            item->setForeground(QColor(Tema::Neutro300));
            m_tabla->setItem(fila, col, item);
        };

        poner(0, new ItemNumerico(QString::number(p.pid), p.pid));

        auto *nombre = new QTableWidgetItem("  " + p.nombre);
        nombre->setData(Qt::UserRole, p.pid);
        nombre->setForeground(QColor(Tema::Texto));
        nombre->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_tabla->setItem(fila, 1, nombre);

        auto *estado = new QTableWidgetItem(estadoATexto(p.estado));
        estado->setForeground(Tema::colorEstado(p.estado));
        estado->setBackground(Tema::tinte(Tema::colorEstadoHex(p.estado), 26));
        estado->setTextAlignment(Qt::AlignCenter);
        m_tabla->setItem(fila, 2, estado);

        auto *itCpu = new ItemNumerico(terminado ? QStringLiteral("—")
                                                 : QString::number(cpu, 'f', 0) + " %", cpu);
        itCpu->setBackground(sombreado(Tema::EstEjecutando, cpu / 100.0));
        poner(3, itCpu);

        auto *itMem = new ItemNumerico(terminado ? QStringLiteral("—") : formatoMemoria(memProc), memProc);
        itMem->setBackground(sombreado(Tema::Acento, memProc / 800.0));
        poner(4, itMem);

        poner(5, new ItemNumerico(QString("%1/%2").arg(p.ticksCpu).arg(p.rafagaTotal), p.ticksCpu));
        poner(6, new ItemNumerico(QString::number(p.ticksEspera),     p.ticksEspera));
        poner(7, new ItemNumerico(QString::number(p.ticksBloqueado),  p.ticksBloqueado));
        poner(8, new ItemNumerico(QString::number(p.operacionesES),   p.operacionesES));
        poner(9, new ItemNumerico(QString::number(p.cambiosContexto), p.cambiosContexto));
        poner(10, new ItemNumerico(QString::number(p.hilos),          p.hilos));
        poner(11, new ItemNumerico(QString::number(p.prioridad),      p.prioridad));

        if (terminado)
            for (int c = 0; c < m_tabla->columnCount(); ++c)
                if (auto *it = m_tabla->item(fila, c))
                    if (c != 2) it->setForeground(QColor(Tema::Neutro700));

        fila++;
    }

    m_tabla->setSortingEnabled(true);
    m_tabla->sortByColumn(columnaOrden < 0 ? 3 : columnaOrden, ordenPrevio);

    // Restaurar la fila seleccionada (el usuario puede tener una marcada)
    if (pidPrevio > 0) {
        for (int r = 0; r < m_tabla->rowCount(); ++r) {
            QTableWidgetItem *celda = m_tabla->item(r, 0);
            if (celda && celda->data(Qt::UserRole).toInt() == pidPrevio) {
                m_tabla->selectRow(r);
                break;
            }
        }
    }
}

void TaskManagerWidget::limpiar() {
    m_tabla->setRowCount(0);
    m_graficaCpu->limpiar();
    m_graficaMem->limpiar();
    m_barraMemoria->actualizar({});
    m_valCpu->setText(QStringLiteral("0 %"));   m_subCpu->setText(QStringLiteral("CPU inactiva"));
    m_valMem->setText(QStringLiteral("—"));     m_subMem->setText(QStringLiteral(" "));
    m_valProc->setText(QStringLiteral("0"));    m_subProc->setText(QStringLiteral("0 activos · 0 terminados"));
    m_valEspera->setText(QStringLiteral("0 t")); m_subEspera->setText(QStringLiteral("sin cambios de contexto"));
    m_barraEstado->setText(QStringLiteral("sin procesos"));
}
