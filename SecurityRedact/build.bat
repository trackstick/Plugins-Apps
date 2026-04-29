@echo off
setlocal EnableDelayedExpansion

echo.
echo SecurityRedact - Build mit MinGW
echo ==================================
echo.

:: MinGW suchen
set "GXX="
for %%P in (
    "C:\mingw64\bin\g++.exe"
    "C:\MinGW\bin\g++.exe"
    "C:\msys64\mingw64\bin\g++.exe"
    "C:\msys64\ucrt64\bin\g++.exe"
    "C:\msys2\mingw64\bin\g++.exe"
) do (
    if not defined GXX (
        if exist %%P set "GXX=%%~P"
    )
)
if not defined GXX (
    where g++ >nul 2>&1
    if !errorlevel! == 0 set "GXX=g++"
)
if not defined GXX (
    echo [FEHLER] g++ nicht gefunden!
    echo Bitte MinGW installieren: winget install -e --id MSYS2.MSYS2
    echo Dann in MSYS2: pacman -S mingw-w64-x86_64-gcc
    pause
    exit /b 1
)
echo [OK] Compiler: !GXX!

:: Verzeichnisse
set "SRC=%~dp0src"
set "OUTDIR=%~dp0bin64"
if not exist "%OUTDIR%" mkdir "%OUTDIR%"
set "OUT=%OUTDIR%\SecurityRedact.dll"

:: Ressourcen kompilieren
set "RES_FILE="
for %%W in (
    "C:\mingw64\bin\windres.exe"
    "C:\msys64\mingw64\bin\windres.exe"
    "C:\msys64\ucrt64\bin\windres.exe"
) do (
    if not defined WINDRES (
        if exist %%W set "WINDRES=%%~W"
    )
)
if defined WINDRES (
    echo [..] Ressourcen kompilieren...
    "!WINDRES!" "%SRC%\SecurityRedact.rc" -O coff -o "%OUTDIR%\SecurityRedact.res"
    if !errorlevel! == 0 set "RES_FILE=%OUTDIR%\SecurityRedact.res"
)

:: DLL kompilieren
echo [..] Kompiliere DLL...

if defined RES_FILE (
    "!GXX!" -std=c++17 -O2 -shared -DUNICODE -D_UNICODE -DWIN32 -D_WINDOWS -D_CRT_SECURE_NO_WARNINGS -I"%SRC%" "%SRC%\SecurityRedact.cpp" "%SRC%\PluginDefinition.cpp" "!RES_FILE!" -o "%OUT%" -static -static-libgcc -static-libstdc++ -luser32 -lkernel32 -lshlwapi
) else (
    "!GXX!" -std=c++17 -O2 -shared -DUNICODE -D_UNICODE -DWIN32 -D_WINDOWS -D_CRT_SECURE_NO_WARNINGS -I"%SRC%" "%SRC%\SecurityRedact.cpp" "%SRC%\PluginDefinition.cpp" -o "%OUT%" -static -static-libgcc -static-libstdc++ -luser32 -lkernel32 -lshlwapi
)

if !errorlevel! neq 0 (
    echo.
    echo [FEHLER] Kompilierung fehlgeschlagen!
    pause
    exit /b 1
)

echo [OK] DLL erstellt: %OUT%
echo.

:: Installieren
set "NPP=%PROGRAMFILES%\Notepad++\plugins\SecurityRedact"
set "NPP32=%PROGRAMFILES(x86)%\Notepad++\plugins\SecurityRedact"

if exist "%PROGRAMFILES%\Notepad++\notepad++.exe" (
    if not exist "%NPP%" mkdir "%NPP%"
    copy /Y "%OUT%" "%NPP%\SecurityRedact.dll" >nul
    if !errorlevel! == 0 (
        echo [OK] Installiert: %NPP%
    ) else (
        echo [!!] Kopieren fehlgeschlagen - Als Administrator ausfuehren!
    )
) else if exist "%PROGRAMFILES(x86)%\Notepad++\notepad++.exe" (
    if not exist "%NPP32%" mkdir "%NPP32%"
    copy /Y "%OUT%" "%NPP32%\SecurityRedact.dll" >nul
    echo [OK] Installiert: %NPP32%
) else (
    echo [!!] Notepad++ nicht gefunden.
    echo      DLL manuell kopieren nach:
    echo      C:\Program Files\Notepad++\plugins\SecurityRedact\SecurityRedact.dll
)

echo.
echo Fertig! Notepad++ neu starten.
echo.
pause
