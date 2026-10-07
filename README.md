# Simulador de Procesos — Proyecto 1 (Qt/C++)

Simulador de gestión de procesos: PCB, estados (Nuevo, Listo, Ejecutando,
Bloqueado, Terminado), colas, transiciones y un panel de rendimiento al
estilo del administrador de tareas. C++17, compila con Qt 6 y con Qt 5.15.

## Descargar el ejecutable (sin instalar Qt)

- **Versión publicada:** pestaña [Releases](../../releases/latest) →
  `SimuladorProcesos-windows.zip`.
- **Última versión de `main`:** pestaña [Actions](../../actions) → la ejecución
  más reciente de *Compilar ejecutable de Windows* → sección *Artifacts*.

Se descomprime el ZIP y se abre `simulador-procesos.exe`. Las DLL que vienen
junto al `.exe` son las bibliotecas de Qt: no hay que separarlas.

## Trabajar en equipo

La guía paso a paso (clonar, abrir en Qt Creator, subir y bajar cambios,
publicar una versión) está en [GUIA-EQUIPO.md](GUIA-EQUIPO.md).

Resumen del flujo:

1. **Pull** antes de empezar, para traer lo que subieron los demás.
2. Editar, compilar y probar en Qt Creator.
3. **Commit** con un mensaje claro y **Push**.
4. GitHub compila el proyecto automáticamente en Windows; si algo no compila,
   aparece una ✗ roja junto al commit.

## Archivos

- `theme.h` — paleta y hoja de estilos global. Único lugar donde viven los
  colores: los widgets que pintan con QPainter piden aquí sus `QColor`, y los
  que se estilan con QSS usan `Tema::hojaDeEstilos()`.
- `process.h` — modelo de datos del PCB (`Proceso`, `ProcState`) y helpers de
  estadísticas (% de CPU, tiempos de retorno y respuesta, memoria usada,
  estado en un tick concreto para el Gantt).
- `statediagramwidget.h/.cpp` — ciclo de vida como grafo de 5 nodos, con la
  última transición resaltada y el nodo Ejecutando latiendo mientras la CPU
  está ocupada.
- `pcbcardwidget.h/.cpp` — tarjeta compacta de un PCB: nombre, PID, chips de
  datos, progreso de ráfaga y los botones de transición válidos para su estado.
- `cpustripwidget.h/.cpp` — franja horizontal de CPU + cola de listos ordenada
  por prioridad. Cada elemento de la cola es un botón que despacha ese proceso.
- `ganttwidget.h/.cpp` — línea de tiempo: una fila por proceso, una celda por
  tick, coloreada con el estado que tuvo en ese momento.
- `memorybarwidget.h/.cpp` — mapa de la memoria física por segmentos (SO,
  procesos vivos, hueco libre) con leyenda.
- `usagegraphwidget.h/.cpp` — gráfica de área con historial deslizante.
- `taskmanagerwidget.h/.cpp` — panel de rendimiento: métricas, gráficas en
  vivo, mapa de memoria y tabla de procesos ordenable.
- `mainwindow.h/.cpp` — ventana principal: barra superior, lateral de creación,
  pestañas, bitácora filtrable y diálogo del PCB completo.
- `main.cpp` — punto de entrada.
- `simulador-procesos.pro` — proyecto qmake.
- `scripts/empaquetar-windows.bat` — genera el `.exe` con sus DLL en Windows.
- `.github/workflows/compilar-windows.yml` — compilación automática en GitHub.
- `GUIA-EQUIPO.md` — cómo trabajar el proyecto en grupo con Git y GitHub.

## Compilar

### Qt Creator (Windows/macOS/Linux)
Abrir `simulador-procesos.pro` y presionar **Ejecutar** (Ctrl+R). No requiere
configuración adicional.

### Generar el `.exe` para compartir (Windows)
Doble clic en `scripts\empaquetar-windows.bat`. Busca el Qt instalado (por
defecto en `D:\Programas\QT`; se cambia en la primera línea `set "QT_ROOT=..."`),
compila en modo Release y ejecuta `windeployqt`, que copia junto al `.exe` todas
las DLL de Qt y de MinGW. Deja la carpeta `SimuladorProcesos\` y el archivo
`SimuladorProcesos-windows.zip` en la raíz del proyecto.

Lo mismo se hace en GitHub automáticamente con cada cambio subido a `main`
(ver `.github/workflows/compilar-windows.yml`).

### Linux (qmake)
```bash
sudo apt install qtbase5-dev qt5-qmake build-essential   # si falta Qt
qmake simulador-procesos.pro
make -j4
./simulador-procesos
```

## Cómo se lee la interfaz

Tres decisiones organizan la pantalla:

**La CPU no es una columna.** Es un recurso único, así que ocupa una franja
horizontal sobre las colas: a la izquierda, qué proceso la tiene, con su
progreso de ráfaga y sus acciones; a la derecha, la cola de listos en el
mismo orden que usa el planificador (prioridad, y FCFS como desempate). Las
columnas quedan en cuatro: Nuevo, Listo, Bloqueado, Terminado.

**El color codifica estado, y nada más.** Cinco tonos desaturados, uno por
estado, usados en el filo de cada columna, en el borde izquierdo de la
tarjeta, en el diagrama, en la tabla y en el Gantt. El acento morado queda
reservado para lo interactivo (botones, foco, valores editables).

**El tiempo se hace visible.** La línea de tiempo al pie de la pestaña de
colas convierte el historial de transiciones —hasta ahora escondido en el
diálogo— en un gráfico permanente: las esperas, las ráfagas y los bloqueos
por E/S se leen de un golpe. Se oculta y se muestra con el botón
**"Línea de tiempo"** de la barra superior.

## Uso

1. Escribe un nombre, ajusta prioridad y ráfaga, y presiona **Crear proceso**.
   Pasa automáticamente de *Nuevo* a *Listo*.
2. Despacha desde la cola de listos de la franja de CPU, o usa los botones de
   cada tarjeta (Admitir, Despachar, Bloquear E/S, Expropiar, E/S completa,
   Terminar).
3. Clic en una tarjeta (fuera de los botones) abre el PCB completo: contexto de
   CPU y memoria a la izquierda, tiempos e historial de transiciones a la derecha.
4. **Avanzar tick** simula el paso del reloj: descuenta la ráfaga del proceso en
   ejecución y lo termina al llegar a 0.
5. **Auto** activa un reloj automático (un tick cada 850 ms) con un planificador
   por prioridad que despacha cuando la CPU queda libre.
6. La bitácora inferior filtra por tipo de evento: transiciones, decisiones del
   planificador o mensajes del sistema.

## Pestaña "Rendimiento"

- **Métricas**: uso de CPU y proceso que la ocupa, memoria usada sobre los 8 GB
  simulados, total de procesos y tiempo medio de espera (con el retorno medio de
  los que ya terminaron).
- **Gráficas en vivo** de CPU y memoria; cada tick añade una muestra.
- **Mapa de memoria física**: un segmento por proceso vivo, más el bloque del SO
  y el hueco libre. Solo se rotulan los segmentos donde cabe el texto; el resto
  se identifica en la leyenda.
- **Tabla de procesos** ordenable por cualquier columna (orden numérico, no
  alfabético). Doble clic en una fila abre el PCB; **Finalizar proceso** termina
  el seleccionado.

### Cómo se calculan las métricas

| Métrica | Cálculo |
|---|---|
| **CPU %** | Ventana deslizante de los últimos `VENTANA_CPU` (12) ticks: ticks en los que el proceso ocupó la CPU / tamaño de la ventana. No es el total histórico, por eso un proceso que deja la CPU baja de forma gradual. |
| **Memoria** | Huella simulada que fluctúa cada tick: crece mientras el proceso ejecuta y se mantiene casi estable mientras espera. Se guarda además el pico histórico. Al terminar, el SO libera su memoria. |
| **T. CPU / Espera / Bloq.** | Ticks acumulados en estado Ejecutando, Listo y Bloqueado respectivamente. |
| **Ctx** | Cambios de contexto: una entrada a la CPU cada vez que el proceso pasa a Ejecutando. |
| **E/S** | Número de veces que el proceso solicitó E/S (pasó a Bloqueado). |
| **T. de respuesta** | Primer tick en que tocó la CPU menos el tick de llegada. |
| **T. de retorno** | Tick de finalización menos el tick de llegada (solo si terminó). |

Los parámetros de la máquina simulada (tamaño de la ventana de CPU, memoria
total y consumo del SO) están al inicio de `process.h`; los colores y la
tipografía, al inicio de `theme.h`.
