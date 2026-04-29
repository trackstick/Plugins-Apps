@echo off
setlocal EnableDelayedExpansion

echo.
echo ╔══════════════════════════════════════════╗
echo ║   SecurityRedact – Build mit MinGW       ║
echo ╚══════════════════════════════════════════╝
echo.

:: ── MinGW suchen ─────────────────────────────────────────────────────────────
set "GXX="

:: Typische Installationspfade durchsuchen
for %%P in (
    "C:\mingw64\bin\g++.exe"
    "C:\MinGW\bin\g++.exe"
    "C:\MinGW64\bin\g++.exe"
    "C:\msys64\mingw64\bin\g++.exe"
    "C:\msys64\ucrt64\bin\g++.exe"
    "C:\msys2\mingw64\bin\g++.exe"
    "C:\Program Files\mingw-w64\x86_64-8.1.0-posix-seh-rt_v6-rev0\mingw64\bin\g++.exe"
) do (
    if exist %%P (
        set "GXX=%%~P"
        goto :found
    )
)

:: Auch im PATH suchen
where g++ >nul 2>&1
if !errorlevel! == 0 (
    set "GXX=g++"
    goto :found
)

:: Nicht gefunden
echo [FEHLER] g++ nicht gefunden!
echo.
echo MinGW-w64 ist nicht installiert oder nicht im PATH.
echo.
echo Bitte installieren:
echo.
echo   Methode 1 – winget (empfohlen):
echo     winget install -e --id MSYS2.MSYS2
echo     Dann in MSYS2-Terminal:  pacman -S mingw-w64-x86_64-gcc
echo.
echo   Methode 2 – direkt:
echo     https://github.com/niXman/mingw-builds-binaries/releases
echo     (mingw64-...-release-posix-seh-ucrt-rt.7z herunterladen)
echo     Nach C:\mingw64 entpacken und C:\mingw64\bin zum PATH hinzufügen
echo.
echo   Methode 3 – winget direkt:
echo     winget install -e --id GnuWin32.Make
echo     winget install mingw
echo.
pause
exit /b 1

:found
echo [OK] Compiler gefunden: !GXX!

:: ── Ausgabeverzeichnis ────────────────────────────────────────────────────────
set "OUTDIR=%~dp0bin64"
if not exist "%OUTDIR%" mkdir "%OUTDIR%"

set "OUT=%OUTDIR%\SecurityRedact.dll"
set "SRC=%~dp0src"

echo [..] Kompiliere ...
echo.

:: ── Ressourcen-Datei (.rc) kompilieren ───────────────────────────────────────
set "WINDRES="
for %%P in (
    "C:\mingw64\bin\windres.exe"
    "C:\MinGW\bin\windres.exe"
    "C:\msys64\mingw64\bin\windres.exe"
    "C:\msys64\ucrt64\bin\windres.exe"
    "C:\msys2\mingw64\bin\windres.exe"
) do (
    if exist %%P (
        set "WINDRES=%%~P"
        goto :windres_found
    )
)
where windres >nul 2>&1
if !errorlevel! == 0 set "WINDRES=windres"

:windres_found
set "RC_OBJ="
if defined WINDRES (
    echo [..] Kompiliere Ressourcen (Version) ...
    "!WINDRES!" "%SRC%\SecurityRedact.rc" -O coff -o "%OUTDIR%\SecurityRedact.res" 2>&1
    if !errorlevel! == 0 (
        set "RC_OBJ=%OUTDIR%\SecurityRedact.res"
        echo [OK] Ressourcen kompiliert
    ) else (
        echo [!!] Ressourcen fehlgeschlagen, wird uebersprungen
    )
) else (
    echo [!!] windres nicht gefunden - Versionsnummer wird nicht eingebettet
)
echo.

:: ── Kompilieren ───────────────────────────────────────────────────────────────
"!GXX!" ^
  -std=c++17 ^
  -O2 ^
  -shared ^
  -DUNICODE -D_UNICODE ^
  -DWIN32 -D_WINDOWS ^
  -D_CRT_SECURE_NO_WARNINGS ^
  -I"%SRC%" ^
  "%SRC%\SecurityRedact.cpp" ^
  "%SRC%\PluginDefinition.cpp" ^
  !RC_OBJ! ^
  -o "%OUT%" ^
  -static -static-libgcc -static-libstdc++ ^
  -luser32 -lkernel32 -lshlwapi ^
  2>&1

if !errorlevel! neq 0 (
    echo.
    echo [FEHLER] Kompilierung fehlgeschlagen! Siehe Fehlermeldungen oben.
    pause
    exit /b 1
)

echo.
echo [OK] DLL erstellt: %OUT%
echo.

:: ── Installieren ─────────────────────────────────────────────────────────────
set "NPP64=%PROGRAMFILES%\Notepad++\plugins\SecurityRedact"
set "NPP32=%PROGRAMFILES(x86)%\Notepad++\plugins\SecurityRedact"

set "INSTALLED=0"

if exist "%PROGRAMFILES%\Notepad++\notepad++.exe" (
    echo [..] Kopiere nach %NPP64% ...
    if not exist "%NPP64%" mkdir "%NPP64%" 2>nul
    copy /Y "%OUT%" "%NPP64%\SecurityRedact.dll" >nul 2>&1
    if !errorlevel! == 0 (
        echo [OK] Installiert in: %NPP64%
        set "INSTALLED=1"
    ) else (
        echo [!!] Kopieren fehlgeschlagen – bitte als Administrator neu starten:
        echo      Rechtsklick auf build.bat → "Als Administrator ausführen"
    )
)

if exist "%PROGRAMFILES(x86)%\Notepad++\notepad++.exe" (
    echo [..] Kopiere nach %NPP32% ...
    if not exist "%NPP32%" mkdir "%NPP32%" 2>nul
    copy /Y "%OUT%" "%NPP32%\SecurityRedact.dll" >nul 2>&1
    if !errorlevel! == 0 (
        echo [OK] Installiert in: %NPP32%
        set "INSTALLED=1"
    )
)

if "!INSTALLED!" == "0" (
    echo.
    echo [!!] Notepad++ nicht automatisch gefunden.
    echo      DLL manuell kopieren nach:
    echo      C:\Program Files\Notepad++\plugins\SecurityRedact\SecurityRedact.dll
    echo.
    echo      Dann: Rechtsklick auf DLL → Eigenschaften → "Zulassen" aktivieren
)

echo.
echo ════════════════════════════════════════════
echo  Fertig! Notepad++ neu starten.
echo  Plugins → SecurityRedact
echo ════════════════════════════════════════════
echo.
pause
