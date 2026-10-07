#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QPair>
#include <functional>
#include "process.h"
#include "memorymodel.h"

class QLineEdit;
class QSlider;
class QLabel;
class QVBoxLayout;
class QHBoxLayout;
class QListWidget;
class QPushButton;
class QTimer;
class QFrame;
class QTabWidget;
class QDialog;
class QSpinBox;
class QComboBox;
class StateDiagramWidget;
class TaskManagerWidget;
class CpuStripWidget;
class GanttWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void crearProceso();
    void avanzarTick();
    void reiniciarSimulacion();

    // Transiciones de estado (conectadas a las senales de cada tarjeta)
    void admitir(int pid);
    void despachar(int pid);
    void bloquear(int pid);
    void despertar(int pid);
    void expropiar(int pid);
    void terminar(int pid);
    void mostrarDetallePcb(int pid);

    // --- Acciones de las pestanas de gestion ---
    void menuMostrarProcesos();
    void menuMostrarPcb();
    void menuCambiarEstado();
    void menuMostrarColas();
    void menuFinalizarProceso();

    // --- Administracion de memoria (pestana 2) ---
    void memAsignarManual();
    void memLiberarManual();
    void memProbarFragmentacion();
    void memCompactar();
    void memAplicarConfiguracion();

    // --- Administracion de recursos (pestana 3) ---
    void recAsignarManual();
    void recLiberarManual();
    void recDetectar();

    // --- Paginacion y Sistema (pestana 4) ---
    void calcularPaginacion();

private:
    // --- Tipos de evento de la bitacora (permiten filtrarla) ---
    enum class TipoEvento { Transicion, Planificador, Sistema };
    struct Evento {
        int tick;
        TipoEvento tipo;
        QString texto;
    };

    // --- estado del simulador ---
    QVector<Proceso> m_procesos;
    int m_siguientePid = 1;
    int m_reloj = 0;

    // Ventana deslizante del uso global de CPU (1 = CPU ocupada ese tick)
    QVector<quint8> m_ventanaSistema;

    QVector<Evento> m_eventos;
    TipoEvento m_filtro = TipoEvento::Transicion;
    bool m_filtroTodos = true;

    // --- Memoria fisica: particiones contiguas ---
    QVector<BloqueMemoria> m_bloques;
    MetodoAjuste m_metodoAjuste = MetodoAjuste::FirstFit;
    double m_memoriaTotalMB = MEMORIA_TOTAL_MB;
    double m_memoriaSoMB = MEMORIA_SO_MB;

    // --- widgets ---
    QLineEdit *m_nombreEdit;
    QSlider *m_prioridadSlider;
    QSlider *m_rafagaSlider;
    QLabel *m_prioridadVal;
    QLabel *m_rafagaVal;
    QLabel *m_relojLabel;
    StateDiagramWidget *m_diagrama;
    TaskManagerWidget *m_taskManager;
    CpuStripWidget *m_franjaCpu;
    GanttWidget *m_gantt;
    QFrame *m_panelGantt;
    QListWidget *m_log;
    QPushButton *m_btnGantt;
    QVector<QPushButton *> m_botonesFiltro;
    QTabWidget *m_pestanas = nullptr;

    // --- widgets de la pestana de memoria ---
    QVBoxLayout *m_mapaMemoriaLayout = nullptr;
    QLabel *m_mapaMemoriaTotalLabel = nullptr;
    QLabel *m_fragmentacionInfo = nullptr;
    QSpinBox *m_totalMemSpin = nullptr;
    QSpinBox *m_soMemSpin = nullptr;
    QComboBox *m_metodoCombo = nullptr;
    QSpinBox *m_memoriaNuevoSpin = nullptr;  // memoria requerida del proximo proceso a crear

    // --- widgets de la pestana de recursos ---
    QVBoxLayout *m_recursosLayout = nullptr;
    QLabel *m_recursosResumen = nullptr;
    QLabel *m_interbloqueoInfo = nullptr;

    // --- widgets de la sub-pestana de paginacion ---
    QSpinBox *m_procesoKBSpin = nullptr;
    QSpinBox *m_paginaKBSpin = nullptr;
    QSpinBox *m_marcosSpin = nullptr;
    QVBoxLayout *m_tablaPaginasLayout = nullptr;
    QLabel *m_paginacionResumen = nullptr;

    // --- widget de la sub-pestana de sistema ---
    QLabel *m_estadoGeneralLabel = nullptr;

    QVBoxLayout *m_colNuevo;
    QVBoxLayout *m_colListo;
    QVBoxLayout *m_colBloqueado;
    QVBoxLayout *m_colTerminado;

    QLabel *m_countNuevo;
    QLabel *m_countListo;
    QLabel *m_countBloqueado;
    QLabel *m_countTerminado;

    // --- helpers de construccion ---
    void construirUI();
    QWidget* construirEncabezado();
    QWidget* construirLateral();
    QWidget* construirPestanaGestionProcesos();
    QWidget* construirPestanaMemoria();
    QWidget* construirPestanaRecursos();
    QWidget* construirPestanaExtra();
    QWidget* construirSubPestanaPaginacion();
    QWidget* construirSubPestanaSistema();
    QWidget* construirPanelAcciones(const QString &titulo,
                    const QVector<QPair<QString, std::function<void()>>> &acciones);
    QWidget* construirPestanaColas();
    QWidget* construirBitacora();
    QWidget* construirColumna(const QString &titulo, const QString &nota,
                              ProcState estado,
                              QVBoxLayout *&contenedorSalida, QLabel *&contadorSalida);
    int elegirProceso(const QString &titulo, const QString &etiqueta,
                      const QVector<Proceso *> &candidatos);

    // --- logica ---
    void cambiarEstado(int pid, ProcState nuevo, bool silencioso = false);
    Proceso* buscarProceso(int pid);
    void refrescarTodo();
    void refrescarColas();
    void refrescarDiagrama();
    void refrescarEstadisticas();
    void registrarEvento(const QString &mensaje, TipoEvento tipo = TipoEvento::Transicion);
    void repintarBitacora();
    void limpiarLayout(QVBoxLayout *layout);
    void conectarTarjeta(QWidget *tarjeta);

    // --- memoria fisica (particiones contiguas) ---
    void inicializarMemoria();
    bool asignarMemoriaProceso(Proceso &p);   // intenta reservar su bloque; registra la decision
    void liberarMemoriaProceso(int pid);      // libera el bloque del proceso y fusiona huecos
    void reintentarAdmisionesPendientes();    // reintenta los NUEVO que aun esperan memoria
    void refrescarMapaMemoria();              // repinta el mapa y el resumen de fragmentacion
    QWidget* construirFilaBloque(const BloqueMemoria &b);

    // --- recursos del sistema (exclusion mutua con cola de espera) ---
    bool intentarAsignarRecurso(Proceso &p, const QString &recurso);  // asigna si esta libre; si no, encola la espera
    void liberarRecurso(Proceso &p, const QString &recurso);         // libera y se lo da a quien lo esperaba
    void refrescarRecursos();                 // repinta la tabla de recursos y el resumen
    QWidget* construirFilaRecurso(const QString &recurso);
    bool detectarCicloEspera(QVector<int> *cicloSalida = nullptr) const;  // true si hay interbloqueo

    // --- paginacion (extension conceptual) y estado general ---
    QWidget* construirFilaPagina(int pagina, int marco);
    void refrescarEstadoGeneral();

    // --- estadisticas ---
    void contabilizarTick();     // acumula CPU/espera/bloqueo y mueve la memoria
    double cpuGlobalPorcentaje() const;
};

#endif // MAINWINDOW_H
