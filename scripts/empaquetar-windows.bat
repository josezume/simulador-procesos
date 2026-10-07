@echo off
rem ===========================================================================
rem  Genera el ejecutable del Simulador de Procesos para compartirlo, sin que
rem  quien lo reciba necesite tener Qt instalado.
rem
rem  Uso: doble clic sobre este archivo (o ejecutarlo desde cmd).
rem  Resultado: la carpeta SimuladorProcesos\ y el archivo
rem             SimuladorProcesos-windows.zip en la raíz del proyecto.
rem
rem  Si tu Qt está instalado en otra carpeta, cambia QT_ROOT abajo.
rem ===========================================================================
setlocal EnableDelayedExpansion

rem --- Carpeta donde está instalado Qt (la que contiene 6.x.x y Tools) ------
set "QT_ROOT=D:\Programas\QT"
if not exist "%QT_ROOT%" set "QT_ROOT=C:\Qt"

rem --- Busca la versión de Qt con MinGW más reciente ------------------------
set "QT_DIR="
for /d %%V in ("%QT_ROOT%\6.*") do (
    for /d %%K in ("%%V\mingw*") do (
        if exist "%%K\bin\qmake.exe" set "QT_DIR=%%K"
    )
)
if not defined QT_DIR (
    echo [ERROR] No encontre Qt con MinGW dentro de "%QT_ROOT%".
    echo         Abre este archivo con el Bloc de notas y corrige QT_ROOT.
    goto :fallo
)

rem --- Busca el compilador MinGW que trae Qt (carpeta Tools) ----------------
set "MINGW_DIR="
for /d %%M in ("%QT_ROOT%\Tools\mingw*") do (
    if exist "%%M\bin\mingw32-make.exe" set "MINGW_DIR=%%M"
)
if not defined MINGW_DIR (
    echo [ERROR] No encontre MinGW en "%QT_ROOT%\Tools".
    goto :fallo
)

echo Qt:     %QT_DIR%
echo MinGW:  %MINGW_DIR%
echo.
set "PATH=%QT_DIR%\bin;%MINGW_DIR%\bin;%PATH%"

rem --- Compila en modo Release en una carpeta aparte ------------------------
cd /d "%~dp0.."
set "RAIZ=%CD%"
if exist build-release rmdir /s /q build-release
if exist SimuladorProcesos rmdir /s /q SimuladorProcesos
if exist SimuladorProcesos-windows.zip del /q SimuladorProcesos-windows.zip
mkdir build-release
cd build-release

echo === Compilando (puede tardar un par de minutos) ===
qmake "%RAIZ%\simulador-procesos.pro" CONFIG+=release CONFIG-=debug_and_release || goto :fallo
mingw32-make -j%NUMBER_OF_PROCESSORS% || goto :fallo

set "EXE="
if exist "release\simulador-procesos.exe" set "EXE=release\simulador-procesos.exe"
if exist "simulador-procesos.exe" set "EXE=simulador-procesos.exe"
if not defined EXE (
    echo [ERROR] No se genero simulador-procesos.exe
    goto :fallo
)

rem --- Copia el .exe y las DLL de Qt que necesita ---------------------------
echo.
echo === Empaquetando con windeployqt ===
mkdir "%RAIZ%\SimuladorProcesos"
copy /y "%EXE%" "%RAIZ%\SimuladorProcesos\" >nul
windeployqt --release --no-translations --no-system-d3d-compiler --no-opengl-sw --compiler-runtime "%RAIZ%\SimuladorProcesos\simulador-procesos.exe" || goto :fallo

cd /d "%RAIZ%"
powershell -NoProfile -Command "Compress-Archive -Path 'SimuladorProcesos' -DestinationPath 'SimuladorProcesos-windows.zip' -Force" || goto :fallo

echo.
echo ===========================================================================
echo  Listo.
echo  - Carpeta:  %RAIZ%\SimuladorProcesos
echo  - ZIP:      %RAIZ%\SimuladorProcesos-windows.zip
echo  Comparte el ZIP: se descomprime y se abre simulador-procesos.exe
echo ===========================================================================
explorer "%RAIZ%"
pause
exit /b 0

:fallo
echo.
echo La generacion del ejecutable fallo. Revisa los mensajes de arriba.
pause
exit /b 1
