#!/bin/bash

echo ""
echo "========================================"
echo "  Optimización de Riego y Polarización - ADA II"
echo "  Compilando interfaz grafica..."
echo "========================================"
echo ""

# Cerrar instancia anterior si esta corriendo
taskkill //F //IM gui.exe >/dev/null 2>&1
if [ -f gui.exe ]; then
    rm -f gui.exe
fi

echo "[1/1] Compilando y enlazando GUI..."
g++ -std=c++17 -IProyectoGUI-Fuentes -o gui.exe ProyectoGUI-Fuentes/gui.cpp -lcomctl32 -lcomdlg32 -lgdi32 -lole32 -luuid -mwindows
if [ $? -ne 0 ]; then
    echo ""
    echo "ERROR: Fallo la compilacion."
    echo "Asegurate de tener g++ instalado y en el PATH."
    echo ""
    read -p "Presiona Enter para salir..."
    exit 1
fi

echo ""
echo "Compilacion exitosa. Iniciando GUI..."
echo ""
./gui.exe &
