#ifndef MEMORYMODEL_H
#define MEMORYMODEL_H

#include <QString>
#include <QVector>
#include <algorithm>
#include "process.h"

// ============================================================
//  Modelo de memoria principal: particiones contiguas
// ------------------------------------------------------------
//  La memoria fisica se representa como una lista ORDENADA de
//  bloques: el bloque 0 empieza en la direccion 0, y cada bloque
//  es contiguo al anterior. No se guarda una direccion explicita
//  por bloque; la direccion de inicio de un bloque es la suma de
//  los tamanos de los bloques que lo preceden en el vector (ver
//  direccionBloque()). Cada bloque es del sistema operativo, de
//  un proceso, o esta libre.
// ============================================================

enum class TipoBloque { SistemaOperativo, Proceso, Libre };

struct BloqueMemoria {
    TipoBloque tipo = TipoBloque::Libre;
    double tamanoMB = 0.0;
    int pid = -1;          // valido solo si tipo == Proceso
    QString nombre;        // copia del nombre del proceso, para pintar sin buscarlo
};

// --- Metodos de ajuste para elegir el hueco donde asignar ---
enum class MetodoAjuste { FirstFit, BestFit, WorstFit };

inline QString metodoATexto(MetodoAjuste m) {
    switch (m) {
        case MetodoAjuste::FirstFit: return QStringLiteral("First Fit");
        case MetodoAjuste::BestFit:  return QStringLiteral("Best Fit");
        case MetodoAjuste::WorstFit: return QStringLiteral("Worst Fit");
    }
    return QString();
}

// Direccion de inicio del bloque en la posicion 'indice'.
inline double direccionBloque(const QVector<BloqueMemoria> &bloques, int indice) {
    double inicio = 0.0;
    for (int i = 0; i < indice && i < bloques.size(); ++i) inicio += bloques[i].tamanoMB;
    return inicio;
}

// Memoria inicial: un bloque para el sistema operativo y el resto libre.
inline QVector<BloqueMemoria> memoriaInicial(double totalMB, double soMB) {
    QVector<BloqueMemoria> bloques;
    bloques.append({TipoBloque::SistemaOperativo, soMB, -1, QString()});
    const double resto = totalMB - soMB;
    if (resto > 0.0) bloques.append({TipoBloque::Libre, resto, -1, QString()});
    return bloques;
}

// Indice del hueco libre a usar segun el metodo de ajuste, o -1 si
// ningun hueco individual alcanza (aunque la suma de todos si alcance:
// eso es justamente la fragmentacion externa).
inline int buscarHueco(const QVector<BloqueMemoria> &bloques, double tamanoMB, MetodoAjuste metodo) {
    int elegido = -1;
    for (int i = 0; i < bloques.size(); ++i) {
        const auto &b = bloques[i];
        if (b.tipo != TipoBloque::Libre || b.tamanoMB < tamanoMB) continue;
        switch (metodo) {
        case MetodoAjuste::FirstFit:
            return i; // el primero que alcanza: no hace falta seguir buscando
        case MetodoAjuste::BestFit:
            if (elegido < 0 || b.tamanoMB < bloques[elegido].tamanoMB) elegido = i;
            break;
        case MetodoAjuste::WorstFit:
            if (elegido < 0 || b.tamanoMB > bloques[elegido].tamanoMB) elegido = i;
            break;
        }
    }
    return elegido;
}

// Por debajo de este tamano no vale la pena dejar un hueco residual
// (evita astillar la memoria en fragmentos inutiles de un par de MB).
static constexpr double MEMORIA_MIN_HUECO_MB = 8.0;

struct ResultadoAsignacion {
    bool exito = false;
    double huecoOriginalMB = 0.0;  // tamano del hueco elegido, antes de partirlo
    double sobranteMB = 0.0;       // lo que quedo libre despues de partirlo (0 si no sobro nada)
};

// Intenta asignar 'tamanoMB' contiguos al proceso 'pid'/'nombre'. Si hay
// un hueco adecuado lo parte: el proceso se queda con exactamente lo que
// pidio y el resto se deja como un nuevo hueco libre (salvo que el resto
// sea menor al umbral minimo, en cuyo caso se lo queda el proceso completo).
inline ResultadoAsignacion asignarMemoria(QVector<BloqueMemoria> &bloques, double tamanoMB,
                                          int pid, const QString &nombre, MetodoAjuste metodo) {
    ResultadoAsignacion r;
    const int idx = buscarHueco(bloques, tamanoMB, metodo);
    if (idx < 0) return r;

    r.exito = true;
    r.huecoOriginalMB = bloques[idx].tamanoMB;
    const double sobrante = bloques[idx].tamanoMB - tamanoMB;

    bloques[idx].tipo = TipoBloque::Proceso;
    bloques[idx].pid = pid;
    bloques[idx].nombre = nombre;

    if (sobrante > MEMORIA_MIN_HUECO_MB) {
        bloques[idx].tamanoMB = tamanoMB;
        bloques.insert(idx + 1, {TipoBloque::Libre, sobrante, -1, QString()});
        r.sobranteMB = sobrante;
    }
    return r;
}

// Libera el bloque del proceso 'pid' y fusiona huecos libres adyacentes
// (coalescing), tal como haria un asignador de memoria real.
inline void liberarMemoria(QVector<BloqueMemoria> &bloques, int pid) {
    for (auto &b : bloques) {
        if (b.tipo == TipoBloque::Proceso && b.pid == pid) {
            b.tipo = TipoBloque::Libre;
            b.pid = -1;
            b.nombre.clear();
        }
    }
    for (int i = 0; i < bloques.size() - 1; ) {
        if (bloques[i].tipo == TipoBloque::Libre && bloques[i + 1].tipo == TipoBloque::Libre) {
            bloques[i].tamanoMB += bloques[i + 1].tamanoMB;
            bloques.remove(i + 1);
        } else {
            ++i;
        }
    }
}

// Compacta: junta todos los bloques ocupados al inicio de la memoria y
// deja un unico hueco libre al final con toda la memoria disponible.
inline void compactarMemoria(QVector<BloqueMemoria> &bloques) {
    QVector<BloqueMemoria> compactados;
    double libreTotal = 0.0;
    for (const auto &b : bloques) {
        if (b.tipo == TipoBloque::Libre) libreTotal += b.tamanoMB;
        else compactados.append(b);
    }
    if (libreTotal > 0.0) compactados.append({TipoBloque::Libre, libreTotal, -1, QString()});
    bloques = compactados;
}

// Tamanos de todos los huecos libres actuales (para el analisis de fragmentacion).
inline QVector<double> huecosLibres(const QVector<BloqueMemoria> &bloques) {
    QVector<double> huecos;
    for (const auto &b : bloques) if (b.tipo == TipoBloque::Libre) huecos.append(b.tamanoMB);
    return huecos;
}

inline double memoriaLibreTotal(const QVector<BloqueMemoria> &bloques) {
    double total = 0.0;
    for (double h : huecosLibres(bloques)) total += h;
    return total;
}

#endif // MEMORYMODEL_H
