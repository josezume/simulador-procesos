#ifndef THEME_H
#define THEME_H

#include <QString>
#include <QColor>
#include "process.h"

// ============================================================
//  Tema visual (paleta Nocturne) y hoja de estilos global
// ------------------------------------------------------------
//  Un solo lugar donde viven los colores. Los widgets que se
//  dibujan con QPainter piden aqui sus QColor; los que se estilan
//  con QSS usan la hoja que devuelve hojaDeEstilos().
// ============================================================
namespace Tema {

// --- Fondos y texto ---
inline const char *Fondo      = "#161826";   // ventana
inline const char *Panel      = "#1b1d2c";   // paneles y tarjetas grandes
inline const char *Superficie = "#232532";   // tarjetas PCB, campos, cabeceras
inline const char *Borde      = "#2e3040";   // borde estandar (1px)
inline const char *BordeSuave = "#26283a";   // separadores internos
inline const char *Texto      = "#e9e9ed";

// --- Acento (solo para lo interactivo) ---
inline const char *Acento     = "#9184d9";
inline const char *Acento300  = "#d2cefd";
inline const char *Acento400  = "#b5abfc";
inline const char *Acento700  = "#5d5294";
inline const char *Acento800  = "#423a6a";

// --- Rampa neutra ---
inline const char *Neutro300  = "#cfd3e5";
inline const char *Neutro400  = "#b2b6ca";
inline const char *Neutro500  = "#9397ab";
inline const char *Neutro600  = "#75798c";
inline const char *Neutro700  = "#595d6c";
inline const char *Neutro800  = "#3f424d";
inline const char *Neutro900  = "#292b31";

// --- Un tono por estado: el color codifica estado y nada mas ---
inline const char *EstNuevo      = "#8b8fa6";
inline const char *EstListo      = "#9184d9";
inline const char *EstEjecutando = "#c9a86a";
inline const char *EstBloqueado  = "#6fa8a0";
inline const char *EstTerminado  = "#a8737f";

inline QString colorEstadoHex(ProcState s) {
    switch (s) {
        case ProcState::Nuevo:      return EstNuevo;
        case ProcState::Listo:      return EstListo;
        case ProcState::Ejecutando: return EstEjecutando;
        case ProcState::Bloqueado:  return EstBloqueado;
        case ProcState::Terminado:  return EstTerminado;
    }
    return Neutro600;
}

inline QColor colorEstado(ProcState s) { return QColor(colorEstadoHex(s)); }

// Color en formato rgba() para hojas de estilo (QSS no admite #AARRGGBB).
inline QString rgba(const QString &hex, int alpha) {
    QColor c(hex);
    return QString("rgba(%1,%2,%3,%4)").arg(c.red()).arg(c.green()).arg(c.blue()).arg(alpha);
}

// Mismo color con transparencia, para rellenos tenues.
inline QColor tinte(const QString &hex, int alpha) {
    QColor c(hex);
    c.setAlpha(alpha);
    return c;
}

// Familias tipograficas: Inter si esta instalada, si no las del sistema.
inline const char *FamiliaUI   = "'Inter','Segoe UI','DejaVu Sans',sans-serif";
inline const char *FamiliaMono = "'DejaVu Sans Mono','Consolas','Menlo',monospace";

// ------------------------------------------------------------
//  Hoja de estilos global de la aplicacion
// ------------------------------------------------------------
inline QString hojaDeEstilos() {
    // Sustitucion por nombre en lugar de %1..%21: con una veintena de
    // marcadores, QString::arg() se desalinea en cuanto uno queda sin usar.
    QString qss = QStringLiteral(R"(
        QMainWindow, QWidget { background:$fondo; color:$texto;
            font-family:$ui; font-size:12.5px; }

        /* --- Campos --- */
        QLineEdit { background:$superficie; border:1px solid $borde; border-radius:8px;
            padding:7px 10px; color:$texto; font-family:$mono; font-size:12.5px; }
        QLineEdit:hover { border-color:$n500; }
        QLineEdit:focus { border-color:$acento; }

        /* --- Botones: contorno, nunca relleno --- */
        QPushButton { background:transparent; border:1px solid $borde; border-radius:8px;
            padding:8px 13px; color:$texto; font-weight:500; font-size:12.5px; }
        QPushButton:hover { background:rgba(233,233,237,18); border-color:$n500; }
        QPushButton:pressed { background:rgba(233,233,237,30); }
        QPushButton#primary { color:$acento; border:1px solid $acento; }
        QPushButton#primary:hover { background:rgba(145,132,217,30); }
        QPushButton#primary:pressed { background:rgba(145,132,217,56); }
        QPushButton#danger { color:$terminado; border:1px solid rgba(168,115,127,140); }
        QPushButton#danger:hover { background:rgba(168,115,127,36); }
        QPushButton#chip { padding:4px 8px; font-size:10.5px; border-radius:5px;
            border:1px solid $borde; color:$n300; }
        QPushButton#chip:hover { background:rgba(233,233,237,20); border-color:$n500; }
        QPushButton:focus { outline:none; }

        /* --- Etiquetas de seccion --- */
        QLabel#sectionLabel { color:$n600; font-family:$mono; font-size:9.5px;
            font-weight:600; letter-spacing:1.6px; }
        QLabel#brand { font-size:15px; font-weight:500; }
        QLabel#sub { color:$n600; font-size:10.5px; font-family:$mono; }
        QLabel#metric { font-family:$mono; font-size:16px; font-weight:600; }
        QLabel#metricCap { color:$n600; font-family:$mono; font-size:9px;
            font-weight:600; letter-spacing:1.4px; }

        /* --- Deslizadores --- */
        QSlider::groove:horizontal { height:4px; background:$n900; border-radius:2px; }
        QSlider::sub-page:horizontal { height:4px; background:$acento700; border-radius:2px; }
        QSlider::handle:horizontal { width:14px; height:14px; margin:-5px 0;
            background:$acento; border-radius:7px; }
        QSlider::handle:horizontal:hover { background:$acento400; }

        /* --- Pestanas --- */
        QTabWidget::pane { border:none; background:$fondo; }
        QTabBar { background:transparent; }
        QTabBar::tab { background:transparent; border:none; color:$n600;
            padding:9px 16px; margin-right:2px; font-size:12.5px; font-weight:500; }
        QTabBar::tab:selected { color:$texto; background:$fondo;
            border:1px solid $borde; border-bottom-color:$fondo;
            border-top-left-radius:8px; border-top-right-radius:8px; }
        QTabBar::tab:hover:!selected { color:$texto; }

        /* --- Tabla de procesos --- */
        QTableWidget { background:$panel; alternate-background-color:$panel;
            border:1px solid $borde; border-radius:8px; gridline-color:transparent;
            font-family:$mono; font-size:11px;
            selection-background-color:$acento800; selection-color:$texto; }
        QTableWidget::item { padding:3px 6px; border:none;
            border-bottom:1px solid $bordeSuave; }
        QHeaderView::section { background:$superficie; color:$n400; border:none;
            border-bottom:1px solid $borde; padding:6px 5px;
            font-size:9.5px; font-weight:600; letter-spacing:0.6px; }
        QHeaderView::section:hover { color:$acento; }

        /* --- Bitacora --- */
        QListWidget { background:$panel; border:1px solid $borde; border-radius:8px;
            font-family:$mono; font-size:11px; color:$n400; outline:none; }
        QListWidget::item { padding:3px 6px; }
        QListWidget::item:selected { background:$acento800; color:$texto; }

        QScrollArea { border:none; background:$fondo; }
        QToolTip { background:$superficie; color:$texto; border:1px solid $borde; padding:4px 6px; }

        /* --- Barras de desplazamiento --- */
        QScrollBar:vertical { background:transparent; width:9px; margin:2px; }
        QScrollBar::handle:vertical { background:$n700; border-radius:4px; min-height:24px; }
        QScrollBar::handle:vertical:hover { background:$n500; }
        QScrollBar:horizontal { background:transparent; height:9px; margin:2px; }
        QScrollBar::handle:horizontal { background:$n700; border-radius:4px; min-width:24px; }
        QScrollBar::handle:horizontal:hover { background:$n500; }
        QScrollBar::add-line, QScrollBar::sub-line { height:0; width:0; }
        QScrollBar::add-page, QScrollBar::sub-page { background:transparent; }

        /* --- Menu principal (arriba a la izquierda) --- */
        QToolButton#menuPrincipal { background:transparent; border:1px solid $borde;
            border-radius:8px; padding:8px 13px; color:$texto; font-weight:500; font-size:12.5px; }
        QToolButton#menuPrincipal:hover { background:rgba(233,233,237,18); border-color:$n500; }
        QToolButton#menuPrincipal::menu-indicator { image:none; }

        QMenu { background:$panel; border:1px solid $borde; border-radius:8px;
            padding:6px; color:$texto; }
        QMenu::item { padding:7px 24px 7px 12px; border-radius:6px; font-size:12px; }
        QMenu::item:selected { background:$acento800; color:$texto; }
        QMenu::item:disabled { color:$n600; font-family:$mono; font-size:9.5px;
            font-weight:600; letter-spacing:1.4px; padding-top:10px; padding-bottom:4px; }
        QMenu::separator { height:1px; background:$bordeSuave; margin:6px 4px; }
    )");

    // El orden importa: los nombres mas largos primero, para que
    // "$acento700" no lo pise "$acento".
    const QVector<QPair<QString, QString>> tokens = {
        {"$bordeSuave", BordeSuave}, {"$superficie", Superficie},
        {"$acento300", Acento300},   {"$acento400", Acento400},
        {"$acento700", Acento700},   {"$acento800", Acento800},
        {"$terminado", EstTerminado},
        {"$fondo", Fondo},   {"$panel", Panel},   {"$borde", Borde},
        {"$texto", Texto},   {"$acento", Acento},
        {"$n300", Neutro300}, {"$n400", Neutro400}, {"$n500", Neutro500},
        {"$n600", Neutro600}, {"$n700", Neutro700}, {"$n900", Neutro900},
        {"$mono", FamiliaMono}, {"$ui", FamiliaUI}
    };
    for (const auto &tk : tokens) qss.replace(tk.first, tk.second);
    return qss;
}

} // namespace Tema

#endif // THEME_H
