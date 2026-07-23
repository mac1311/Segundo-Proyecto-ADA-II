@echo off
title Optimización de Riego y Polarización - ADA II
chcp 65001 >nul

echo.
echo ========================================
echo   Optimización de Riego y Polarización - ADA II
echo   Compilando interfaz grafica...
echo ========================================
echo.

:: Cerrar instancia anterior si esta corriendo
taskkill /F /IM gui.exe >nul 2>&1
if exist gui.exe del /F gui.exe

echo [1/1] Compilando y enlazando GUI...
g++ -std=c++17 -IProyectoGUI-Fuentes -o gui.exe ProyectoGUI-Fuentes\gui.cpp -lcomctl32 -lcomdlg32 -lgdi32 -lole32 -luuid -mwindows
if errorlevel 1 goto error

echo.
echo Compilacion exitosa. Iniciando GUI...
echo.
start "" gui.exe
goto end

:error
echo.
echo ERROR: Fallo la compilacion.
echo Asegurate de tener MinGW-w64 (g++) instalado y en el PATH.
echo.
pause
exit /b 1

:end
