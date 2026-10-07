#ifndef PROCESS_H
#define PROCESS_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QPair>
#include <array>

// ============================================================
//  Parámetros de la "máquina" simulada
// ============================================================
// Ventana deslizante usada para calcular el % de CPU (como el
// administrador de tareas, que promedia el uso reciente y no el total).
static constexpr int VENTANA_CPU = 12;

// Memoria física simulada del sistema y consumo del propio SO.
static constexpr double MEMORIA_TOTAL_MB = 8192.0;
static constexpr double MEMORIA_SO_MB    = 1850.0;

// Estados posibles de un proceso (ciclo de vida clásico de un SO)
enum class ProcState {
    Nuevo,
    Listo,
    Ejecutando,
    Bloqueado,
    Terminado
};

inline QString estadoATexto(ProcState s) {
    switch (s) {
        case ProcState::Nuevo:      return "Nuevo";
        case ProcState::Listo:      return "Listo";
        case ProcState::Ejecutando: return "Ejecutando";
        case ProcState::Bloqueado:  return "Bloqueado";
        case ProcState::Terminado:  return "Terminado";
    }
    return "Desconocido";
}

// Un evento de la bitácora de transiciones (para el historial del PCB)
struct TransicionHistorial {
    ProcState estado;
    int tick;
};

// Bloque de Control de Proceso (PCB)
struct Proceso {
    int pid = 0;
    QString nombre;
    int prioridad = 0;              // 9 = más alta, 0 = más baja
    ProcState estado = ProcState::Nuevo;

    // Contexto simulado de CPU
    quint32 contadorPrograma = 0;   // Program Counter (PC)
    std::array<quint32, 4> registros{}; // R0..R3

    // Gestión de memoria simulada
    quint32 baseMemoria = 0;
    quint32 limiteMemoriaKB = 0;

    // Memoria requerida vs. memoria realmente asignada por el SO.
    // "requerida" es lo que el proceso pidio al nacer; "asignada" indica
    // si el SO ya se la otorgo (false mientras esta en NUEVO o tras TERMINAR).
    double memoriaRequeridaMB = 0.0;
    bool memoriaAsignada = false;

    // Recursos (dispositivos) del PCB: los que el proceso tiene en uso y
    // los que esta esperando que el SO le conceda (peticion pendiente).
    QStringList recursosAsignados;
    QStringList recursosSolicitados;

    // Planificación
    int rafagaTotal = 0;
    int rafagaRestante = 0;
    int tickLlegada = 0;

    QVector<TransicionHistorial> historial;

    // --- Estadísticas de rendimiento (estilo administrador de tareas) ---
    int ticksCpu = 0;               // ticks realmente ejecutados en la CPU
    int ticksEspera = 0;            // ticks acumulados en la cola de Listos
    int ticksBloqueado = 0;         // ticks acumulados esperando E/S
    int cambiosContexto = 0;        // veces que entró a la CPU
    int operacionesES = 0;          // veces que se bloqueó por E/S
    int hilos = 1;                  // hilos simulados del proceso
    int tickPrimeraEjecucion = -1;  // para el tiempo de respuesta
    int tickFin = -1;               // para el tiempo de retorno

    double memoriaMB = 0.0;         // huella de memoria actual (fluctúa)
    double memoriaPicoMB = 0.0;     // pico histórico de memoria

    // Ventana deslizante: 1 = usó la CPU en ese tick, 0 = no la usó
    QVector<quint8> ventanaCpu;
};

// ------------------------------------------------------------
//  Helpers de estadísticas
// ------------------------------------------------------------

// Registra si el proceso ocupó (o no) la CPU durante el último tick.
inline void registrarMuestraCpu(Proceso &p, bool usoCpu) {
    p.ventanaCpu.append(usoCpu ? 1 : 0);
    while (p.ventanaCpu.size() > VENTANA_CPU)
        p.ventanaCpu.removeFirst();
}

// % de CPU del proceso dentro de la ventana reciente (0..100).
inline double cpuPorcentaje(const Proceso &p) {
    if (p.estado == ProcState::Terminado || p.ventanaCpu.isEmpty()) return 0.0;
    int usados = 0;
    for (quint8 v : p.ventanaCpu) usados += v;
    return 100.0 * usados / double(p.ventanaCpu.size());
}

// Tiempo de retorno (turnaround): solo definido si el proceso terminó.
inline int tiempoRetorno(const Proceso &p) {
    return p.tickFin >= 0 ? p.tickFin - p.tickLlegada : -1;
}

// Tiempo de respuesta: desde que llegó hasta que tocó la CPU por primera vez.
inline int tiempoRespuesta(const Proceso &p) {
    return p.tickPrimeraEjecucion >= 0 ? p.tickPrimeraEjecucion - p.tickLlegada : -1;
}

// Memoria física ocupada por el SO + los procesos vivos.
inline double memoriaUsadaMB(const QVector<Proceso> &procesos) {
    double total = MEMORIA_SO_MB;
    for (const auto &p : procesos)
        if (p.estado != ProcState::Terminado) total += p.memoriaMB;
    return total;
}

inline QString formatoMemoria(double mb) {
    if (mb >= 1024.0) return QString::number(mb / 1024.0, 'f', 2) + " GB";
    return QString::number(mb, 'f', 1) + " MB";
}

// ------------------------------------------------------------
//  Recursos (dispositivos) que un proceso puede solicitar/tener
// ------------------------------------------------------------
inline const QStringList& catalogoRecursos() {
    static const QStringList recursos = {
        "Disco", "Impresora", "Escaner", "Tarjeta de red", "Unidad USB"
    };
    return recursos;
}

// Texto legible de una lista de recursos ("Ninguno" si esta vacia).
inline QString textoRecursos(const QStringList &lista) {
    return lista.isEmpty() ? QStringLiteral("Ninguno") : lista.join(", ");
}

// ------------------------------------------------------------
//  Estado del proceso en un tick concreto (para el diagrama de Gantt)
// ------------------------------------------------------------
// El historial guarda cada transición con su tick. El estado durante el
// tick t es el de la última transición cuyo tick sea <= t. Devuelve false
// si el proceso todavía no existía en ese momento.
inline bool estadoEnTick(const Proceso &p, int t, ProcState &salida) {
    if (t < p.tickLlegada || p.historial.isEmpty()) return false;
    bool encontrado = false;
    for (const auto &h : p.historial) {
        if (h.tick > t) break;
        salida = h.estado;
        encontrado = true;
    }
    return encontrado;
}

#endif // PROCESS_H
