#include "mainwindow.h"
#include "theme.h"
#include "statediagramwidget.h"
#include "pcbcardwidget.h"
#include "taskmanagerwidget.h"
#include "cpustripwidget.h"
#include "ganttwidget.h"
#include "memorymodel.h"

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSlider>
#include <QPushButton>
#include <QToolButton>
#include <QMenu>
#include <QAction>
#include <QSpinBox>
#include <QComboBox>
#include <QInputDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QScrollArea>
#include <QDialog>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QFrame>
#include <QApplication>
#include <QTabWidget>
#include <QTimer>
#include <QMap>
#include <functional>

// ============================================================
//  Utilidades locales de presentacion
// ============================================================
namespace {

QLabel *rotulo(const QString &texto) {
    auto *l = new QLabel(texto);
    l->setObjectName("sectionLabel");
    return l;
}

QFrame *separadorHorizontal() {
    auto *l = new QFrame();
    l->setFixedHeight(1);
    l->setStyleSheet(QString("background:%1; border:none;").arg(Tema::BordeSuave));
    return l;
}

QString relojTexto(int t) { return QString("t%1").arg(t); }

// Boton de una entrada del menu, alineado a la izquierda como un item de lista.
QPushButton *botonAccion(const QString &texto) {
    auto *b = new QPushButton(texto);
    b->setCursor(Qt::PointingHandCursor);
    b->setStyleSheet(QStringLiteral("text-align:left; padding:9px 12px;"));
    return b;
}

// Nota didactica de cada columna: explica que significa la cola.
QString notaDeEstado(ProcState s) {
    switch (s) {
        case ProcState::Nuevo:     return "admitidos por el planificador a largo plazo";
        case ProcState::Listo:     return "esperan CPU · se ordenan por prioridad";
        case ProcState::Bloqueado: return "esperan que termine una operacion de E/S";
        case ProcState::Terminado: return "el SO libero su memoria; el PCB se conserva";
        default:                   return QString();
    }
}

} // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("Simulador de Procesos — (Proyecto 1)"));
    resize(1180, 780);
    inicializarMemoria();
    construirUI();

    // Dos procesos de arranque para que la demo no inicie vacia
    m_nombreEdit->setText(QStringLiteral("init"));
    m_prioridadSlider->setValue(9);
    m_rafagaSlider->setValue(3);
    crearProceso();

    m_nombreEdit->setText(QStringLiteral("shell_usuario"));
    m_prioridadSlider->setValue(5);
    m_rafagaSlider->setValue(6);
    crearProceso();
}

// ============================================================
//  Construccion de la interfaz
// ============================================================
void MainWindow::construirUI() {
    qApp->setStyleSheet(Tema::hojaDeEstilos());

    auto *central = new QWidget();
    setCentralWidget(central);
    auto *raiz = new QVBoxLayout(central);
    raiz->setContentsMargins(0, 0, 0, 0);
    raiz->setSpacing(0);

    raiz->addWidget(construirEncabezado());
    raiz->addWidget(separadorHorizontal());

    auto *cuerpo = new QWidget();
    auto *layCuerpo = new QHBoxLayout(cuerpo);
    layCuerpo->setContentsMargins(0, 0, 0, 0);
    layCuerpo->setSpacing(0);
    raiz->addWidget(cuerpo, 1);

    layCuerpo->addWidget(construirLateral());

    // --- Area principal: pestanas + bitacora ---
    auto *area = new QWidget();
    auto *layArea = new QVBoxLayout(area);
    layArea->setContentsMargins(16, 14, 16, 14);
    layArea->setSpacing(12);

    auto *pestanasPrincipales = new QTabWidget();
    pestanasPrincipales->addTab(construirPestanaGestionProcesos(), QStringLiteral("Gestion de procesos"));
    pestanasPrincipales->addTab(construirPestanaMemoria(), QStringLiteral("Administracion de memoria"));
    pestanasPrincipales->addTab(construirPestanaRecursos(), QStringLiteral("Administracion de recursos"));
    pestanasPrincipales->addTab(construirPestanaExtra(), QStringLiteral("Paginacion y Sistema"));
    layArea->addWidget(pestanasPrincipales, 1);

    layArea->addWidget(construirBitacora());
    layCuerpo->addWidget(area, 1);
}

// ------------------------------------------------------------
//  Barra superior: identidad, reloj y control del tiempo
// ------------------------------------------------------------
QWidget* MainWindow::construirEncabezado() {
    auto *header = new QWidget();
    header->setObjectName("header");
    // El selector QWidget#header limita la regla a este contenedor; sin el,
    // la hoja se heredaria a los botones hijos y pisaria su estilo.
    header->setStyleSheet(QString("QWidget#header { background:%1; }").arg(Tema::Panel));
    auto *lay = new QHBoxLayout(header);
    lay->setContentsMargins(18, 12, 16, 12);
    lay->setSpacing(14);

    auto *marca = new QVBoxLayout();
    marca->setSpacing(2);
    auto *titulo = new QLabel(QStringLiteral("Simulador de Procesos"));
    titulo->setObjectName("brand");
    auto *sub = new QLabel(QStringLiteral("Proyecto 1 · PCB · Estados · Colas"));
    sub->setObjectName("sub");
    marca->addWidget(titulo);
    marca->addWidget(sub);
    lay->addLayout(marca);
    lay->addStretch();

    // Reloj: la unidad de tiempo de la simulacion
    m_relojLabel = new QLabel(relojTexto(0));
    m_relojLabel->setStyleSheet(QString(
        "font-family:%1; font-size:13px; font-weight:600; color:%2;"
        "background:%3; border:1px solid %4; border-radius:8px; padding:7px 13px;")
        .arg(Tema::FamiliaMono).arg(Tema::Acento300)
        .arg(Tema::Superficie).arg(Tema::Borde));
    lay->addWidget(m_relojLabel);

    auto *btnTick = new QPushButton(QStringLiteral("Avanzar tick"));
    btnTick->setObjectName("primary");
    btnTick->setCursor(Qt::PointingHandCursor);
    m_btnGantt = new QPushButton(QStringLiteral("Linea de tiempo"));
    m_btnGantt->setCursor(Qt::PointingHandCursor);
    auto *btnReset = new QPushButton(QStringLiteral("Reiniciar"));
    btnReset->setCursor(Qt::PointingHandCursor);

    connect(btnTick, &QPushButton::clicked, this, &MainWindow::avanzarTick);
    connect(btnReset, &QPushButton::clicked, this, &MainWindow::reiniciarSimulacion);
    connect(m_btnGantt, &QPushButton::clicked, this, [this] {
        const bool visible = !m_panelGantt->isVisible();
        m_panelGantt->setVisible(visible);
        m_btnGantt->setText(visible ? QStringLiteral("Ocultar linea de tiempo")
                                    : QStringLiteral("Linea de tiempo"));

        // La linea de tiempo vive dos niveles adentro de pestanas anidadas
        // (Gestion de procesos -> Colas de procesos). Al ocultarla ahi, Qt a
        // veces deja el cache de geometria de TODA la ventana desactualizado
        // -incluida la bitacora, que ni siquiera es hija de ese arbol- y solo
        // se repinta bien tras un resize real (por eso "arreglaba" minimizar
        // y volver a pantalla completa). Forzamos aqui el mismo recalculo
        // que dispara un resize, sin necesidad de que el usuario mueva la
        // ventana.
        if (auto *cw = centralWidget()) {
            if (auto *l = cw->layout()) {
                l->invalidate();
                l->activate();
            }
            cw->updateGeometry();
            cw->update();
        }
    });

    lay->addWidget(btnTick);
    lay->addWidget(m_btnGantt);
    lay->addWidget(btnReset);
    return header;
}

// ------------------------------------------------------------
//  Lateral: creacion de procesos y diagrama de estados
// ------------------------------------------------------------
QWidget* MainWindow::construirLateral() {
    auto *lateral = new QWidget();
    lateral->setObjectName("lateral");
    lateral->setFixedWidth(266);
    lateral->setStyleSheet(QString("QWidget#lateral { background:%1;"
                                   " border-right:1px solid %2; }")
                               .arg(Tema::Fondo).arg(Tema::Borde));
    auto *lay = new QVBoxLayout(lateral);
    lay->setContentsMargins(16, 16, 16, 16);
    lay->setSpacing(9);

    lay->addWidget(rotulo(QStringLiteral("NUEVO PROCESO")));

    m_nombreEdit = new QLineEdit();
    m_nombreEdit->setPlaceholderText(QStringLiteral("compilador_gcc"));
    lay->addWidget(m_nombreEdit);

    auto deslizador = [&](const QString &etiqueta, const QString &pista,
                          int minimo, int maximo, int inicial,
                          QSlider *&salida, QLabel *&valorSalida) {
        auto *fila = new QHBoxLayout();
        auto *lbl = new QLabel(etiqueta);
        lbl->setStyleSheet(QString("font-size:11.5px; color:%1;").arg(Tema::Neutro500));
        valorSalida = new QLabel(QString::number(inicial));
        valorSalida->setStyleSheet(QString("font-family:%1; font-size:11.5px; font-weight:600;"
                                           "color:%2;")
                                       .arg(Tema::FamiliaMono).arg(Tema::Acento300));
        auto *nota = new QLabel(pista);
        nota->setStyleSheet(QString("font-family:%1; font-size:10px; color:%2;")
                                .arg(Tema::FamiliaMono).arg(Tema::Neutro700));
        fila->addWidget(lbl);
        fila->addStretch();
        fila->addWidget(valorSalida);
        fila->addWidget(nota);
        lay->addLayout(fila);

        salida = new QSlider(Qt::Horizontal);
        salida->setRange(minimo, maximo);
        salida->setValue(inicial);
        connect(salida, &QSlider::valueChanged, this,
                [valorSalida](int v) { valorSalida->setText(QString::number(v)); });
        lay->addWidget(salida);
    };

    deslizador(QStringLiteral("Prioridad"), QStringLiteral("9 = alta"),
               0, 9, 3, m_prioridadSlider, m_prioridadVal);
    deslizador(QStringLiteral("Rafaga de CPU"), QStringLiteral("ticks"),
               1, 12, 5, m_rafagaSlider, m_rafagaVal);

    auto *filaMem = new QHBoxLayout();
    auto *lblMem = new QLabel(QStringLiteral("Memoria requerida"));
    lblMem->setStyleSheet(QString("font-size:11.5px; color:%1;").arg(Tema::Neutro500));
    filaMem->addWidget(lblMem);
    lay->addLayout(filaMem);

    m_memoriaNuevoSpin = new QSpinBox();
    m_memoriaNuevoSpin->setRange(16, 65536);
    m_memoriaNuevoSpin->setSingleStep(16);
    m_memoriaNuevoSpin->setSuffix(QStringLiteral(" MB"));
    m_memoriaNuevoSpin->setValue(QRandomGenerator::global()->bounded(64, 512));
    lay->addWidget(m_memoriaNuevoSpin);

    auto *btnCrear = new QPushButton(QStringLiteral("Crear proceso"));
    btnCrear->setObjectName("primary");
    btnCrear->setCursor(Qt::PointingHandCursor);
    connect(btnCrear, &QPushButton::clicked, this, &MainWindow::crearProceso);
    lay->addWidget(btnCrear);

    lay->addSpacing(8);
    lay->addWidget(separadorHorizontal());
    lay->addSpacing(8);

    auto *cabDiag = new QHBoxLayout();
    cabDiag->addWidget(rotulo(QStringLiteral("CICLO DE VIDA")));
    cabDiag->addStretch();
    lay->addLayout(cabDiag);

    m_diagrama = new StateDiagramWidget();
    lay->addWidget(m_diagrama);

    lay->addStretch();
    return lateral;
}

// ------------------------------------------------------------
//  Panel generico de acciones: una columna de botones numerados,
//  igual a las entradas del menu principal (se reusan los mismos
//  slots, asi que ambos caminos hacen exactamente lo mismo).
// ------------------------------------------------------------
QWidget* MainWindow::construirPanelAcciones(const QString &titulo,
        const QVector<QPair<QString, std::function<void()>>> &acciones) {
    auto *panel = new QFrame();
    panel->setFixedWidth(226);
    panel->setStyleSheet(QString("QFrame { background:%1; border:1px solid %2; border-radius:8px; }")
                             .arg(Tema::Panel).arg(Tema::Borde));
    auto *lay = new QVBoxLayout(panel);
    lay->setContentsMargins(14, 14, 14, 14);
    lay->setSpacing(7);
    lay->addWidget(rotulo(titulo));
    lay->addSpacing(4);
    for (const auto &accion : acciones) {
        auto *b = botonAccion(accion.first);
        connect(b, &QPushButton::clicked, this, accion.second);
        lay->addWidget(b);
    }
    lay->addStretch();
    return panel;
}

// ------------------------------------------------------------
//  Pestana 1: GESTION DE PROCESOS — los 6 puntos del menu a la
//  izquierda, y a la derecha las vistas que ya teniamos (colas y
//  rendimiento), que son exactamente lo que abren "Mostrar procesos"
//  y "Mostrar colas".
// ------------------------------------------------------------
QWidget* MainWindow::construirPestanaGestionProcesos() {
    auto *host = new QWidget();
    auto *lay = new QHBoxLayout(host);
    lay->setContentsMargins(14, 14, 14, 14);
    lay->setSpacing(14);

    lay->addWidget(construirPanelAcciones(QStringLiteral("GESTION DE PROCESOS"), {
        {QStringLiteral("1. Crear proceso"),     [this]{ crearProceso(); }},
        {QStringLiteral("2. Mostrar procesos"),  [this]{ menuMostrarProcesos(); }},
        {QStringLiteral("3. Mostrar PCB"),       [this]{ menuMostrarPcb(); }},
        {QStringLiteral("4. Cambiar estado"),    [this]{ menuCambiarEstado(); }},
        {QStringLiteral("5. Mostrar colas"),     [this]{ menuMostrarColas(); }},
        {QStringLiteral("6. Finalizar proceso"), [this]{ menuFinalizarProceso(); }},
    }));

    auto *pestanasVistas = new QTabWidget();
    pestanasVistas->addTab(construirPestanaColas(), QStringLiteral("Colas de procesos"));

    m_taskManager = new TaskManagerWidget();
    connect(m_taskManager, &TaskManagerWidget::verDetalle, this, &MainWindow::mostrarDetallePcb);
    connect(m_taskManager, &TaskManagerWidget::solicitudTerminar, this, &MainWindow::terminar);
    pestanasVistas->addTab(m_taskManager, QStringLiteral("Rendimiento"));
    m_pestanas = pestanasVistas;

    lay->addWidget(pestanasVistas, 1);
    return host;
}

// ------------------------------------------------------------
//  Pestana 2: ADMINISTRACION DE MEMORIA (puntos 7 a 12 del menu)
// ------------------------------------------------------------
// Estilo comun de las tarjetas de esta pestana (mismo look que el resto
// de paneles de la app: fondo de panel, borde suave, esquinas redondeadas).
namespace {
QFrame *marcoMemoria() {
    auto *f = new QFrame();
    f->setStyleSheet(QString("QFrame { background:%1; border:1px solid %2; border-radius:8px; }")
                         .arg(Tema::Panel).arg(Tema::Borde));
    return f;
}
QLabel *notaMemoria(const QString &texto) {
    auto *l = new QLabel(texto);
    l->setWordWrap(true);
    l->setStyleSheet(QString("font-size:10.5px; color:%1;").arg(Tema::Neutro600));
    return l;
}
} // namespace

QWidget* MainWindow::construirPestanaMemoria() {
    auto *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);

    auto *host = new QWidget();
    auto *lay = new QVBoxLayout(host);
    lay->setContentsMargins(14, 14, 14, 14);
    lay->setSpacing(14);

    // --- Configuracion del espacio de memoria fisica (total / SO) ---
    auto *panelConfig = marcoMemoria();
    auto *layConfig = new QVBoxLayout(panelConfig);
    layConfig->setContentsMargins(14, 12, 14, 12);
    layConfig->setSpacing(8);
    layConfig->addWidget(rotulo(QStringLiteral("CONFIGURACION DEL ESPACIO DE MEMORIA")));

    auto *filaConfig = new QHBoxLayout();
    filaConfig->setSpacing(10);

    auto campoSpin = [&](const QString &etiqueta, int minimo, int maximo, int valor, QSpinBox *&salida) {
        auto *col = new QVBoxLayout();
        col->setSpacing(3);
        auto *lbl = new QLabel(etiqueta);
        lbl->setStyleSheet(QString("font-size:11px; color:%1;").arg(Tema::Neutro500));
        salida = new QSpinBox();
        salida->setRange(minimo, maximo);
        salida->setSingleStep(64);
        salida->setSuffix(QStringLiteral(" MB"));
        salida->setValue(valor);
        col->addWidget(lbl);
        col->addWidget(salida);
        filaConfig->addLayout(col);
    };

    campoSpin(QStringLiteral("Memoria total"), 256, 65536, int(m_memoriaTotalMB), m_totalMemSpin);
    campoSpin(QStringLiteral("Reservada para el SO"), 0, 65536, int(m_memoriaSoMB), m_soMemSpin);

    auto *btnAplicar = new QPushButton(QStringLiteral("Aplicar"));
    btnAplicar->setObjectName("primary");
    btnAplicar->setCursor(Qt::PointingHandCursor);
    connect(btnAplicar, &QPushButton::clicked, this, &MainWindow::memAplicarConfiguracion);
    filaConfig->addWidget(btnAplicar, 0, Qt::AlignBottom);
    filaConfig->addStretch();
    layConfig->addLayout(filaConfig);
    layConfig->addWidget(notaMemoria(QStringLiteral(
        "Modifica el tamano total de la memoria fisica o el bloque reservado por el sistema "
        "operativo. Los procesos que ya tienen memoria asignada conservan su bloque; si no "
        "alcanza, el cambio se rechaza.")));
    lay->addWidget(panelConfig);

    // --- 7. Mostrar mapa de memoria ---
    auto *panelMapa = marcoMemoria();
    auto *layMapa = new QVBoxLayout(panelMapa);
    layMapa->setContentsMargins(14, 12, 14, 12);
    layMapa->setSpacing(8);

    auto *cabMapa = new QHBoxLayout();
    cabMapa->addWidget(rotulo(QStringLiteral("7. MOSTRAR MAPA DE MEMORIA")));
    cabMapa->addStretch();
    m_mapaMemoriaTotalLabel = new QLabel();
    m_mapaMemoriaTotalLabel->setStyleSheet(QString("font-family:%1; font-size:11px; font-weight:600; color:%2;")
                                               .arg(Tema::FamiliaMono).arg(Tema::Acento300));
    cabMapa->addWidget(m_mapaMemoriaTotalLabel);
    layMapa->addLayout(cabMapa);

    auto *mapaHost = new QWidget();
    m_mapaMemoriaLayout = new QVBoxLayout(mapaHost);
    m_mapaMemoriaLayout->setContentsMargins(0, 0, 0, 0);
    m_mapaMemoriaLayout->setSpacing(4);
    layMapa->addWidget(mapaHost);
    lay->addWidget(panelMapa);

    // --- Dos columnas: 8/9 a la izquierda, 10/11/12 a la derecha ---
    auto *dosColumnas = new QHBoxLayout();
    dosColumnas->setSpacing(14);

    // 8. Asignar memoria
    auto *panel8 = marcoMemoria();
    auto *lay8 = new QVBoxLayout(panel8);
    lay8->setContentsMargins(14, 12, 14, 12);
    lay8->setSpacing(6);
    lay8->addWidget(rotulo(QStringLiteral("8. ASIGNAR MEMORIA")));
    lay8->addWidget(notaMemoria(QStringLiteral(
        "Al crear un proceso se solicita su memoria de inmediato. Si existe un hueco contiguo "
        "suficiente (segun el metodo de ajuste elegido) se le asigna y puede pasar a LISTO. Si "
        "no existe, el proceso permanece en NUEVO sin memoria asignada, hasta que se libere o "
        "compacte memoria y vuelva a intentarse.")));
    auto *btn8 = new QPushButton(QStringLiteral("Reintentar asignacion pendiente"));
    btn8->setCursor(Qt::PointingHandCursor);
    connect(btn8, &QPushButton::clicked, this, &MainWindow::memAsignarManual);
    lay8->addWidget(btn8);

    // 9. Liberar memoria
    auto *panel9 = marcoMemoria();
    auto *lay9 = new QVBoxLayout(panel9);
    lay9->setContentsMargins(14, 12, 14, 12);
    lay9->setSpacing(6);
    lay9->addWidget(rotulo(QStringLiteral("9. LIBERAR MEMORIA")));
    lay9->addWidget(notaMemoria(QStringLiteral(
        "Cuando un proceso finaliza, su bloque se libera de inmediato y aparece como un hueco "
        "nuevo en el mapa (los huecos libres adyacentes se fusionan automaticamente).")));
    auto *btn9 = new QPushButton(QStringLiteral("Finalizar un proceso (libera su bloque)"));
    btn9->setCursor(Qt::PointingHandCursor);
    connect(btn9, &QPushButton::clicked, this, &MainWindow::memLiberarManual);
    lay9->addWidget(btn9);

    auto *colIzq = new QVBoxLayout();
    colIzq->setSpacing(14);
    colIzq->addWidget(panel8);
    colIzq->addWidget(panel9);
    colIzq->addStretch();
    dosColumnas->addLayout(colIzq, 1);

    // 10. Mostrar fragmentacion
    auto *panel10 = marcoMemoria();
    auto *lay10 = new QVBoxLayout(panel10);
    lay10->setContentsMargins(14, 12, 14, 12);
    lay10->setSpacing(6);
    lay10->addWidget(rotulo(QStringLiteral("10. MOSTRAR FRAGMENTACION")));
    m_fragmentacionInfo = new QLabel();
    m_fragmentacionInfo->setWordWrap(true);
    m_fragmentacionInfo->setStyleSheet(QString("font-family:%1; font-size:10.5px; color:%2;")
                                           .arg(Tema::FamiliaMono).arg(Tema::Neutro500));
    lay10->addWidget(m_fragmentacionInfo);
    auto *btn10 = new QPushButton(QStringLiteral("Probar una asignacion hipotetica"));
    btn10->setCursor(Qt::PointingHandCursor);
    connect(btn10, &QPushButton::clicked, this, &MainWindow::memProbarFragmentacion);
    lay10->addWidget(btn10);

    // 11. Compactar memoria
    auto *panel11 = marcoMemoria();
    auto *lay11 = new QVBoxLayout(panel11);
    lay11->setContentsMargins(14, 12, 14, 12);
    lay11->setSpacing(6);
    lay11->addWidget(rotulo(QStringLiteral("11. COMPACTAR MEMORIA")));
    lay11->addWidget(notaMemoria(QStringLiteral(
        "Junta todos los bloques ocupados al inicio de la memoria y deja un unico hueco libre "
        "al final, eliminando la fragmentacion externa actual.")));
    auto *btn11 = new QPushButton(QStringLiteral("Compactar"));
    btn11->setObjectName("primary");
    btn11->setCursor(Qt::PointingHandCursor);
    connect(btn11, &QPushButton::clicked, this, &MainWindow::memCompactar);
    lay11->addWidget(btn11);

    // 12. Cambiar metodo de ajuste
    auto *panel12 = marcoMemoria();
    auto *lay12 = new QVBoxLayout(panel12);
    lay12->setContentsMargins(14, 12, 14, 12);
    lay12->setSpacing(6);
    lay12->addWidget(rotulo(QStringLiteral("12. CAMBIAR METODO DE AJUSTE")));
    m_metodoCombo = new QComboBox();
    m_metodoCombo->addItem(QStringLiteral("1. First Fit"));
    m_metodoCombo->addItem(QStringLiteral("2. Best Fit"));
    m_metodoCombo->addItem(QStringLiteral("3. Worst Fit"));
    m_metodoCombo->setCurrentIndex(static_cast<int>(m_metodoAjuste));
    // QOverload<int> hace que compile igual en Qt 5 (donde la señal tiene dos
    // versiones: int y QString) y en Qt 6.
    connect(m_metodoCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        m_metodoAjuste = static_cast<MetodoAjuste>(idx);
        registrarEvento(QString("Metodo de ajuste cambiado a %1").arg(metodoATexto(m_metodoAjuste)),
                         TipoEvento::Sistema);
    });
    lay12->addWidget(m_metodoCombo);
    lay12->addWidget(notaMemoria(QStringLiteral(
        "Cada asignacion queda registrada en la bitacora con el proceso, la cantidad "
        "solicitada, los huecos disponibles, el bloque elegido y el espacio restante.")));

    auto *colDer = new QVBoxLayout();
    colDer->setSpacing(14);
    colDer->addWidget(panel10);
    colDer->addWidget(panel11);
    colDer->addWidget(panel12);
    colDer->addStretch();
    dosColumnas->addLayout(colDer, 1);

    lay->addLayout(dosColumnas);
    lay->addStretch();

    scroll->setWidget(host);
    refrescarMapaMemoria();
    return scroll;
}

// ------------------------------------------------------------
//  Pestana 3: ADMINISTRACION DE RECURSOS (puntos 13 a 16) + el
//  unico punto de SISTEMA (17), para no necesitar una cuarta pestana.
// ------------------------------------------------------------
QWidget* MainWindow::construirPestanaRecursos() {
    auto *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);

    auto *host = new QWidget();
    auto *lay = new QVBoxLayout(host);
    lay->setContentsMargins(14, 14, 14, 14);
    lay->setSpacing(14);

    // --- 13. Mostrar recursos ---
    auto *panelRec = marcoMemoria();
    auto *layRec = new QVBoxLayout(panelRec);
    layRec->setContentsMargins(14, 12, 14, 12);
    layRec->setSpacing(8);
    layRec->addWidget(rotulo(QStringLiteral("13. MOSTRAR RECURSOS")));

    auto *recursosHost = new QWidget();
    m_recursosLayout = new QVBoxLayout(recursosHost);
    m_recursosLayout->setContentsMargins(0, 0, 0, 0);
    m_recursosLayout->setSpacing(4);
    layRec->addWidget(recursosHost);

    m_recursosResumen = new QLabel();
    m_recursosResumen->setWordWrap(true);
    m_recursosResumen->setStyleSheet(QString("font-family:%1; font-size:10.5px; color:%2;")
                                         .arg(Tema::FamiliaMono).arg(Tema::Neutro500));
    layRec->addWidget(m_recursosResumen);
    lay->addWidget(panelRec);

    // --- Dos columnas: 14/15 a la izquierda, 16 a la derecha ---
    auto *dosColumnas = new QHBoxLayout();
    dosColumnas->setSpacing(14);

    // 14. Asignar recurso
    auto *panel14 = marcoMemoria();
    auto *lay14 = new QVBoxLayout(panel14);
    lay14->setContentsMargins(14, 12, 14, 12);
    lay14->setSpacing(6);
    lay14->addWidget(rotulo(QStringLiteral("14. ASIGNAR RECURSO")));
    lay14->addWidget(notaMemoria(QStringLiteral(
        "Un proceso solicita un recurso. Si esta disponible se le asigna de inmediato. "
        "Si esta ocupado, el proceso queda registrado en espera hasta que quien lo tiene "
        "lo libere (o finalice).")));
    auto *btn14 = new QPushButton(QStringLiteral("Solicitar recurso para un proceso"));
    btn14->setCursor(Qt::PointingHandCursor);
    connect(btn14, &QPushButton::clicked, this, &MainWindow::recAsignarManual);
    lay14->addWidget(btn14);

    // 15. Liberar recurso
    auto *panel15 = marcoMemoria();
    auto *lay15 = new QVBoxLayout(panel15);
    lay15->setContentsMargins(14, 12, 14, 12);
    lay15->setSpacing(6);
    lay15->addWidget(rotulo(QStringLiteral("15. LIBERAR RECURSO")));
    lay15->addWidget(notaMemoria(QStringLiteral(
        "Libera manualmente un recurso que un proceso tiene asignado. Si alguien mas lo "
        "estaba esperando, se le concede de inmediato. Esto tambien ocurre solo cuando "
        "un proceso finaliza.")));
    auto *btn15 = new QPushButton(QStringLiteral("Liberar recurso de un proceso"));
    btn15->setCursor(Qt::PointingHandCursor);
    connect(btn15, &QPushButton::clicked, this, &MainWindow::recLiberarManual);
    lay15->addWidget(btn15);

    auto *colIzq = new QVBoxLayout();
    colIzq->setSpacing(14);
    colIzq->addWidget(panel14);
    colIzq->addWidget(panel15);
    colIzq->addStretch();
    dosColumnas->addLayout(colIzq, 1);

    // 16. Detectar interbloqueo
    auto *panel16 = marcoMemoria();
    auto *lay16 = new QVBoxLayout(panel16);
    lay16->setContentsMargins(14, 12, 14, 12);
    lay16->setSpacing(6);
    lay16->addWidget(rotulo(QStringLiteral("16. DETECTAR INTERBLOQUEO")));
    m_interbloqueoInfo = new QLabel();
    m_interbloqueoInfo->setWordWrap(true);
    m_interbloqueoInfo->setStyleSheet(QString("font-family:%1; font-size:10.5px; color:%2;")
                                          .arg(Tema::FamiliaMono).arg(Tema::Neutro400));
    lay16->addWidget(m_interbloqueoInfo);
    auto *btn16 = new QPushButton(QStringLiteral("Volver a evaluar el grafo de espera"));
    btn16->setCursor(Qt::PointingHandCursor);
    connect(btn16, &QPushButton::clicked, this, &MainWindow::recDetectar);
    lay16->addWidget(btn16);

    auto *colDer = new QVBoxLayout();
    colDer->setSpacing(14);
    colDer->addWidget(panel16);
    colDer->addStretch();
    dosColumnas->addLayout(colDer, 1);

    lay->addLayout(dosColumnas);
    lay->addStretch();

    scroll->setWidget(host);
    refrescarRecursos();
    return scroll;
}

// ------------------------------------------------------------
//  Pestana 4: PAGINACION Y SISTEMA — dos sub-pestanas
// ------------------------------------------------------------
QWidget* MainWindow::construirPestanaExtra() {
    auto *sub = new QTabWidget();
    sub->addTab(construirSubPestanaPaginacion(), QStringLiteral("Paginacion"));
    sub->addTab(construirSubPestanaSistema(), QStringLiteral("Sistema"));
    return sub;
}

// --- Sub-pestana PAGINACION: extension conceptual (requerimiento 13) ---
// No es un administrador de memoria virtual completo: solo demuestra que
// un proceso puede repartirse en paginas iguales que van a marcos que NO
// necesitan ser contiguos, a diferencia del modelo de particiones de la
// pestana "Administracion de memoria".
QWidget* MainWindow::construirSubPestanaPaginacion() {
    auto *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);

    auto *host = new QWidget();
    auto *lay = new QVBoxLayout(host);
    lay->setContentsMargins(14, 14, 14, 14);
    lay->setSpacing(14);

    auto *intro = notaMemoria(QStringLiteral(
        "Extension conceptual: aqui el proceso ya NO necesita un unico bloque "
        "contiguo (como en \"Administracion de memoria\"). Se divide en paginas "
        "de tamano fijo, y cada pagina se ubica en cualquier marco libre de la "
        "memoria fisica, sin importar el orden."));
    lay->addWidget(intro);

    // --- Configuracion: tamano de pagina, tamano del proceso, marcos totales ---
    auto *panelConfig = marcoMemoria();
    auto *layConfig = new QVBoxLayout(panelConfig);
    layConfig->setContentsMargins(14, 12, 14, 12);
    layConfig->setSpacing(8);
    layConfig->addWidget(rotulo(QStringLiteral("DATOS DEL PROCESO A PAGINAR")));

    auto *filaConfig = new QHBoxLayout();
    filaConfig->setSpacing(10);

    auto campoSpin = [&](const QString &etiqueta, int minimo, int maximo, int valor, QSpinBox *&salida) {
        auto *col = new QVBoxLayout();
        col->setSpacing(3);
        auto *lbl = new QLabel(etiqueta);
        lbl->setStyleSheet(QString("font-size:11px; color:%1;").arg(Tema::Neutro500));
        salida = new QSpinBox();
        salida->setRange(minimo, maximo);
        salida->setSuffix(QStringLiteral(" KB"));
        salida->setValue(valor);
        col->addWidget(lbl);
        col->addWidget(salida);
        filaConfig->addLayout(col);
    };

    campoSpin(QStringLiteral("Tamano del proceso"), 1, 1000000, 14, m_procesoKBSpin);
    campoSpin(QStringLiteral("Tamano de pagina (= tamano de marco)"), 1, 65536, 4, m_paginaKBSpin);

    auto *colMarcos = new QVBoxLayout();
    colMarcos->setSpacing(3);
    auto *lblMarcos = new QLabel(QStringLiteral("Marcos totales disponibles"));
    lblMarcos->setStyleSheet(QString("font-size:11px; color:%1;").arg(Tema::Neutro500));
    m_marcosSpin = new QSpinBox();
    m_marcosSpin->setRange(1, 4096);
    m_marcosSpin->setValue(16);
    colMarcos->addWidget(lblMarcos);
    colMarcos->addWidget(m_marcosSpin);
    filaConfig->addLayout(colMarcos);

    auto *btnCalcular = new QPushButton(QStringLiteral("Calcular paginacion"));
    btnCalcular->setObjectName("primary");
    btnCalcular->setCursor(Qt::PointingHandCursor);
    connect(btnCalcular, &QPushButton::clicked, this, &MainWindow::calcularPaginacion);
    filaConfig->addWidget(btnCalcular, 0, Qt::AlignBottom);
    filaConfig->addStretch();
    layConfig->addLayout(filaConfig);
    lay->addWidget(panelConfig);

    // --- Resultado: paginas necesarias + tabla pagina -> marco ---
    auto *panelTabla = marcoMemoria();
    auto *layTabla = new QVBoxLayout(panelTabla);
    layTabla->setContentsMargins(14, 12, 14, 12);
    layTabla->setSpacing(8);
    layTabla->addWidget(rotulo(QStringLiteral("TABLA DE PAGINAS (SIMPLIFICADA)")));

    m_paginacionResumen = new QLabel();
    m_paginacionResumen->setWordWrap(true);
    m_paginacionResumen->setStyleSheet(QString("font-family:%1; font-size:10.5px; color:%2;")
                                           .arg(Tema::FamiliaMono).arg(Tema::Neutro500));
    layTabla->addWidget(m_paginacionResumen);

    auto *tablaHost = new QWidget();
    m_tablaPaginasLayout = new QVBoxLayout(tablaHost);
    m_tablaPaginasLayout->setContentsMargins(0, 0, 0, 0);
    m_tablaPaginasLayout->setSpacing(4);
    layTabla->addWidget(tablaHost);
    lay->addWidget(panelTabla);

    lay->addWidget(notaMemoria(QStringLiteral(
        "¿Por que estos marcos no necesitan ser contiguos? Porque la tabla de "
        "paginas guarda, para cada pagina logica del proceso, en que marco fisico "
        "quedo — el hardware (o aqui, la simulacion) traduce cada direccion "
        "logica a su marco real usando esa tabla. El proceso \"ve\" su memoria "
        "como un bloque continuo de paginas 0..N-1, pero fisicamente puede estar "
        "dispersa en cualquier parte de la RAM. Esto es justo lo opuesto al "
        "problema de la pestana \"Administracion de memoria\": alli un proceso "
        "necesitaba UN hueco contiguo lo bastante grande (y por eso aparecia "
        "fragmentacion externa); con paginacion, cualquier conjunto de marcos "
        "libres alcanza, sin importar si estan dispersos, asi que la "
        "fragmentacion externa deja de ser un problema (aunque puede quedar algo "
        "de fragmentacion interna en la ultima pagina si el proceso no es multiplo "
        "exacto del tamano de pagina).")));

    lay->addStretch();
    scroll->setWidget(host);
    calcularPaginacion();
    return scroll;
}

// Calcula cuantas paginas necesita el proceso configurado y les asigna
// marcos al azar, sin repetir y sin exigir contiguidad, para dejar visible
// que el orden de los marcos no importa en absoluto.
void MainWindow::calcularPaginacion() {
    if (!m_procesoKBSpin || !m_paginaKBSpin || !m_marcosSpin || !m_tablaPaginasLayout) return;

    const int tamProceso = m_procesoKBSpin->value();
    const int tamPagina = m_paginaKBSpin->value();
    const int marcosTotales = m_marcosSpin->value();
    const int paginas = (tamProceso + tamPagina - 1) / tamPagina;  // redondeo hacia arriba

    limpiarLayout(m_tablaPaginasLayout);

    if (paginas > marcosTotales) {
        auto *aviso = notaMemoria(QString(
            "El proceso necesita %1 paginas de %2 KB, pero solo hay %3 marcos "
            "disponibles en esta demo. Sube \"Marcos totales disponibles\" o baja "
            "el tamano del proceso.").arg(paginas).arg(tamPagina).arg(marcosTotales));
        aviso->setStyleSheet(QString("font-size:10.5px; color:%1;").arg(Tema::EstTerminado));
        m_tablaPaginasLayout->addWidget(aviso);
        m_tablaPaginasLayout->addStretch();
        if (m_paginacionResumen)
            m_paginacionResumen->setText(QString(
                "Proceso: %1 KB · Pagina/marco: %2 KB · Paginas necesarias: %3 (no alcanzan los marcos)")
                .arg(tamProceso).arg(tamPagina).arg(paginas));
        return;
    }

    // Elige 'paginas' marcos distintos al azar entre 0..marcosTotales-1: el
    // orden en que salen es justamente la prueba de que no hace falta que
    // sean consecutivos.
    QVector<int> disponibles;
    for (int i = 0; i < marcosTotales; ++i) disponibles.append(i);
    QVector<int> asignados;
    for (int i = 0; i < paginas; ++i) {
        const int idx = QRandomGenerator::global()->bounded(disponibles.size());
        asignados.append(disponibles[idx]);
        disponibles.remove(idx);
    }

    auto *encabezado = new QWidget();
    auto *layEnc = new QHBoxLayout(encabezado);
    layEnc->setContentsMargins(10, 2, 10, 4);
    auto *lblPagE = new QLabel(QStringLiteral("PAGINA"));
    lblPagE->setStyleSheet(QString("font-family:%1; font-size:9.5px; font-weight:600; "
                                   "letter-spacing:1.2px; color:%2;")
                               .arg(Tema::FamiliaMono).arg(Tema::Neutro600));
    auto *lblMarE = new QLabel(QStringLiteral("MARCO"));
    lblMarE->setStyleSheet(lblPagE->styleSheet());
    layEnc->addWidget(lblPagE);
    layEnc->addStretch();
    layEnc->addWidget(lblMarE);
    m_tablaPaginasLayout->addWidget(encabezado);

    for (int i = 0; i < paginas; ++i)
        m_tablaPaginasLayout->addWidget(construirFilaPagina(i, asignados[i]));
    m_tablaPaginasLayout->addStretch();

    if (m_paginacionResumen)
        m_paginacionResumen->setText(QString(
            "Proceso: %1 KB · Tamano de pagina/marco: %2 KB · Paginas necesarias: %3 "
            "de %4 marcos totales")
            .arg(tamProceso).arg(tamPagina).arg(paginas).arg(marcosTotales));

    registrarEvento(QString("Paginacion (demo conceptual): proceso de %1 KB con paginas de "
                            "%2 KB necesita %3 paginas; asignadas a marcos no contiguos")
                        .arg(tamProceso).arg(tamPagina).arg(paginas), TipoEvento::Sistema);
}

// Fila de la tabla de paginas: numero de pagina logica -> marco fisico.
QWidget* MainWindow::construirFilaPagina(int pagina, int marco) {
    auto *fila = new QFrame();
    fila->setStyleSheet(QString(
        "QFrame { background:%1; border:1px solid %2; border-left:3px solid %3; border-radius:3px; }"
        "QLabel { background:transparent; border:none; }")
        .arg(Tema::Superficie).arg(Tema::BordeSuave).arg(Tema::Acento300));
    auto *lay = new QHBoxLayout(fila);
    lay->setContentsMargins(10, 6, 10, 6);

    auto *lblPag = new QLabel(QString::number(pagina));
    lblPag->setStyleSheet(QString("font-family:%1; font-size:11.5px; color:%2;")
                              .arg(Tema::FamiliaMono).arg(Tema::Texto));
    auto *flecha = new QLabel(QStringLiteral("→"));
    flecha->setStyleSheet(QString("color:%1;").arg(Tema::Neutro600));
    auto *lblMar = new QLabel(QString::number(marco));
    lblMar->setStyleSheet(QString("font-family:%1; font-size:11.5px; font-weight:600; color:%2;")
                              .arg(Tema::FamiliaMono).arg(Tema::Acento300));

    lay->addWidget(lblPag);
    lay->addStretch();
    lay->addWidget(flecha);
    lay->addStretch();
    lay->addWidget(lblMar);
    return fila;
}

// --- Sub-pestana SISTEMA: resumen unico (requerimiento 17) ---
QWidget* MainWindow::construirSubPestanaSistema() {
    auto *host = new QWidget();
    auto *lay = new QVBoxLayout(host);
    lay->setContentsMargins(14, 14, 14, 14);
    lay->setSpacing(12);

    auto *panel = marcoMemoria();
    auto *layPanel = new QVBoxLayout(panel);
    layPanel->setContentsMargins(16, 14, 16, 14);
    layPanel->setSpacing(10);

    auto *cab = new QHBoxLayout();
    cab->addWidget(rotulo(QStringLiteral("17. MOSTRAR ESTADO GENERAL")));
    cab->addStretch();
    auto *btnActualizar = new QPushButton(QStringLiteral("Actualizar"));
    btnActualizar->setCursor(Qt::PointingHandCursor);
    connect(btnActualizar, &QPushButton::clicked, this, &MainWindow::refrescarEstadoGeneral);
    cab->addWidget(btnActualizar);
    layPanel->addLayout(cab);

    m_estadoGeneralLabel = new QLabel();
    m_estadoGeneralLabel->setStyleSheet(QString("font-family:%1; font-size:11.5px; color:%2;")
                                            .arg(Tema::FamiliaMono).arg(Tema::Neutro300));
    m_estadoGeneralLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layPanel->addWidget(m_estadoGeneralLabel);
    layPanel->addStretch();

    lay->addWidget(panel, 1);

    refrescarEstadoGeneral();
    return host;
}

// ------------------------------------------------------------
//  Pestana de colas: franja de CPU, columnas y linea de tiempo
// ------------------------------------------------------------
QWidget* MainWindow::construirPestanaColas() {
    auto *host = new QWidget();
    auto *lay = new QVBoxLayout(host);
    lay->setContentsMargins(14, 14, 14, 14);
    lay->setSpacing(12);

    // La CPU es un recurso unico: franja horizontal, no una columna mas.
    m_franjaCpu = new CpuStripWidget();
    connect(m_franjaCpu, &CpuStripWidget::accionDespachar, this, &MainWindow::despachar);
    connect(m_franjaCpu, &CpuStripWidget::accionBloquear, this, &MainWindow::bloquear);
    connect(m_franjaCpu, &CpuStripWidget::accionExpropiar, this, &MainWindow::expropiar);
    connect(m_franjaCpu, &CpuStripWidget::accionTerminar, this, &MainWindow::terminar);
    connect(m_franjaCpu, &CpuStripWidget::verDetalle, this, &MainWindow::mostrarDetallePcb);
    lay->addWidget(m_franjaCpu);

    // --- Cuatro columnas de estado ---
    auto *columnas = new QWidget();
    auto *layCols = new QHBoxLayout(columnas);
    layCols->setContentsMargins(0, 0, 0, 0);
    layCols->setSpacing(10);
    layCols->addWidget(construirColumna(QStringLiteral("NUEVO"), notaDeEstado(ProcState::Nuevo),
                                        ProcState::Nuevo, m_colNuevo, m_countNuevo));
    layCols->addWidget(construirColumna(QStringLiteral("LISTO"), notaDeEstado(ProcState::Listo),
                                        ProcState::Listo, m_colListo, m_countListo));
    layCols->addWidget(construirColumna(QStringLiteral("BLOQUEADO"), notaDeEstado(ProcState::Bloqueado),
                                        ProcState::Bloqueado, m_colBloqueado, m_countBloqueado));
    layCols->addWidget(construirColumna(QStringLiteral("TERMINADO"), notaDeEstado(ProcState::Terminado),
                                        ProcState::Terminado, m_colTerminado, m_countTerminado));

    auto *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setWidget(columnas);
    lay->addWidget(scroll, 1);

    // --- Linea de tiempo (Gantt) al pie ---
    m_panelGantt = new QFrame();
    m_panelGantt->setStyleSheet(QString("QFrame { background:%1; border:1px solid %2;"
                                        " border-radius:8px; }"
                                        "QLabel { border:none; background:transparent; }")
                                    .arg(Tema::Panel).arg(Tema::Borde));
    auto *layGantt = new QVBoxLayout(m_panelGantt);
    layGantt->setContentsMargins(12, 10, 12, 11);
    layGantt->setSpacing(8);

    auto *cabGantt = new QHBoxLayout();
    cabGantt->setSpacing(8);
    cabGantt->addWidget(rotulo(QStringLiteral("LINEA DE TIEMPO")));
    auto *notaGantt = new QLabel(QStringLiteral("un tick por celda · el tono lleno es tiempo de CPU"));
    notaGantt->setObjectName("sub");
    cabGantt->addWidget(notaGantt);
    cabGantt->addStretch();

    // Leyenda de los cinco estados, junto al grafico que la usa.
    for (auto s : {ProcState::Nuevo, ProcState::Listo, ProcState::Ejecutando,
                   ProcState::Bloqueado, ProcState::Terminado}) {
        auto *item = new QLabel(estadoATexto(s).toLower());
        item->setStyleSheet(QString("font-family:%1; font-size:9.5px; color:%2;"
                                    "background:%3; border-radius:3px; padding:3px 6px;")
                                .arg(Tema::FamiliaMono)
                                .arg(Tema::colorEstadoHex(s))
                                .arg(Tema::rgba(Tema::colorEstadoHex(s), 30)));
        cabGantt->addWidget(item);
    }
    layGantt->addLayout(cabGantt);

    m_gantt = new GanttWidget();
    layGantt->addWidget(m_gantt);
    lay->addWidget(m_panelGantt);

    return host;
}

QWidget* MainWindow::construirColumna(const QString &titulo, const QString &nota,
                                      ProcState estado,
                                      QVBoxLayout *&contenedorSalida, QLabel *&contadorSalida) {
    auto *marco = new QFrame();
    marco->setMinimumWidth(196);
    marco->setStyleSheet(QString("QFrame { background:%1; border:1px solid %2;"
                                 " border-radius:8px; }")
                             .arg(Tema::Panel).arg(Tema::Borde));
    auto *outer = new QVBoxLayout(marco);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // Filo de 2px con el color del estado: identifica la columna sin teñirla.
    auto *filo = new QFrame();
    filo->setFixedHeight(2);
    filo->setStyleSheet(QString("background:%1; border:none;"
                                "border-top-left-radius:8px; border-top-right-radius:8px;")
                            .arg(Tema::colorEstadoHex(estado)));
    outer->addWidget(filo);

    auto *cab = new QWidget();
    cab->setStyleSheet("background:transparent; border:none;");
    auto *layCab = new QVBoxLayout(cab);
    layCab->setContentsMargins(11, 9, 11, 9);
    layCab->setSpacing(3);

    auto *fila = new QHBoxLayout();
    auto *lblTitulo = new QLabel(titulo);
    lblTitulo->setStyleSheet(QString("font-family:%1; font-size:10px; font-weight:600;"
                                     "letter-spacing:1.4px; color:%2;")
                                 .arg(Tema::FamiliaMono).arg(Tema::colorEstadoHex(estado)));
    contadorSalida = new QLabel(QStringLiteral("0"));
    contadorSalida->setStyleSheet(QString("font-family:%1; font-size:10.5px; color:%2;")
                                       .arg(Tema::FamiliaMono).arg(Tema::Neutro500));
    fila->addWidget(lblTitulo);
    fila->addStretch();
    fila->addWidget(contadorSalida);
    layCab->addLayout(fila);

    auto *lblNota = new QLabel(nota);
    lblNota->setWordWrap(true);
    lblNota->setStyleSheet(QString("font-size:10px; color:%1;").arg(Tema::Neutro700));
    layCab->addWidget(lblNota);
    outer->addWidget(cab);

    auto *sep = new QFrame();
    sep->setFixedHeight(1);
    sep->setStyleSheet(QString("background:%1; border:none;").arg(Tema::BordeSuave));
    outer->addWidget(sep);

    auto *cuerpo = new QWidget();
    cuerpo->setStyleSheet("background:transparent; border:none;");
    contenedorSalida = new QVBoxLayout(cuerpo);
    contenedorSalida->setContentsMargins(9, 9, 9, 9);
    contenedorSalida->setSpacing(7);
    contenedorSalida->addStretch();
    outer->addWidget(cuerpo, 1);

    return marco;
}

// ------------------------------------------------------------
//  Bitacora con filtros por tipo de evento
// ------------------------------------------------------------
QWidget* MainWindow::construirBitacora() {
    auto *host = new QWidget();
    host->setMinimumHeight(180);   // nunca se comprime a 0 aunque el resto del layout cambie
    auto *lay = new QVBoxLayout(host);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(7);

    auto *cab = new QHBoxLayout();
    cab->setSpacing(8);
    cab->addWidget(rotulo(QStringLiteral("BITACORA DE EVENTOS")));
    cab->addStretch();

    struct { const char *etiqueta; bool todos; TipoEvento tipo; } filtros[] = {
        {"Todos",        true,  TipoEvento::Transicion},
        {"Transiciones", false, TipoEvento::Transicion},
        {"Planificador", false, TipoEvento::Planificador},
        {"Sistema",      false, TipoEvento::Sistema},
    };

    for (const auto &f : filtros) {
        auto *b = new QPushButton(QString::fromUtf8(f.etiqueta));
        b->setObjectName("chip");
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
        b->setChecked(f.todos);
        const bool todos = f.todos;
        const TipoEvento tipo = f.tipo;
        connect(b, &QPushButton::clicked, this, [this, todos, tipo, b] {
            for (QPushButton *otro : m_botonesFiltro) otro->setChecked(otro == b);
            m_filtroTodos = todos;
            m_filtro = tipo;
            repintarBitacora();
        });
        m_botonesFiltro.append(b);
        cab->addWidget(b);
    }
    // El chip activo se distingue con el contorno de acento.
    for (QPushButton *b : m_botonesFiltro)
        b->setStyleSheet(QString("QPushButton:checked { color:%1; border-color:%1; }")
                             .arg(Tema::Acento));
    lay->addLayout(cab);

    m_log = new QListWidget();
    m_log->setFixedHeight(126);
    m_log->setSelectionMode(QAbstractItemView::NoSelection);
    lay->addWidget(m_log);
    return host;
}

// ============================================================
//  Logica del simulador
// ============================================================
Proceso* MainWindow::buscarProceso(int pid) {
    for (auto &p : m_procesos) if (p.pid == pid) return &p;
    return nullptr;
}

void MainWindow::crearProceso() {
    Proceso p;
    p.pid = m_siguientePid++;
    const QString nombre = m_nombreEdit ? m_nombreEdit->text().trimmed() : QString();
    p.nombre = nombre.isEmpty() ? QString("proceso_%1").arg(p.pid) : nombre;
    p.prioridad = m_prioridadSlider ? m_prioridadSlider->value() : 3;
    p.rafagaTotal = m_rafagaSlider ? m_rafagaSlider->value() : 5;
    p.rafagaRestante = p.rafagaTotal;
    p.tickLlegada = m_reloj;
    p.estado = ProcState::Nuevo;
    p.contadorPrograma = QRandomGenerator::global()->bounded(0, 65535);
    p.baseMemoria = QRandomGenerator::global()->bounded(1000, 5000);
    p.limiteMemoriaKB = QRandomGenerator::global()->bounded(100, 500);
    for (auto &r : p.registros) r = QRandomGenerator::global()->bounded(0, 65535);
    p.historial.append({ProcState::Nuevo, m_reloj});

    p.hilos = QRandomGenerator::global()->bounded(1, 9);

    // Algunos procesos ya nacen con uno o dos recursos en uso (p. ej. ya
    // tenian un archivo abierto en disco). Solo se le dan los que esten
    // libres en ese momento (exclusion mutua); el resto se pide mas
    // adelante cuando el proceso se bloquea por E/S.
    const auto &catalogo = catalogoRecursos();
    const int nRecursos = QRandomGenerator::global()->bounded(0, 3); // 0, 1 o 2
    for (int i = 0; i < nRecursos; ++i) {
        const QString elegido = catalogo.at(QRandomGenerator::global()->bounded(catalogo.size()));
        if (!p.recursosAsignados.contains(elegido))
            intentarAsignarRecurso(p, elegido);
    }

    // Memoria que el proceso pide al SO al nacer (la elige quien lo crea,
    // ver flujo de admision: crear proceso -> generar PCB -> solicitar
    // memoria -> ¿hay bloque adecuado? -> asignar / no asignar). Se
    // intenta de inmediato, antes de que el proceso quede visible en NUEVO.
    p.memoriaRequeridaMB = m_memoriaNuevoSpin ? m_memoriaNuevoSpin->value()
                                               : QRandomGenerator::global()->bounded(64, 512);
    p.memoriaAsignada = false;
    p.memoriaMB = 0.0;
    p.memoriaPicoMB = 0.0;
    const bool memoriaOk = asignarMemoriaProceso(p);

    m_procesos.append(p);
    registrarEvento(QString("PID %1 \"%2\" creado · PCB inicializado")
                        .arg(p.pid).arg(p.nombre), TipoEvento::Sistema);
    if (!memoriaOk)
        registrarEvento(QString("PID %1 \"%2\": sin bloque contiguo de %3 disponible; "
                                "el proceso espera en NUEVO")
                            .arg(p.pid).arg(p.nombre, formatoMemoria(p.memoriaRequeridaMB)),
                         TipoEvento::Sistema);

    if (m_nombreEdit) m_nombreEdit->clear();

    // El proceso nace en NUEVO y permanece visible en esa cola. Si ya tiene
    // memoria asignada, poco despues el planificador de largo plazo lo
    // admite a LISTO (el usuario tambien puede adelantarlo con "Admitir").
    // Si no consiguio memoria, se queda esperando: se reintentara solo
    // cuando otro proceso libere memoria, se compacte o se reconfigure
    // el espacio de memoria fisica.
    refrescarTodo();

    const int pidNuevo = p.pid;
    QTimer::singleShot(900, this, [this, pidNuevo] {
        Proceso *proc = buscarProceso(pidNuevo);
        if (!proc || proc->estado != ProcState::Nuevo) return;  // ya fue admitido a mano
        if (!proc->memoriaAsignada) return;                      // sigue esperando memoria
        registrarEvento(QString("Planificador de largo plazo: admite PID %1 \"%2\" -> LISTO")
                            .arg(proc->pid).arg(proc->nombre), TipoEvento::Planificador);
        cambiarEstado(pidNuevo, ProcState::Listo, true);
        refrescarTodo();
    });
}

void MainWindow::cambiarEstado(int pid, ProcState nuevo, bool silencioso) {
    Proceso *p = buscarProceso(pid);
    if (!p) return;
    const ProcState anterior = p->estado;
    p->estado = nuevo;
    p->historial.append({nuevo, m_reloj});
    m_diagrama->setUltimaTransicion(anterior, nuevo);

    if (nuevo == ProcState::Ejecutando) {
        p->cambiosContexto++;                        // entrada a la CPU
        if (p->tickPrimeraEjecucion < 0)
            p->tickPrimeraEjecucion = m_reloj;       // tiempo de respuesta
    }
    if (nuevo == ProcState::Bloqueado) {
        p->operacionesES++;                          // peticion de E/S
        // Solicita un recurso del catalogo que todavia no tenga: si esta
        // libre se le asigna de inmediato; si esta ocupado, queda esperando
        // (recursosSolicitados) hasta que quien lo tiene lo libere.
        QStringList candidatos;
        for (const auto &r : catalogoRecursos())
            if (!p->recursosAsignados.contains(r)) candidatos.append(r);
        if (!candidatos.isEmpty()) {
            const QString elegido = candidatos.at(QRandomGenerator::global()->bounded(candidatos.size()));
            intentarAsignarRecurso(*p, elegido);
        }
    }
    if (nuevo == ProcState::Terminado) {
        if (p->tickFin < 0) p->tickFin = m_reloj;
        if (p->memoriaAsignada) liberarMemoriaProceso(p->pid);  // libera el bloque contiguo real
        p->memoriaMB = 0.0;                          // el SO libera su memoria
        p->memoriaAsignada = false;
        // Libera cada recurso que tenia: si alguien lo estaba esperando, se
        // lo lleva automaticamente (igual que un proceso real que finaliza).
        const QStringList tenia = p->recursosAsignados;
        for (const auto &r : tenia) liberarRecurso(*p, r);
        p->recursosSolicitados.clear();               // ya no puede seguir esperando
        p->ventanaCpu.clear();
        reintentarAdmisionesPendientes();             // puede que ahora si alcance para alguien
    }

    if (!silencioso)
        registrarEvento(QString("PID %1 \"%2\": %3 → %4")
                            .arg(p->pid).arg(p->nombre)
                            .arg(estadoATexto(anterior), estadoATexto(nuevo)));
}

// Admitir (NUEVO -> LISTO) exige tener memoria real asignada: si el
// proceso todavia no la tiene, se reintenta la asignacion aqui mismo
// antes de dejarlo competir por la CPU.
void MainWindow::admitir(int pid) {
    Proceso *p = buscarProceso(pid);
    if (!p) return;
    if (!p->memoriaAsignada && !asignarMemoriaProceso(*p)) {
        registrarEvento(QString("PID %1 \"%2\": no se pudo admitir, sigue sin memoria contigua")
                            .arg(p->pid).arg(p->nombre), TipoEvento::Sistema);
        refrescarTodo();
        return;
    }
    cambiarEstado(pid, ProcState::Listo);
    refrescarTodo();
}
void MainWindow::despertar(int pid) { cambiarEstado(pid, ProcState::Listo);     refrescarTodo(); }
void MainWindow::expropiar(int pid) { cambiarEstado(pid, ProcState::Listo);     refrescarTodo(); }
void MainWindow::bloquear(int pid)  { cambiarEstado(pid, ProcState::Bloqueado); refrescarTodo(); }
void MainWindow::terminar(int pid)  { cambiarEstado(pid, ProcState::Terminado); refrescarTodo(); }

void MainWindow::despachar(int pid) {
    for (const auto &p : m_procesos) {
        if (p.estado == ProcState::Ejecutando) {
            registrarEvento(QString("No se puede despachar PID %1: la CPU ya esta ocupada "
                                    "por PID %2").arg(pid).arg(p.pid), TipoEvento::Sistema);
            repintarBitacora();
            return;
        }
    }
    cambiarEstado(pid, ProcState::Ejecutando);
    refrescarTodo();
}

void MainWindow::avanzarTick() {
    m_reloj++;
    m_relojLabel->setText(relojTexto(m_reloj));

    // 1) Se contabiliza el tick con el estado que tuvieron los procesos
    //    durante ese tick (antes de descontar la rafaga).
    contabilizarTick();

    // 2) El proceso en CPU consume una unidad de su rafaga.
    for (auto &p : m_procesos) {
        if (p.estado == ProcState::Ejecutando) {
            p.rafagaRestante = qMax(0, p.rafagaRestante - 1);
            if (p.rafagaRestante == 0) {
                registrarEvento(QString("PID %1 \"%2\" agoto su rafaga de CPU")
                                    .arg(p.pid).arg(p.nombre), TipoEvento::Sistema);
                cambiarEstado(p.pid, ProcState::Terminado);
            }
            break; // un unico proceso puede estar ejecutando a la vez
        }
    }

    // 3) Nueva muestra para las graficas de rendimiento.
    m_taskManager->muestrear(m_procesos, cpuGlobalPorcentaje());

    refrescarTodo();
}

// ------------------------------------------------------------
//  Estadisticas
// ------------------------------------------------------------
void MainWindow::contabilizarTick() {
    auto *rnd = QRandomGenerator::global();
    bool cpuOcupada = false;

    for (auto &p : m_procesos) {
        if (p.estado == ProcState::Terminado) continue;

        const bool usoCpu = (p.estado == ProcState::Ejecutando);
        if (usoCpu)                                 { p.ticksCpu++; cpuOcupada = true; }
        else if (p.estado == ProcState::Listo)      { p.ticksEspera++; }
        else if (p.estado == ProcState::Bloqueado)  { p.ticksBloqueado++; }

        registrarMuestraCpu(p, usoCpu);

        // El uso real dentro del bloque reservado se mueve: crece mientras
        // ejecuta y se mantiene casi estable mientras espera, como en un
        // sistema real. Nunca supera el bloque contiguo que se le asigno
        // (reservado != usado), y si todavia no tiene bloque no hay nada
        // que mover.
        if (!p.memoriaAsignada) continue;

        double delta = 0.0;
        switch (p.estado) {
            case ProcState::Ejecutando: delta = rnd->bounded(-10, 34); break;
            case ProcState::Bloqueado:  delta = rnd->bounded(-4, 7);   break;
            case ProcState::Listo:      delta = rnd->bounded(-3, 4);   break;
            default:                    delta = 0.0;                   break;
        }
        // El piso nunca puede superar el techo: si el proceso pidio menos
        // de 16 MB, el piso baja con el (evita min > max en qBound).
        const double piso = qMin(16.0, p.memoriaRequeridaMB);
        p.memoriaMB = qBound(piso, p.memoriaMB + delta, p.memoriaRequeridaMB);
        p.memoriaPicoMB = qMax(p.memoriaPicoMB, p.memoriaMB);
    }

    m_ventanaSistema.append(cpuOcupada ? 1 : 0);
    while (m_ventanaSistema.size() > VENTANA_CPU)
        m_ventanaSistema.removeFirst();
}

double MainWindow::cpuGlobalPorcentaje() const {
    if (m_ventanaSistema.isEmpty()) return 0.0;
    int ocupados = 0;
    for (quint8 v : m_ventanaSistema) ocupados += v;
    return 100.0 * ocupados / double(m_ventanaSistema.size());
}

void MainWindow::refrescarEstadisticas() {
    m_taskManager->actualizar(m_procesos, m_reloj, cpuGlobalPorcentaje());
}

void MainWindow::reiniciarSimulacion() {
    const auto resp = QMessageBox::question(this, QStringLiteral("Reiniciar simulacion"),
                                            QStringLiteral("Se perderan todos los procesos. ¿Continuar?"));
    if (resp != QMessageBox::Yes) return;
    m_procesos.clear();
    m_ventanaSistema.clear();
    m_eventos.clear();
    m_siguientePid = 1;
    m_reloj = 0;
    m_relojLabel->setText(relojTexto(0));
    m_log->clear();
    m_diagrama->limpiarUltimaTransicion();
    m_taskManager->limpiar();
    inicializarMemoria();
    refrescarMapaMemoria();
    refrescarTodo();
}

// ------------------------------------------------------------
//  Bitacora
// ------------------------------------------------------------
void MainWindow::registrarEvento(const QString &mensaje, TipoEvento tipo) {
    m_eventos.append({m_reloj, tipo, mensaje});
    while (m_eventos.size() > 400) m_eventos.removeFirst();
}

void MainWindow::repintarBitacora() {
    m_log->clear();
    for (const Evento &e : m_eventos) {
        if (!m_filtroTodos && e.tipo != m_filtro) continue;
        QString marca;
        switch (e.tipo) {
            case TipoEvento::Transicion:   marca = QStringLiteral("transicion "); break;
            case TipoEvento::Planificador: marca = QStringLiteral("planificador"); break;
            case TipoEvento::Sistema:      marca = QStringLiteral("sistema     "); break;
        }
        auto *item = new QListWidgetItem(QString("t%1  %2  %3")
                                             .arg(e.tick, -3).arg(marca, e.texto));
        // El tipo se distingue por tono, sin usar los colores de estado.
        item->setForeground(QColor(e.tipo == TipoEvento::Transicion ? Tema::Neutro300
                                  : e.tipo == TipoEvento::Planificador ? Tema::Acento300
                                                                       : Tema::Neutro600));
        m_log->addItem(item);
    }
    m_log->scrollToBottom();
}

// ------------------------------------------------------------
//  Refresco de la vista
// ------------------------------------------------------------
void MainWindow::limpiarLayout(QVBoxLayout *layout) {
    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (QWidget *w = item->widget()) {
            // Ocultar y desligar antes de destruir evita que la tarjeta
            // antigua se siga pintando sobre la nueva durante el refresco.
            w->hide();
            w->setParent(nullptr);
            w->deleteLater();
        }
        delete item;
    }
}

void MainWindow::conectarTarjeta(QWidget *tarjeta) {
    auto *card = qobject_cast<PcbCardWidget *>(tarjeta);
    if (!card) return;
    connect(card, &PcbCardWidget::verDetalle, this, &MainWindow::mostrarDetallePcb);
    connect(card, &PcbCardWidget::accionAdmitir, this, &MainWindow::admitir);
    connect(card, &PcbCardWidget::accionDespachar, this, &MainWindow::despachar);
    connect(card, &PcbCardWidget::accionBloquear, this, &MainWindow::bloquear);
    connect(card, &PcbCardWidget::accionDespertar, this, &MainWindow::despertar);
    connect(card, &PcbCardWidget::accionExpropiar, this, &MainWindow::expropiar);
    connect(card, &PcbCardWidget::accionTerminar, this, &MainWindow::terminar);
}

void MainWindow::refrescarTodo() {
    refrescarColas();
    refrescarDiagrama();
    refrescarEstadisticas();
    repintarBitacora();
    m_franjaCpu->actualizar(m_procesos);
    m_gantt->actualizar(m_procesos, m_reloj);
    refrescarRecursos();
    refrescarEstadoGeneral();
}

void MainWindow::refrescarColas() {
    QVBoxLayout *columnas[4] = {m_colNuevo, m_colListo, m_colBloqueado, m_colTerminado};
    const ProcState estados[4] = {ProcState::Nuevo, ProcState::Listo,
                                  ProcState::Bloqueado, ProcState::Terminado};
    QLabel *contadores[4] = {m_countNuevo, m_countListo, m_countBloqueado, m_countTerminado};

    for (int i = 0; i < 4; ++i) {
        limpiarLayout(columnas[i]);
        int total = 0;
        for (const auto &p : m_procesos) {
            if (p.estado != estados[i]) continue;
            total++;
            auto *card = new PcbCardWidget(p);
            conectarTarjeta(card);
            columnas[i]->addWidget(card);
        }
        columnas[i]->addStretch();
        contadores[i]->setText(total == 1 ? QStringLiteral("1 proceso")
                                          : QString("%1 procesos").arg(total));
        if (total == 0) {
            auto *vacio = new QLabel(QStringLiteral("vacia"));
            vacio->setAlignment(Qt::AlignCenter);
            vacio->setStyleSheet(QString("font-family:%1; font-size:10.5px; color:%2;"
                                         "border:1px dashed %3; border-radius:6px; padding:12px;")
                                     .arg(Tema::FamiliaMono).arg(Tema::Neutro700).arg(Tema::Borde));
            columnas[i]->insertWidget(0, vacio);
        }
    }
}

void MainWindow::refrescarDiagrama() {
    QMap<ProcState, int> conteos;
    for (auto s : {ProcState::Nuevo, ProcState::Listo, ProcState::Ejecutando,
                   ProcState::Bloqueado, ProcState::Terminado})
        conteos[s] = 0;
    for (const auto &p : m_procesos) conteos[p.estado]++;
    m_diagrama->setCounts(conteos);
}

// ============================================================
//  Dialogo del PCB completo (dos columnas)
// ============================================================
void MainWindow::mostrarDetallePcb(int pid) {
    Proceso *p = buscarProceso(pid);
    if (!p) return;

    QDialog dlg(this);
    dlg.setWindowTitle(QString("PCB — PID %1").arg(p->pid));
    dlg.setMinimumSize(700, 540);
    dlg.setStyleSheet(QString("QDialog { background:%1; }").arg(Tema::Fondo));

    auto *raiz = new QVBoxLayout(&dlg);
    raiz->setContentsMargins(20, 18, 20, 18);
    raiz->setSpacing(14);

    // --- Cabecera: nombre, estado y procedencia ---
    auto *cab = new QHBoxLayout();
    cab->setSpacing(10);
    auto *bloqueTitulo = new QVBoxLayout();
    bloqueTitulo->setSpacing(4);

    auto *filaTitulo = new QHBoxLayout();
    filaTitulo->setSpacing(9);
    auto *titulo = new QLabel(p->nombre);
    titulo->setStyleSheet(QString("font-size:19px; font-weight:500; color:%1;").arg(Tema::Texto));
    auto *pastilla = new QLabel(estadoATexto(p->estado).toUpper());
    pastilla->setStyleSheet(QString("font-family:%1; font-size:10px; font-weight:600;"
                                    "letter-spacing:1.2px; color:%2; background:%3;"
                                    "border-radius:4px; padding:4px 8px;")
                                .arg(Tema::FamiliaMono)
                                .arg(Tema::colorEstadoHex(p->estado))
                                .arg(Tema::rgba(Tema::colorEstadoHex(p->estado), 40)));
    filaTitulo->addWidget(titulo);
    filaTitulo->addWidget(pastilla);
    filaTitulo->addStretch();
    bloqueTitulo->addLayout(filaTitulo);

    auto *procedencia = new QLabel(QString("PCB · PID %1 · llego en t%2 · %3 transiciones")
                                       .arg(p->pid).arg(p->tickLlegada).arg(p->historial.size()));
    procedencia->setStyleSheet(QString("font-family:%1; font-size:11px; color:%2;")
                                   .arg(Tema::FamiliaMono).arg(Tema::Neutro600));
    bloqueTitulo->addWidget(procedencia);
    cab->addLayout(bloqueTitulo);
    cab->addStretch();
    raiz->addLayout(cab);
    raiz->addWidget(separadorHorizontal());

    // --- Dos columnas: contexto a la izquierda, tiempos a la derecha ---
    auto *cuerpo = new QHBoxLayout();
    cuerpo->setSpacing(22);

    auto columna = [&]() {
        auto *lay = new QVBoxLayout();
        lay->setSpacing(9);
        cuerpo->addLayout(lay, 1);
        return lay;
    };
    auto *izq = columna();
    auto *der = columna();

    auto grupo = [&](QVBoxLayout *destino, const QString &titulo,
                     const QVector<QPair<QString, QString>> &campos,
                     bool acentuarValor = false) {
        destino->addWidget(rotulo(titulo));
        for (const auto &campo : campos) {
            auto *fila = new QHBoxLayout();
            auto *k = new QLabel(campo.first);
            k->setStyleSheet(QString("font-size:11.5px; color:%1;").arg(Tema::Neutro500));
            auto *v = new QLabel(campo.second);
            v->setStyleSheet(QString("font-family:%1; font-size:11.5px; font-weight:500; color:%2;")
                                 .arg(Tema::FamiliaMono)
                                 .arg(acentuarValor ? Tema::Acento300 : Tema::Texto));
            fila->addWidget(k);
            fila->addStretch();
            fila->addWidget(v);
            destino->addLayout(fila);
        }
        destino->addSpacing(6);
    };

    const auto hex = [](quint32 v) {
        return QString("0x%1").arg(v, 4, 16, QChar('0')).toUpper();
    };

    grupo(izq, QStringLiteral("MEMORIA"), {
        {QStringLiteral("Memoria requerida"), formatoMemoria(p->memoriaRequeridaMB)},
        {QStringLiteral("Memoria asignada"), p->memoriaAsignada ? QStringLiteral("Si")
                                                                 : QStringLiteral("No")},
        {QStringLiteral("Registro base"), hex(p->baseMemoria)},
        {QStringLiteral("Limite"), QString("%1 KB").arg(p->limiteMemoriaKB)},
        {QStringLiteral("En uso"), p->estado == ProcState::Terminado
                                       ? QStringLiteral("liberada")
                                       : formatoMemoria(p->memoriaMB)},
        {QStringLiteral("Pico"), formatoMemoria(p->memoriaPicoMB)},
        {QStringLiteral("Hilos"), QString::number(p->hilos)},
    });

    grupo(izq, QStringLiteral("RECURSOS"), {
        {QStringLiteral("Asignados"), textoRecursos(p->recursosAsignados)},
        {QStringLiteral("Solicitados"), textoRecursos(p->recursosSolicitados)},
    });

    grupo(izq, QStringLiteral("PLANIFICACION"), {
        {QStringLiteral("Prioridad"), QString("%1  (9 = alta)").arg(p->prioridad)},
        {QStringLiteral("Rafaga de CPU"), QString("%1 / %2 ticks")
                                              .arg(p->rafagaTotal - p->rafagaRestante)
                                              .arg(p->rafagaTotal)},
    });

    const int retorno = tiempoRetorno(*p);
    const int respuesta = tiempoRespuesta(*p);
    grupo(der, QStringLiteral("TIEMPOS"), {
        {QStringLiteral("Uso de CPU"), QString::number(cpuPorcentaje(*p), 'f', 0) + " %"},
        {QStringLiteral("En CPU"), QString("%1 ticks").arg(p->ticksCpu)},
        {QStringLiteral("En espera"), QString("%1 ticks").arg(p->ticksEspera)},
        {QStringLiteral("Bloqueado"), QString("%1 ticks").arg(p->ticksBloqueado)},
        {QStringLiteral("T. de respuesta"), respuesta >= 0 ? QString("%1 ticks").arg(respuesta)
                                                           : QStringLiteral("aun sin CPU")},
        {QStringLiteral("T. de retorno"), retorno >= 0 ? QString("%1 ticks").arg(retorno)
                                                       : QStringLiteral("en curso")},
        {QStringLiteral("Cambios de contexto"), QString::number(p->cambiosContexto)},
        {QStringLiteral("Operaciones de E/S"), QString::number(p->operacionesES)},
    }, true);

    der->addWidget(rotulo(QStringLiteral("HISTORIAL DE TRANSICIONES")));
    auto *historial = new QListWidget();
    for (const auto &h : p->historial) {
        auto *item = new QListWidgetItem(QString("t%1   %2")
                                             .arg(h.tick, -3).arg(estadoATexto(h.estado)));
        item->setForeground(Tema::colorEstado(h.estado));
        historial->addItem(item);
    }
    historial->scrollToBottom();
    der->addWidget(historial, 1);

    raiz->addLayout(cuerpo, 1);

    auto *pie = new QHBoxLayout();
    pie->addStretch();
    auto *btnCerrar = new QPushButton(QStringLiteral("Cerrar"));
    btnCerrar->setObjectName("primary");
    btnCerrar->setCursor(Qt::PointingHandCursor);
    connect(btnCerrar, &QPushButton::clicked, &dlg, &QDialog::accept);
    pie->addWidget(btnCerrar);
    raiz->addLayout(pie);

    dlg.exec();
}

// ============================================================
//  Acciones del menu principal
// ============================================================

// Deja elegir un proceso de una lista ya filtrada por el llamador.
// Devuelve el PID elegido, o -1 si no hay candidatos o el usuario cancela.
int MainWindow::elegirProceso(const QString &titulo, const QString &etiqueta,
                              const QVector<Proceso *> &candidatos) {
    if (candidatos.isEmpty()) {
        QMessageBox::information(this, titulo,
            QStringLiteral("No hay ningun proceso que cumpla esta condicion en este momento."));
        return -1;
    }
    QStringList opciones;
    for (auto *p : candidatos)
        opciones << QString("PID %1 — %2 (%3)").arg(p->pid).arg(p->nombre, estadoATexto(p->estado));

    bool ok = false;
    const QString elegido = QInputDialog::getItem(this, titulo, etiqueta, opciones, 0, false, &ok);
    if (!ok) return -1;
    const int idx = opciones.indexOf(elegido);
    return idx >= 0 ? candidatos[idx]->pid : -1;
}

// --- Gestion de procesos ---

void MainWindow::menuMostrarProcesos() {
    if (m_pestanas) m_pestanas->setCurrentWidget(m_taskManager);
}

void MainWindow::menuMostrarPcb() {
    QVector<Proceso *> todos;
    for (auto &p : m_procesos) todos.append(&p);
    const int pid = elegirProceso(QStringLiteral("Mostrar PCB"),
                                  QStringLiteral("Selecciona el proceso:"), todos);
    if (pid >= 0) mostrarDetallePcb(pid);
}

void MainWindow::menuCambiarEstado() {
    QVector<Proceso *> todos;
    for (auto &p : m_procesos) if (p.estado != ProcState::Terminado) todos.append(&p);
    const int pid = elegirProceso(QStringLiteral("Cambiar estado"),
                                  QStringLiteral("Selecciona el proceso:"), todos);
    if (pid < 0) return;
    Proceso *p = buscarProceso(pid);
    if (!p) return;

    QStringList opciones;
    switch (p->estado) {
    case ProcState::Nuevo:
        opciones << QStringLiteral("Admitir  (Nuevo -> Listo)");
        break;
    case ProcState::Listo:
        opciones << QStringLiteral("Despachar  (Listo -> Ejecutando)")
                  << QStringLiteral("Finalizar  (Listo -> Terminado)");
        break;
    case ProcState::Ejecutando:
        opciones << QStringLiteral("Bloquear por E/S  (Ejecutando -> Bloqueado)")
                  << QStringLiteral("Expropiar  (Ejecutando -> Listo)")
                  << QStringLiteral("Finalizar  (Ejecutando -> Terminado)");
        break;
    case ProcState::Bloqueado:
        opciones << QStringLiteral("E/S completa  (Bloqueado -> Listo)");
        break;
    default:
        break;
    }
    if (opciones.isEmpty()) return;

    bool ok = false;
    const QString elegido = QInputDialog::getItem(this, QStringLiteral("Cambiar estado"),
        QString("PID %1 \"%2\" esta en %3. Elige la transicion:")
            .arg(p->pid).arg(p->nombre, estadoATexto(p->estado)),
        opciones, 0, false, &ok);
    if (!ok) return;

    if (elegido.startsWith(QStringLiteral("Admitir")))          admitir(pid);
    else if (elegido.startsWith(QStringLiteral("Despachar")))   despachar(pid);
    else if (elegido.startsWith(QStringLiteral("Bloquear")))    bloquear(pid);
    else if (elegido.startsWith(QStringLiteral("Expropiar")))   expropiar(pid);
    else if (elegido.startsWith(QStringLiteral("E/S completa"))) despertar(pid);
    else if (elegido.startsWith(QStringLiteral("Finalizar")))   terminar(pid);
}

void MainWindow::menuMostrarColas() {
    if (m_pestanas) m_pestanas->setCurrentIndex(0);
}

void MainWindow::menuFinalizarProceso() {
    QVector<Proceso *> candidatos;
    for (auto &p : m_procesos) if (p.estado != ProcState::Terminado) candidatos.append(&p);
    const int pid = elegirProceso(QStringLiteral("Finalizar proceso"),
                                  QStringLiteral("Selecciona el proceso a finalizar:"), candidatos);
    if (pid >= 0) terminar(pid);
}

// --- Administracion de memoria: particiones contiguas ---

// Memoria inicial al arrancar o tras reiniciar la simulacion.
void MainWindow::inicializarMemoria() {
    m_bloques = memoriaInicial(m_memoriaTotalMB, m_memoriaSoMB);
}

// Intenta reservarle a 'p' un bloque contiguo de su memoria requerida
// segun el metodo de ajuste vigente, y deja constancia en la bitacora de
// la decision (proceso, cantidad, huecos disponibles, bloque elegido,
// espacio restante y el resultado).
bool MainWindow::asignarMemoriaProceso(Proceso &p) {
    const QVector<double> huecosAntes = huecosLibres(m_bloques);
    const ResultadoAsignacion r =
        asignarMemoria(m_bloques, p.memoriaRequeridaMB, p.pid, p.nombre, m_metodoAjuste);

    QStringList huecosTxt;
    for (double h : huecosAntes) huecosTxt << formatoMemoria(h);
    const QString huecosStr = huecosTxt.isEmpty() ? QStringLiteral("ninguno") : huecosTxt.join(", ");

    if (r.exito) {
        p.memoriaAsignada = true;
        p.memoriaMB = p.memoriaRequeridaMB;
        p.memoriaPicoMB = qMax(p.memoriaPicoMB, p.memoriaMB);
        registrarEvento(QString("Asignacion (%1): PID %2 \"%3\" pidio %4 · huecos disponibles [%5] "
                                "· bloque elegido de %6 · espacio restante %7 · OK, memoria asignada")
                            .arg(metodoATexto(m_metodoAjuste)).arg(p.pid)
                            .arg(p.nombre, formatoMemoria(p.memoriaRequeridaMB), huecosStr,
                                 formatoMemoria(r.huecoOriginalMB), formatoMemoria(r.sobranteMB)),
                         TipoEvento::Sistema);
    } else {
        registrarEvento(QString("Asignacion (%1): PID %2 \"%3\" pidio %4 · huecos disponibles [%5] "
                                "· ningun hueco individual alcanza · memoria NO asignada, sigue en NUEVO")
                            .arg(metodoATexto(m_metodoAjuste)).arg(p.pid)
                            .arg(p.nombre, formatoMemoria(p.memoriaRequeridaMB), huecosStr),
                         TipoEvento::Sistema);
    }
    refrescarMapaMemoria();
    return r.exito;
}

// Libera el bloque real del proceso 'pid' (con fusion de huecos adyacentes).
void MainWindow::liberarMemoriaProceso(int pid) {
    Proceso *p = buscarProceso(pid);
    const QString nombre = p ? p->nombre : QString();
    liberarMemoria(m_bloques, pid);
    registrarEvento(QString("PID %1 \"%2\": bloque de memoria liberado · nuevo hueco disponible")
                        .arg(pid).arg(nombre), TipoEvento::Sistema);
    refrescarMapaMemoria();
}

// Cada vez que se libera o compacta memoria, algun proceso que seguia en
// NUEVO por falta de memoria puede caber ahora: se reintenta y, si cabe,
// se admite automaticamente (igual que el planificador de largo plazo).
void MainWindow::reintentarAdmisionesPendientes() {
    QVector<int> pendientes;
    for (const auto &p : m_procesos)
        if (p.estado == ProcState::Nuevo && !p.memoriaAsignada) pendientes.append(p.pid);

    for (int pid : pendientes) {
        Proceso *p = buscarProceso(pid);
        if (!p || p->estado != ProcState::Nuevo || p->memoriaAsignada) continue;
        if (asignarMemoriaProceso(*p)) {
            registrarEvento(QString("PID %1 \"%2\": ya hay espacio contiguo, admitido a LISTO")
                                .arg(p->pid).arg(p->nombre), TipoEvento::Planificador);
            cambiarEstado(pid, ProcState::Listo, true);
        }
    }
}

// Fila visual de un bloque del mapa de memoria (SO / proceso / libre).
QWidget* MainWindow::construirFilaBloque(const BloqueMemoria &b) {
    QString etiqueta;
    QString color;
    switch (b.tipo) {
    case TipoBloque::SistemaOperativo:
        etiqueta = QStringLiteral("Sistema Operativo");
        color = Tema::Neutro600;
        break;
    case TipoBloque::Proceso:
        etiqueta = QString("PID %1 · %2").arg(b.pid).arg(b.nombre);
        color = Tema::Acento300;
        break;
    case TipoBloque::Libre:
        etiqueta = QStringLiteral("Libre");
        color = Tema::Neutro800;
        break;
    }

    auto *fila = new QFrame();
    fila->setStyleSheet(QString(
        "QFrame { background:%1; border:1px solid %2; border-left:3px solid %3; border-radius:3px; }"
        "QLabel { background:transparent; border:none; }")
        .arg(b.tipo == TipoBloque::Libre ? Tema::Fondo : Tema::Superficie)
        .arg(Tema::BordeSuave).arg(color));
    auto *lay = new QHBoxLayout(fila);
    lay->setContentsMargins(10, 7, 10, 7);
    auto *lblNombre = new QLabel(etiqueta);
    lblNombre->setStyleSheet(QString("font-size:11.5px; color:%1;")
                                 .arg(b.tipo == TipoBloque::Libre ? Tema::Neutro600 : Tema::Texto));
    auto *lblTam = new QLabel(formatoMemoria(b.tamanoMB));
    lblTam->setStyleSheet(QString("font-family:%1; font-size:11px; font-weight:600; color:%2;")
                              .arg(Tema::FamiliaMono).arg(color));
    lay->addWidget(lblNombre);
    lay->addStretch();
    lay->addWidget(lblTam);
    return fila;
}

// Repinta el mapa de bloques y el resumen de fragmentacion (huecos,
// mayor hueco y memoria libre total).
void MainWindow::refrescarMapaMemoria() {
    if (!m_mapaMemoriaLayout) return;
    limpiarLayout(m_mapaMemoriaLayout);
    for (const auto &b : m_bloques)
        m_mapaMemoriaLayout->addWidget(construirFilaBloque(b));
    m_mapaMemoriaLayout->addStretch();

    if (m_mapaMemoriaTotalLabel)
        m_mapaMemoriaTotalLabel->setText(QString("MEMORIA TOTAL: %1").arg(formatoMemoria(m_memoriaTotalMB)));

    if (m_fragmentacionInfo) {
        const auto huecos = huecosLibres(m_bloques);
        const double libreTotal = memoriaLibreTotal(m_bloques);
        double mayor = 0.0;
        for (double h : huecos) mayor = qMax(mayor, h);
        QStringList huecosTxt;
        for (double h : huecos) huecosTxt << formatoMemoria(h);
        m_fragmentacionInfo->setText(QString(
            "Memoria libre total: %1 en %2 hueco(s)\nHueco mas grande: %3\nHuecos: [%4]")
            .arg(formatoMemoria(libreTotal)).arg(huecos.size())
            .arg(formatoMemoria(mayor),
                 huecosTxt.isEmpty() ? QStringLiteral("ninguno") : huecosTxt.join(", ")));
    }
}

// --- 8. Asignar memoria: reintento manual para un proceso en espera ---
void MainWindow::memAsignarManual() {
    QVector<Proceso *> candidatos;
    for (auto &p : m_procesos)
        if (p.estado == ProcState::Nuevo && !p.memoriaAsignada) candidatos.append(&p);
    const int pid = elegirProceso(QStringLiteral("Asignar memoria"),
        QStringLiteral("Procesos en NUEVO esperando un hueco contiguo:"), candidatos);
    if (pid < 0) return;
    Proceso *p = buscarProceso(pid);
    if (!p) return;

    if (asignarMemoriaProceso(*p)) {
        admitir(pid);
    } else {
        QMessageBox::information(this, QStringLiteral("Asignar memoria"),
            QString("Todavia no hay un hueco contiguo de al menos %1 para PID %2 \"%3\".")
                .arg(formatoMemoria(p->memoriaRequeridaMB)).arg(p->pid).arg(p->nombre));
    }
}

// --- 9. Liberar memoria: demuestra que finalizar libera el bloque ---
void MainWindow::memLiberarManual() {
    QVector<Proceso *> candidatos;
    for (auto &p : m_procesos)
        if (p.estado != ProcState::Terminado && p.memoriaAsignada) candidatos.append(&p);
    const int pid = elegirProceso(QStringLiteral("Liberar memoria"),
        QStringLiteral("Finalizar uno de estos procesos libera su bloque de memoria:"), candidatos);
    if (pid >= 0) terminar(pid);
}

// --- 10. Mostrar fragmentacion: prueba una asignacion hipotetica ---
void MainWindow::memProbarFragmentacion() {
    const auto huecos = huecosLibres(m_bloques);
    const double libreTotal = memoriaLibreTotal(m_bloques);
    double mayorHueco = 0.0;
    for (double h : huecos) mayorHueco = qMax(mayorHueco, h);

    bool ok = false;
    const double solicitado = QInputDialog::getDouble(this, QStringLiteral("Probar asignacion hipotetica"),
        QStringLiteral("Tamano a solicitar (MB):"), 300.0, 1.0, 100000.0, 0, &ok);
    if (!ok) return;

    QStringList huecosTxt;
    for (double h : huecos) huecosTxt << formatoMemoria(h);
    const QString huecosStr = huecosTxt.isEmpty() ? QStringLiteral("ninguno") : huecosTxt.join(", ");

    if (solicitado <= mayorHueco) {
        QMessageBox::information(this, QStringLiteral("Cabe sin problema"),
            QString("Se pidieron %1. El hueco mas grande disponible es de %2, asi que si cabe "
                    "en un unico bloque contiguo.\n\nHuecos actuales: [%3]")
                .arg(formatoMemoria(solicitado), formatoMemoria(mayorHueco), huecosStr));
    } else {
        QMessageBox::warning(this, QStringLiteral("Fragmentacion externa"),
            QString("Se pidieron %1.\n\nMemoria libre total: %2 (suficiente en teoria).\n"
                    "Pero el hueco mas grande disponible es de %3: ningun hueco individual "
                    "alcanza, aunque la suma de todos si.\n\nHuecos actuales: [%4]\n\n"
                    "Esto es fragmentacion externa: sobra memoria libre, pero esta repartida "
                    "en bloques no contiguos y ninguno es lo bastante grande por si solo.")
                .arg(formatoMemoria(solicitado), formatoMemoria(libreTotal),
                     formatoMemoria(mayorHueco), huecosStr));
    }
}

// --- 11. Compactar memoria ---
void MainWindow::memCompactar() {
    const double libreAntes = memoriaLibreTotal(m_bloques);
    const int huecosAntes = huecosLibres(m_bloques).size();
    compactarMemoria(m_bloques);
    registrarEvento(QString("Memoria compactada: %1 hueco(s) dispersos (%2 libres en total) "
                            "se unieron en un solo bloque libre al final")
                        .arg(huecosAntes).arg(formatoMemoria(libreAntes)), TipoEvento::Sistema);
    refrescarMapaMemoria();
    reintentarAdmisionesPendientes();
    refrescarTodo();
}

// --- Configuracion del espacio de memoria fisica (total / reservado al SO) ---
void MainWindow::memAplicarConfiguracion() {
    if (!m_totalMemSpin || !m_soMemSpin) return;
    const double nuevoTotal = m_totalMemSpin->value();
    const double nuevoSo = m_soMemSpin->value();

    if (nuevoSo >= nuevoTotal) {
        QMessageBox::warning(this, QStringLiteral("Configuracion de memoria"),
            QStringLiteral("La memoria reservada para el SO debe ser menor que la memoria total."));
        return;
    }

    double reservadaProcesos = 0.0;
    for (const auto &p : m_procesos)
        if (p.estado != ProcState::Terminado && p.memoriaAsignada) reservadaProcesos += p.memoriaRequeridaMB;

    if (nuevoSo + reservadaProcesos > nuevoTotal) {
        QMessageBox::warning(this, QStringLiteral("Configuracion de memoria"),
            QString("No alcanza: el SO (%1) mas los procesos que ya tienen memoria asignada (%2) "
                    "superan la nueva memoria total (%3).")
                .arg(formatoMemoria(nuevoSo), formatoMemoria(reservadaProcesos), formatoMemoria(nuevoTotal)));
        return;
    }

    m_memoriaTotalMB = nuevoTotal;
    m_memoriaSoMB = nuevoSo;

    // Reconstruye la memoria: SO, luego cada proceso que ya tenia bloque
    // (conserva su tamano fijo) y el resto como un unico hueco libre.
    QVector<BloqueMemoria> nuevos;
    nuevos.append({TipoBloque::SistemaOperativo, m_memoriaSoMB, -1, QString()});
    double usados = m_memoriaSoMB;
    for (auto &p : m_procesos) {
        if (p.estado != ProcState::Terminado && p.memoriaAsignada) {
            nuevos.append({TipoBloque::Proceso, p.memoriaRequeridaMB, p.pid, p.nombre});
            usados += p.memoriaRequeridaMB;
        }
    }
    const double libre = m_memoriaTotalMB - usados;
    if (libre > 0.0) nuevos.append({TipoBloque::Libre, libre, -1, QString()});
    m_bloques = nuevos;

    registrarEvento(QString("Memoria fisica reconfigurada: total %1 · SO %2")
                        .arg(formatoMemoria(m_memoriaTotalMB), formatoMemoria(m_memoriaSoMB)),
                     TipoEvento::Sistema);
    refrescarMapaMemoria();
    reintentarAdmisionesPendientes();
    refrescarTodo();
}

// --- Administracion de recursos: exclusion mutua con cola de espera ---

// Intenta darle 'recurso' a 'p'. Si nadie mas lo tiene en ese momento se
// le asigna de inmediato; si esta ocupado, 'p' queda anotado en su lista
// de espera hasta que quien lo tiene lo libere. Deja constancia en la
// bitacora de cual de los dos casos ocurrio.
bool MainWindow::intentarAsignarRecurso(Proceso &p, const QString &recurso) {
    if (p.recursosAsignados.contains(recurso)) return true;  // ya lo tiene

    Proceso *tenedor = nullptr;
    for (auto &q : m_procesos) {
        if (q.pid == p.pid || q.estado == ProcState::Terminado) continue;
        if (q.recursosAsignados.contains(recurso)) { tenedor = &q; break; }
    }

    if (!tenedor) {
        p.recursosAsignados.append(recurso);
        p.recursosSolicitados.removeAll(recurso);
        registrarEvento(QString("PID %1 \"%2\": recurso \"%3\" asignado (estaba disponible)")
                            .arg(p.pid).arg(p.nombre, recurso), TipoEvento::Sistema);
        refrescarRecursos();
        return true;
    }

    if (!p.recursosSolicitados.contains(recurso))
        p.recursosSolicitados.append(recurso);
    registrarEvento(QString("PID %1 \"%2\": recurso \"%3\" ocupado por PID %4, queda en espera")
                        .arg(p.pid).arg(p.nombre, recurso).arg(tenedor->pid), TipoEvento::Sistema);
    refrescarRecursos();
    return false;
}

// Libera 'recurso' de 'p'. Si algun otro proceso lo estaba esperando, se
// lo concede de inmediato al primero de la lista (FCFS por orden de PID
// dentro del vector de procesos).
void MainWindow::liberarRecurso(Proceso &p, const QString &recurso) {
    if (!p.recursosAsignados.contains(recurso)) return;
    p.recursosAsignados.removeAll(recurso);
    registrarEvento(QString("PID %1 \"%2\": recurso \"%3\" liberado")
                        .arg(p.pid).arg(p.nombre, recurso), TipoEvento::Sistema);

    Proceso *siguiente = nullptr;
    for (auto &q : m_procesos) {
        if (q.pid == p.pid || q.estado == ProcState::Terminado) continue;
        if (q.recursosSolicitados.contains(recurso)) { siguiente = &q; break; }
    }
    if (siguiente) {
        siguiente->recursosSolicitados.removeAll(recurso);
        siguiente->recursosAsignados.append(recurso);
        registrarEvento(QString("PID %1 \"%2\": recurso \"%3\" concedido (estaba esperando)")
                            .arg(siguiente->pid).arg(siguiente->nombre, recurso), TipoEvento::Sistema);
    }
    refrescarRecursos();
}

// Fila visual de un recurso del catalogo: a quien pertenece (o "Disponible")
// y quien esta esperando por el, si alguien.
QWidget* MainWindow::construirFilaRecurso(const QString &recurso) {
    Proceso *tenedor = nullptr;
    for (auto &p : m_procesos)
        if (p.estado != ProcState::Terminado && p.recursosAsignados.contains(recurso)) { tenedor = &p; break; }

    QStringList esperando;
    for (const auto &p : m_procesos)
        if (p.estado != ProcState::Terminado && p.recursosSolicitados.contains(recurso))
            esperando << QString("PID %1").arg(p.pid);

    const bool libre = (tenedor == nullptr);
    const QString valor = libre ? QStringLiteral("Disponible")
                                 : QString("PID %1 · %2").arg(tenedor->pid).arg(tenedor->nombre);
    const QString color = libre ? Tema::Neutro600 : Tema::Acento300;

    auto *fila = new QFrame();
    fila->setStyleSheet(QString(
        "QFrame { background:%1; border:1px solid %2; border-left:3px solid %3; border-radius:3px; }"
        "QLabel { background:transparent; border:none; }")
        .arg(libre ? Tema::Fondo : Tema::Superficie)
        .arg(Tema::BordeSuave).arg(color));
    auto *lay = new QVBoxLayout(fila);
    lay->setContentsMargins(10, 7, 10, 7);
    lay->setSpacing(2);

    auto *filaSup = new QHBoxLayout();
    auto *lblNombre = new QLabel(recurso);
    lblNombre->setStyleSheet(QString("font-size:11.5px; font-weight:500; color:%1;").arg(Tema::Texto));
    auto *lblValor = new QLabel(valor);
    lblValor->setStyleSheet(QString("font-family:%1; font-size:11px; font-weight:600; color:%2;")
                                .arg(Tema::FamiliaMono).arg(color));
    filaSup->addWidget(lblNombre);
    filaSup->addStretch();
    filaSup->addWidget(lblValor);
    lay->addLayout(filaSup);

    if (!esperando.isEmpty()) {
        auto *lblEspera = new QLabel(QString("en espera: %1").arg(esperando.join(", ")));
        lblEspera->setStyleSheet(QString("font-family:%1; font-size:9.5px; color:%2;")
                                     .arg(Tema::FamiliaMono).arg(Tema::EstBloqueado));
        lay->addWidget(lblEspera);
    }
    return fila;
}

// Repinta la tabla de recursos (13), el resumen de disponibles/ocupados y
// quien posee o espera cada cosa, y de paso recalcula el estado del grafo
// de espera (16) porque puede haber cambiado.
void MainWindow::refrescarRecursos() {
    if (!m_recursosLayout) return;
    limpiarLayout(m_recursosLayout);
    for (const auto &r : catalogoRecursos())
        m_recursosLayout->addWidget(construirFilaRecurso(r));
    m_recursosLayout->addStretch();

    if (m_recursosResumen) {
        QStringList poseen, esperan;
        for (const auto &p : m_procesos) {
            if (p.estado == ProcState::Terminado) continue;
            if (!p.recursosAsignados.isEmpty())
                poseen << QString("PID %1 (%2)").arg(p.pid).arg(p.recursosAsignados.join(", "));
            if (!p.recursosSolicitados.isEmpty())
                esperan << QString("PID %1 (%2)").arg(p.pid).arg(p.recursosSolicitados.join(", "));
        }
        int libres = 0, ocupados = 0;
        for (const auto &r : catalogoRecursos()) {
            bool ocupado = false;
            for (const auto &p : m_procesos)
                if (p.estado != ProcState::Terminado && p.recursosAsignados.contains(r)) { ocupado = true; break; }
            ocupado ? ++ocupados : ++libres;
        }
        m_recursosResumen->setText(QString(
            "Disponibles: %1 · Ocupados: %2\n"
            "Procesos que poseen un recurso: %3\n"
            "Procesos que esperan un recurso: %4")
            .arg(libres).arg(ocupados)
            .arg(poseen.isEmpty() ? QStringLiteral("ninguno") : poseen.join("  ·  "),
                 esperan.isEmpty() ? QStringLiteral("ninguno") : esperan.join("  ·  ")));
    }

    recDetectar();
}

// --- 14. Asignar recurso: un proceso solicita un recurso concreto ---
void MainWindow::recAsignarManual() {
    QVector<Proceso *> candidatos;
    for (auto &p : m_procesos) if (p.estado != ProcState::Terminado) candidatos.append(&p);
    const int pid = elegirProceso(QStringLiteral("Asignar recurso"),
                                  QStringLiteral("Selecciona el proceso que solicita:"), candidatos);
    if (pid < 0) return;
    Proceso *p = buscarProceso(pid);
    if (!p) return;

    QStringList solicitables;
    for (const auto &r : catalogoRecursos())
        if (!p->recursosAsignados.contains(r)) solicitables << r;
    if (solicitables.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Asignar recurso"),
            QStringLiteral("Este proceso ya tiene asignados todos los recursos del catalogo."));
        return;
    }

    bool ok = false;
    const QString elegido = QInputDialog::getItem(this, QStringLiteral("Asignar recurso"),
        QString("Recurso que solicita PID %1 \"%2\":").arg(p->pid).arg(p->nombre),
        solicitables, 0, false, &ok);
    if (!ok) return;

    if (intentarAsignarRecurso(*p, elegido)) {
        QMessageBox::information(this, QStringLiteral("Recurso asignado"),
            QString("\"%1\" estaba disponible: se le asigno a PID %2 \"%3\".")
                .arg(elegido).arg(p->pid).arg(p->nombre));
    } else {
        QMessageBox::warning(this, QStringLiteral("Recurso ocupado"),
            QString("\"%1\" esta ocupado. PID %2 \"%3\" queda en espera hasta que se libere.")
                .arg(elegido).arg(p->pid).arg(p->nombre));
    }
    refrescarTodo();
}

// --- 15. Liberar recurso ---
void MainWindow::recLiberarManual() {
    QVector<Proceso *> candidatos;
    for (auto &p : m_procesos) if (!p.recursosAsignados.isEmpty()) candidatos.append(&p);
    const int pid = elegirProceso(QStringLiteral("Liberar recurso"),
                                  QStringLiteral("Procesos con recursos asignados:"), candidatos);
    if (pid < 0) return;
    Proceso *p = buscarProceso(pid);
    if (!p) return;

    bool ok = false;
    const QString elegido = QInputDialog::getItem(this, QStringLiteral("Liberar recurso"),
        QString("Recurso a liberar de PID %1 \"%2\":").arg(p->pid).arg(p->nombre),
        p->recursosAsignados, 0, false, &ok);
    if (!ok) return;

    liberarRecurso(*p, elegido);
    refrescarTodo();
}

// --- 16. Detectar interbloqueo ---
// Construye el grafo de espera (todo proceso vivo con una solicitud
// pendiente apunta hacia quien tiene ese recurso) y busca ciclos con DFS.
// Actualiza el panel en vivo con el resultado; si hay ciclo, lo explica y
// propone la estrategia del simulador para tratarlo.
void MainWindow::recDetectar() {
    if (!m_interbloqueoInfo) return;

    QMap<int, QVector<int>> espera;       // PID -> PIDs que poseen lo que pide
    QMap<int, QStringList> detalleEspera; // PID -> "recurso (la tiene PID X)"
    for (const auto &p : m_procesos) {
        if (p.estado == ProcState::Terminado || p.recursosSolicitados.isEmpty()) continue;
        for (const auto &r : p.recursosSolicitados) {
            for (const auto &q : m_procesos) {
                if (q.pid == p.pid || q.estado == ProcState::Terminado) continue;
                if (q.recursosAsignados.contains(r)) {
                    espera[p.pid].append(q.pid);
                    detalleEspera[p.pid].append(QString("%1 (la tiene PID %2)").arg(r).arg(q.pid));
                }
            }
        }
    }

    if (espera.isEmpty()) {
        m_interbloqueoInfo->setText(QStringLiteral(
            "No hay ningun proceso esperando un recurso ocupado en este momento."));
        return;
    }

    // El ciclo (si existe) lo calcula el mismo helper que usa el resumen
    // de "Sistema", para que ambas vistas sean siempre consistentes.
    QVector<int> ciclo;
    const bool encontrado = detectarCicloEspera(&ciclo);

    QStringList lineas;
    lineas << QStringLiteral("Espera actual:");
    for (auto it = espera.keyBegin(); it != espera.keyEnd(); ++it) {
        Proceso *p = buscarProceso(*it);
        lineas << QString("  PID %1 (%2) espera: %3")
                      .arg(*it).arg(p ? p->nombre : QString(), detalleEspera.value(*it).join(", "));
    }
    lineas << QString();

    if (!encontrado) {
        lineas << QStringLiteral("No hay interbloqueo: el grafo de espera no tiene ciclos "
                                 "(en algun punto de la cadena alguien terminara y liberara "
                                 "su recurso).");
    } else {
        QStringList nombres;
        for (int pid : ciclo) {
            Proceso *p = buscarProceso(pid);
            nombres << QString("PID %1 (%2)").arg(pid).arg(p ? p->nombre : QString());
        }
        lineas << QStringLiteral("INTERBLOQUEO DETECTADO:")
               << QStringLiteral("  ") + nombres.join(QStringLiteral("  →  espera →  "))
               << QString()
               << QStringLiteral(
                    "Cada proceso del ciclo tiene un recurso que otro del mismo ciclo "
                    "necesita, asi que ninguno puede avanzar (condiciones de Coffman: "
                    "exclusion mutua, retencion y espera, no expropiacion, y espera "
                    "circular). Estrategia de este simulador: DETECCION + RECUPERACION "
                    "— se detecta el ciclo (como aqui) y se rompe manualmente finalizando "
                    "uno de los procesos involucrados (pestana \"Gestion de procesos\" "
                    "→ Finalizar proceso), lo que libera su recurso y deja avanzar al resto.");
    }
    m_interbloqueoInfo->setText(lineas.join("\n"));
}

// --- Interbloqueo: helper compartido entre la pestana de recursos (16)
//     y el resumen de "Sistema" (pestana 4) ---
// Construye el grafo de espera (todo proceso vivo con una solicitud
// pendiente apunta hacia quien tiene ese recurso) y busca un ciclo con
// DFS. Si lo encuentra y 'cicloSalida' no es null, deja ahi los PIDs.
bool MainWindow::detectarCicloEspera(QVector<int> *cicloSalida) const {
    QMap<int, QVector<int>> espera;
    for (const auto &p : m_procesos) {
        if (p.estado == ProcState::Terminado || p.recursosSolicitados.isEmpty()) continue;
        for (const auto &r : p.recursosSolicitados) {
            for (const auto &q : m_procesos) {
                if (q.pid == p.pid || q.estado == ProcState::Terminado) continue;
                if (q.recursosAsignados.contains(r)) espera[p.pid].append(q.pid);
            }
        }
    }
    if (espera.isEmpty()) return false;

    QVector<int> ciclo;
    QVector<int> visitados;
    std::function<bool(int, QVector<int> &)> dfs = [&](int nodo, QVector<int> &pila) -> bool {
        if (pila.contains(nodo)) {
            ciclo = pila.mid(pila.indexOf(nodo));
            ciclo.append(nodo);
            return true;
        }
        if (visitados.contains(nodo)) return false;
        visitados.append(nodo);
        pila.append(nodo);
        for (int vecino : espera.value(nodo))
            if (dfs(vecino, pila)) return true;
        pila.removeLast();
        return false;
    };

    for (auto it = espera.keyBegin(); it != espera.keyEnd(); ++it) {
        QVector<int> pila;
        if (dfs(*it, pila)) {
            if (cicloSalida) *cicloSalida = ciclo;
            return true;
        }
    }
    return false;
}

// --- Sistema: resumen unico con procesos, memoria, recursos e interbloqueo ---
void MainWindow::refrescarEstadoGeneral() {
    if (!m_estadoGeneralLabel) return;

    auto listaPids = [&](ProcState s) {
        QStringList l;
        for (const auto &p : m_procesos) if (p.estado == s) l << QString("P%1").arg(p.pid);
        return l.isEmpty() ? QStringLiteral("-") : l.join(QStringLiteral(", "));
    };

    const QString raya = QStringLiteral("========================================");
    const QString filo = QStringLiteral("----------------------------------------");

    QStringList l;
    l << raya << QStringLiteral("          ESTADO DEL SISTEMA") << raya << QString();

    l << QStringLiteral("PROCESOS") << filo;
    l << QString("New:         %1").arg(listaPids(ProcState::Nuevo));
    l << QString("Ready:       %1").arg(listaPids(ProcState::Listo));
    l << QString("Running:     %1").arg(listaPids(ProcState::Ejecutando));
    l << QString("Waiting:     %1").arg(listaPids(ProcState::Bloqueado));
    l << QString("Terminated:  %1").arg(listaPids(ProcState::Terminado));
    l << QString();

    const double libre = memoriaLibreTotal(m_bloques);
    const double ocupada = m_memoriaTotalMB - libre;
    QStringList huecosTxt;
    for (double h : huecosLibres(m_bloques)) huecosTxt << QString::number(qRound(h));

    l << QStringLiteral("MEMORIA") << filo;
    l << QString("Total:       %1").arg(formatoMemoria(m_memoriaTotalMB));
    l << QString("Ocupada:     %1").arg(formatoMemoria(ocupada));
    l << QString("Libre:       %1").arg(formatoMemoria(libre));
    l << QString("Metodo:      %1").arg(metodoATexto(m_metodoAjuste));
    l << QString("Huecos:      [%1]").arg(huecosTxt.join(QStringLiteral(", ")));
    l << QString();

    l << QStringLiteral("RECURSOS") << filo;
    for (const auto &r : catalogoRecursos()) {
        QString valor = QStringLiteral("Disponible");
        for (const auto &p : m_procesos) {
            if (p.estado != ProcState::Terminado && p.recursosAsignados.contains(r)) {
                valor = QString("P%1").arg(p.pid);
                break;
            }
        }
        l << QString("%1: %2").arg(r.leftJustified(12), valor);
    }
    l << QString();

    l << QStringLiteral("INTERBLOQUEO") << filo;
    QVector<int> ciclo;
    if (detectarCicloEspera(&ciclo)) {
        QStringList nombres;
        for (int pid : ciclo) nombres << QString("P%1").arg(pid);
        l << QString("DETECTADO: %1").arg(nombres.join(QStringLiteral(" -> ")));
    } else {
        l << QStringLiteral("No detectado");
    }
    l << QString() << raya;

    m_estadoGeneralLabel->setText(l.join(QStringLiteral("\n")));
}
