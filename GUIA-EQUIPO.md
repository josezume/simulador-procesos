# Guía para trabajar el proyecto en equipo

El código vive en GitHub: <https://github.com/josezume/simulador-procesos>.
Ahí está siempre la versión más reciente, así que ya no hace falta pasarse ZIPs
por chat.

## 1. Preparación (una sola vez)

1. **Cuenta de GitHub.** Cada integrante crea una en <https://github.com/signup>.
2. **Acceso al repositorio.** El dueño del repositorio entra a
   *Settings → Collaborators → Add people* y agrega a cada integrante por su
   usuario. A cada uno le llega una invitación por correo que debe aceptar.
3. **GitHub Desktop.** Es la forma más sencilla de usar Git en Windows:
   descargarlo de <https://desktop.github.com> e iniciar sesión con la cuenta
   de GitHub.
4. **Clonar el proyecto.** En GitHub Desktop: *File → Clone repository* →
   pestaña *GitHub.com* → elegir `simulador-procesos` → escoger la carpeta
   local (por ejemplo `Documentos\GitHub\simulador-procesos`) → *Clone*.
5. **Abrirlo en Qt Creator.** *File → Open File or Project…* →
   `simulador-procesos.pro` → seleccionar el kit *Desktop Qt 6.x MinGW 64-bit*
   → *Configure Project*. Con **Ejecutar** (Ctrl+R) debe compilar y abrir.

Qt Creator crea un archivo `simulador-procesos.pro.user` con la configuración
de cada computadora. Ese archivo **no** se sube (está en `.gitignore`), así que
cada quien conserva su propia configuración.

## 2. Flujo de cada día

| Paso | En GitHub Desktop | Para qué |
|---|---|---|
| Antes de empezar | **Fetch origin** y luego **Pull origin** | Traer lo que subieron los demás |
| Mientras trabajas | (nada; se trabaja en Qt Creator) | Editar, compilar y probar |
| Al terminar algo | Escribir un resumen abajo a la izquierda → **Commit to main** | Guardar el cambio en tu compu |
| Para compartirlo | **Push origin** | Subirlo a GitHub para todos |

Buenas prácticas para no pisarse el trabajo:

- **Pull antes de empezar y push seguido.** Mientras más tiempo pase entre
  ambos, más probable es que haya conflictos.
- **Commits pequeños y con mensaje claro**, por ejemplo
  `Corrige el color de Bloqueado en el Gantt`, no `cambios`.
- **Avisen en el grupo qué archivo van a tocar**, sobre todo `mainwindow.cpp`,
  que es el más grande y donde es más fácil chocar.
- **Solo suban código que compile.** GitHub lo compila solo después de cada push:
  si aparece una ✗ roja junto al commit, algo quedó roto.

## 3. Si aparece un conflicto

Pasa cuando dos personas cambiaron las mismas líneas. GitHub Desktop avisa y
muestra los archivos en conflicto. Dentro del archivo se ven marcas así:

```
<<<<<<< HEAD
versión de tu compu
=======
versión que subió tu compañero
>>>>>>> origin/main
```

Hay que dejar el código correcto (una de las dos versiones o una mezcla),
**borrar las tres líneas de marcas**, compilar para confirmar y luego hacer
*Commit* y *Push*. Si hay duda, mejor hablarlo con quien hizo el otro cambio.

## 4. Cambios grandes: ramas (opcional)

Para algo que tomará varios días: *Current branch → New branch* (por ejemplo
`planificador-round-robin`), trabajar ahí con commits y push normales, y al
terminar *Create Pull Request*. GitHub compila la rama y, si todo está bien, se
une a `main` con *Merge pull request*.

## 5. El ejecutable para el ingeniero

GitHub genera el `.exe` (con todas las DLL de Qt) automáticamente:

- **Con cada push a `main`:** pestaña *Actions* → la ejecución más reciente →
  sección *Artifacts* → `SimuladorProcesos-windows`. Sirve para probar entre
  ustedes; para descargarlo hay que tener sesión iniciada en GitHub.
- **Para una entrega:** pestaña *Releases* → *Draft a new release* → en
  *Choose a tag* escribir `v1.0` (o la versión que toque) y elegir
  *Create new tag* → *Publish release*. Unos 3–5 minutos después aparece
  `SimuladorProcesos-windows.zip` adjunto al Release.

El enlace para el ingeniero es siempre el mismo y lleva a la última versión
publicada: <https://github.com/josezume/simulador-procesos/releases/latest>.
Como el repositorio es público, lo puede descargar sin cuenta de GitHub. (Si
algún día lo hacen privado, tendrán que enviarle el ZIP por correo o Drive, o
agregarlo como colaborador.)

También se puede generar en la propia computadora con
`scripts\empaquetar-windows.bat` (ver el README).
