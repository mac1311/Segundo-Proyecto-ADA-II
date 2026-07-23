// ============================================================
// GUI — Sistema de Optimizacion de Mediana y Polarizacion (MinPol)
// Win32 API c/ Visual Styles (ComCtl32 v6)
// ============================================================

#if defined(_MSC_VER)
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <chrono>
#include "MinPolData.h"
#include "Timer.h"

using namespace std;

// ─── IDs de controles ──────────────────────────────────────
#define ID_BTN_RESOLVER   1001
#define ID_BTN_RELOAD     1008
#define ID_FILE_OPEN_DIR  2001

#define ID_FILE_EXIT      2002
#define ID_HELP_ABOUT     2003

#define ID_LIST_ARCHIVOS  3000   // ListBox con los archivos y carpetas
#define ID_LIST_TABLONES  3001   // ListView con las opiniones del archivo seleccionado
#define ID_EDIT_RESULT    3002   // Edit para el resultado de MiniZinc
#define ID_STATUSBAR      3003
#define ID_EDIT_PREVIEW   3004   // Vista previa del contenido crudo del archivo

#define ID_RADIO_GECODE   4001
#define ID_RADIO_HIGHS    4002
#define ID_RADIO_COINBC   4003

// ─── Estado global ─────────────────────────────────────────
string          g_archivoActual;      // Ruta completa del archivo seleccionado
string          g_nombreArchivo;      // Solo el nombre base
MinPolData      g_parsedData;         // Datos cargados y parseados

HWND g_hListArchivos = NULL;
HWND g_hListTablones = NULL;
HWND g_hEditPreview  = NULL;
HWND g_hEdit         = NULL;
HWND g_hStatus       = NULL;

HWND g_hLabelN       = NULL;
HWND g_hLabelCT      = NULL;
HWND g_hLabelMaxM    = NULL;

HFONT g_hFontMono    = NULL;
HFONT g_hFontUI      = NULL;
HFONT g_hFontUIBold  = NULL;
HINSTANCE g_hInst    = NULL;

// Rutas de datos y modelos
char g_dirDatos[MAX_PATH]     = {};   // Ruta raíz de datos (Proyecto/Datos)
char g_mznPath[MAX_PATH]      = {};   // Ruta de Proyecto.mzn
char g_tempDznPath[MAX_PATH]  = {};   // Ruta de temp_converted.dzn
char g_currentDir[MAX_PATH]   = {};   // Directorio actualmente explorado

// ─── Forward declarations ─────────────────────────────────
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void CrearControles(HWND hwnd);
void ComputarRutas();
void CargarListaArchivos(HWND hwnd);
void SeleccionarArchivo(HWND hwnd, const string& ruta, const string& nombre);
void LlenarListaOpiniones();
void EjecutarSolver(HWND hwnd);
void DialogoAbrirCarpeta(HWND hwnd);

// ─── Entry point ──────────────────────────────────────────
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow) {
    g_hInst = hInst;

    // Inicializar COM para el diálogo de selección de carpeta
    CoInitialize(NULL);

    INITCOMMONCONTROLSEX icc = {
        sizeof(INITCOMMONCONTROLSEX),
        ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES
    };
    InitCommonControlsEx(&icc);

    ComputarRutas();

    const char CLASS_NAME[] = "MinPolGUI";
    WNDCLASSEX wc = {};
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = CLASS_NAME;
    if (!RegisterClassEx(&wc)) return 0;

    g_hFontUI = CreateFont(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                           ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    g_hFontUIBold = CreateFont(15, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                               ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    g_hFontMono = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");

    HWND hwnd = CreateWindowEx(0, CLASS_NAME,
        "Optimización de Riego y Polarización ADA II",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1100, 740,
        NULL, NULL, hInst, NULL);
    if (!hwnd) return 0;

    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (g_hFontMono)   DeleteObject(g_hFontMono);
    if (g_hFontUI)     DeleteObject(g_hFontUI);
    if (g_hFontUIBold) DeleteObject(g_hFontUIBold);
    
    CoUninitialize();
    return 0;
}

// ─── Calcular rutas del proyecto ──────────────────────────
void ComputarRutas() {
    char exePath[MAX_PATH] = {};
    GetModuleFileNameA(NULL, exePath, MAX_PATH);

    string path(exePath);
    size_t last = path.find_last_of("\\/");
    if (last != string::npos) path = path.substr(0, last); // directory of gui.exe

    // Check if "Datos" folder exists in the executable's directory
    string datosPath = path + "\\Datos";
    DWORD attrib = GetFileAttributesA(datosPath.c_str());
    string resolvedBase;
    
    if (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY)) {
        // "Datos" is in the exe directory, so the exe directory is the base
        resolvedBase = path;
    } else {
        // Check if "Datos" is in the parent directory
        string parentPath = path + "\\..";
        char resolvedParent[MAX_PATH] = {};
        GetFullPathNameA(parentPath.c_str(), MAX_PATH, resolvedParent, NULL);
        resolvedBase = resolvedParent;
    }

    snprintf(g_dirDatos,    MAX_PATH, "%s\\Datos", resolvedBase.c_str());
    snprintf(g_mznPath,     MAX_PATH, "%s\\Proyecto.mzn", resolvedBase.c_str());
    snprintf(g_tempDznPath, MAX_PATH, "%s\\temp_converted.dzn", g_dirDatos);

    // Directorio de inicio del explorador
    strncpy(g_currentDir, g_dirDatos, MAX_PATH);
}

// ─── Window procedure ─────────────────────────────────────
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            CrearControles(hwnd);
            CargarListaArchivos(hwnd);
            break;

        case WM_SIZE: {
            RECT rc; GetClientRect(hwnd, &rc);
            int W = rc.right, H = rc.bottom;
            const int M = 10;          // margen
            const int PANEL_W = 240;   // ancho panel izquierdo
            const int GAP = 8;

            // Status bar
            SendMessage(g_hStatus, WM_SIZE, 0, 0);
            RECT rs; GetWindowRect(g_hStatus, &rs);
            int statusH = rs.bottom - rs.top;

            int innerH = H - statusH - M;

            // ── Panel izquierdo ──
            HWND hLblA = GetDlgItem(hwnd, 5001);
            SetWindowPos(hLblA, NULL, M, M, PANEL_W, 18, SWP_NOZORDER);

            // Botón Reload
            HWND hReload = GetDlgItem(hwnd, ID_BTN_RELOAD);
            SetWindowPos(hReload, NULL, M, M + 22, PANEL_W, 24, SWP_NOZORDER);

            // Lista de archivos
            int listArchH = (innerH - 22 - 24 - GAP * 2) / 2;
            if (listArchH < 80) listArchH = 80;
            SetWindowPos(g_hListArchivos, NULL, M, M + 22 + 24 + GAP,
                         PANEL_W, listArchH, SWP_NOZORDER);

            // Label vista previa
            HWND hLblP = GetDlgItem(hwnd, 5002);
            int previewY = M + 22 + 24 + GAP + listArchH + GAP;
            SetWindowPos(hLblP, NULL, M, previewY, PANEL_W, 18, SWP_NOZORDER);

            // Edit vista previa
            int previewH = innerH - previewY - 18 - GAP - M;
            if (previewH < 60) previewH = 60;
            SetWindowPos(g_hEditPreview, NULL, M, previewY + 20,
                         PANEL_W, previewH, SWP_NOZORDER);

            // ── Panel derecho ──
            int rightX = M + PANEL_W + GAP;
            int rightW = W - rightX - M;

            // Label "Opiniones del archivo:"
            HWND hLblT = GetDlgItem(hwnd, 5003);
            SetWindowPos(hLblT, NULL, rightX, M, 160, 18, SWP_NOZORDER);

            // Labels con variables globales (N, CT, MaxM)
            int lblX = rightX + 170;
            SetWindowPos(g_hLabelN, NULL, lblX, M, 75, 18, SWP_NOZORDER);
            SetWindowPos(g_hLabelCT, NULL, lblX + 80, M, 110, 18, SWP_NOZORDER);
            SetWindowPos(g_hLabelMaxM, NULL, lblX + 195, M, 110, 18, SWP_NOZORDER);

            // ListView opiniones
            int lvH = (innerH - 18 - GAP) * 2 / 5;
            if (lvH < 90) lvH = 90;
            SetWindowPos(g_hListTablones, NULL, rightX, M + 20,
                         rightW, lvH, SWP_NOZORDER);

            // Solver row
            int algoY = M + 20 + lvH + GAP;
            HWND hLblAlg = GetDlgItem(hwnd, 5004);
            SetWindowPos(hLblAlg, NULL, rightX, algoY + 4, 60, 20, SWP_NOZORDER);

            // Radio buttons
            const int radios[] = { ID_RADIO_GECODE, ID_RADIO_HIGHS, ID_RADIO_COINBC };
            const int radioW   = 85;
            int rx = rightX + 65;
            for (int i = 0; i < 3; i++) {
                HWND hR = GetDlgItem(hwnd, radios[i]);
                SetWindowPos(hR, NULL, rx, algoY + 2, radioW, 24, SWP_NOZORDER);
                rx += radioW + 4;
            }

            // Botón Resolver
            HWND hResol  = GetDlgItem(hwnd, ID_BTN_RESOLVER);
            int resolX   = rightX + rightW - 110;
            SetWindowPos(hResol,  NULL, resolX,  algoY, 105, 28, SWP_NOZORDER);

            // Edit resultados
            int editY = algoY + 28 + GAP;
            int editH = innerH - editY - M;
            if (editH < 80) editH = 80;
            SetWindowPos(g_hEdit, NULL, rightX, editY,
                         rightW, editH, SWP_NOZORDER);
            break;
        }

        case WM_CTLCOLORSTATIC: {
            HWND hCtrl = (HWND)lParam;
            int ctrlId = GetDlgCtrlID(hCtrl);
            HDC hdc = (HDC)wParam;
            if ((ctrlId >= 5000 && ctrlId <= 5005) || 
                hCtrl == g_hLabelN || hCtrl == g_hLabelCT || hCtrl == g_hLabelMaxM) {
                SetTextColor(hdc, RGB(30, 60, 120));
                SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
                return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
            }
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }

        case WM_COMMAND: {
            int id   = LOWORD(wParam);
            int code = HIWORD(wParam);

            // Doble clic en la lista (para abrir carpetas o subir)
            if (id == ID_LIST_ARCHIVOS && code == LBN_DBLCLK) {
                int sel = (int)SendMessageA(g_hListArchivos, LB_GETCURSEL, 0, 0);
                if (sel != LB_ERR) {
                    char buf[MAX_PATH] = {};
                    SendMessageA(g_hListArchivos, LB_GETTEXT, sel, (LPARAM)buf);
                    string entry(buf);

                    if (entry == ".. [Subir]") {
                        string dirStr(g_currentDir);
                        size_t pos = dirStr.find_last_of("\\/");
                        if (pos != string::npos) {
                            string parent = dirStr.substr(0, pos);
                            // Limitar a no subir más arriba que la carpeta g_dirDatos original si es posible
                            // pero permitir subir si se abrió otra carpeta arbitraria
                            strncpy(g_currentDir, parent.c_str(), MAX_PATH);
                            CargarListaArchivos(hwnd);
                        }
                    } else if (entry.rfind("<DIR> ", 0) == 0) {
                        string subFolder = entry.substr(6);
                        string nextDir = string(g_currentDir) + "\\" + subFolder;
                        strncpy(g_currentDir, nextDir.c_str(), MAX_PATH);
                        CargarListaArchivos(hwnd);
                    }
                }
                break;
            }

            // Selección simple en ListBox de archivos
            if (id == ID_LIST_ARCHIVOS && code == LBN_SELCHANGE) {
                int sel = (int)SendMessageA(g_hListArchivos, LB_GETCURSEL, 0, 0);
                if (sel != LB_ERR) {
                    char buf[MAX_PATH] = {};
                    SendMessageA(g_hListArchivos, LB_GETTEXT, sel, (LPARAM)buf);
                    string entry(buf);

                    if (entry != ".. [Subir]" && entry.rfind("<DIR> ", 0) != 0) {
                        string fullPath = string(g_currentDir) + "\\" + entry;
                        SeleccionarArchivo(hwnd, fullPath, entry);
                    }
                }
                break;
            }

            switch (id) {
                case ID_BTN_RELOAD:
                    CargarListaArchivos(hwnd);
                    break;
                case ID_FILE_OPEN_DIR:
                    DialogoAbrirCarpeta(hwnd);
                    break;
                case ID_FILE_EXIT:
                    DestroyWindow(hwnd);
                    break;
                case ID_HELP_ABOUT:
                    MessageBoxA(hwnd,
                        "Optimizador de Riego y Polarización — MinPol ADA II\n\n"
                        "Solvers disponibles:\n"
                        "  • Gecode (CP)   — Solver CP robusto\n"
                        "  • HiGHS (MIP)   — Solver MIP de alto rendimiento\n"
                        "  • COIN-BC (MIP) — Solver MIP de COIN-OR\n\n"
                        "Formatos de datos soportados:\n"
                        "  • .dzn (Formato nativo de MiniZinc)\n"
                        "  • .mpl (Formato de texto secuencial)",
                        "Acerca de", MB_OK | MB_ICONINFORMATION);
                    break;
                case ID_BTN_RESOLVER:
                    EjecutarSolver(hwnd);
                    break;
            }
            break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ─── Crear controles ──────────────────────────────────────
void CrearControles(HWND hwnd) {
    // Menú
    HMENU hMenu = CreateMenu();
    HMENU hFile = CreatePopupMenu();
    AppendMenu(hFile, MF_STRING, ID_FILE_OPEN_DIR, "&Abrir Carpeta...");
    AppendMenu(hFile, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFile, MF_STRING, ID_FILE_EXIT,  "&Salir");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFile, "&Archivo");
    HMENU hHelp = CreatePopupMenu();
    AppendMenu(hHelp, MF_STRING, ID_HELP_ABOUT, "&Acerca de");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hHelp, "&Ayuda");
    SetMenu(hwnd, hMenu);

    // Labels estáticos
    auto mkLabel = [&](int id, const char* txt, HFONT font) {
        HWND h = CreateWindow("STATIC", txt,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)id, g_hInst, NULL);
        if (font) SendMessage(h, WM_SETFONT, (WPARAM)font, TRUE);
        return h;
    };
    mkLabel(5001, "Archivos de entrada:",    g_hFontUIBold);
    mkLabel(5002, "Vista previa:",            g_hFontUIBold);
    mkLabel(5003, "Opiniones del archivo:",   g_hFontUIBold);
    mkLabel(5004, "Solver:",                  g_hFontUIBold);

    g_hLabelN = CreateWindow("STATIC", "N: -", WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 0, 10, 10, hwnd, NULL, g_hInst, NULL);
    SendMessage(g_hLabelN, WM_SETFONT, (WPARAM)g_hFontUIBold, TRUE);
    g_hLabelCT = CreateWindow("STATIC", "CT: -", WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 0, 10, 10, hwnd, NULL, g_hInst, NULL);
    SendMessage(g_hLabelCT, WM_SETFONT, (WPARAM)g_hFontUIBold, TRUE);
    g_hLabelMaxM = CreateWindow("STATIC", "MaxM: -", WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 0, 10, 10, hwnd, NULL, g_hInst, NULL);
    SendMessage(g_hLabelMaxM, WM_SETFONT, (WPARAM)g_hFontUIBold, TRUE);

    // Botones del panel izquierdo
    {
        HWND h = CreateWindow("BUTTON", "Recargar",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, 0, 10, 10, hwnd, (HMENU)ID_BTN_RELOAD, g_hInst, NULL);
        SendMessage(h, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
    }

    // ListBox
    g_hListArchivos = CreateWindow("LISTBOX", "",
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
        0, 0, 10, 10, hwnd, (HMENU)ID_LIST_ARCHIVOS, g_hInst, NULL);
    SendMessage(g_hListArchivos, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

    // Edit vista previa
    g_hEditPreview = CreateWindow("EDIT", "",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_READONLY |
        ES_AUTOVSCROLL | WS_VSCROLL | ES_NOHIDESEL,
        0, 0, 10, 10, hwnd, (HMENU)ID_EDIT_PREVIEW, g_hInst, NULL);
    SendMessage(g_hEditPreview, WM_SETFONT, (WPARAM)g_hFontMono, TRUE);

    // ListView opiniones
    g_hListTablones = CreateWindow(WC_LISTVIEWA, "",
        WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SINGLESEL,
        0, 0, 10, 10, hwnd, (HMENU)ID_LIST_TABLONES, g_hInst, NULL);
    ListView_SetExtendedListViewStyle(g_hListTablones,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

    LV_COLUMNA lvc = {};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT | LVCF_SUBITEM;
    lvc.fmt  = LVCFMT_RIGHT;
    lvc.pszText = (char*)"#";              lvc.cx = 45;  ListView_InsertColumn(g_hListTablones, 0, &lvc);
    lvc.pszText = (char*)"p (Poblacion)";  lvc.cx = 120; ListView_InsertColumn(g_hListTablones, 1, &lvc);
    lvc.pszText = (char*)"v (Valor)";      lvc.cx = 120; ListView_InsertColumn(g_hListTablones, 2, &lvc);
    lvc.pszText = (char*)"ce (Costo Extra)";lvc.cx = 140; ListView_InsertColumn(g_hListTablones, 3, &lvc);

    // Radio buttons — HiGHS primero (Gecode no soporta float en este modelo)
    struct { int id; const char* lbl; DWORD extra; } radios[] = {
        { ID_RADIO_HIGHS,  "Highs",   WS_GROUP | BS_AUTORADIOBUTTON },
        { ID_RADIO_COINBC, "coin-bc", BS_AUTORADIOBUTTON },
        { ID_RADIO_GECODE, "gecode",  BS_AUTORADIOBUTTON },
    };
    for (auto& r : radios) {
        HWND h = CreateWindow("BUTTON", r.lbl,
            WS_CHILD | WS_VISIBLE | r.extra,
            0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)r.id, g_hInst, NULL);
        SendMessage(h, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
    }
    // Marcar HiGHS por defecto (Gecode no es compatible con float del modelo)
    SendMessage(GetDlgItem(hwnd, ID_RADIO_HIGHS), BM_SETCHECK, BST_CHECKED, 0);

    // Botón Resolver
    {
        HWND h = CreateWindow("BUTTON", "Resolver",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_DEFPUSHBUTTON,
            0, 0, 10, 10, hwnd, (HMENU)ID_BTN_RESOLVER, g_hInst, NULL);
        SendMessage(h, WM_SETFONT, (WPARAM)g_hFontUIBold, TRUE);
    }

    // Edit resultados
    g_hEdit = CreateWindow("EDIT",
        "Bienvenido.\r\nSeleccione un archivo de la lista, elija un solver y presione Resolver.",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_READONLY |
        ES_AUTOVSCROLL | WS_VSCROLL,
        0, 0, 10, 10, hwnd, (HMENU)ID_EDIT_RESULT, g_hInst, NULL);
    SendMessage(g_hEdit, WM_SETFONT, (WPARAM)g_hFontMono, TRUE);

    // Status bar
    g_hStatus = CreateWindow(STATUSCLASSNAME,
        "Listo. Seleccione un archivo de la lista izquierda o abra otra carpeta.",
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0, hwnd, (HMENU)ID_STATUSBAR, g_hInst, NULL);
}

// Comparator for case-insensitive alphabetical sorting
bool compareNoCase(const string& a, const string& b) {
    return _stricmp(a.c_str(), b.c_str()) < 0;
}

// ─── Cargar lista de archivos y directorios de g_currentDir ──
void CargarListaArchivos(HWND hwnd) {
    SendMessageA(g_hListArchivos, LB_RESETCONTENT, 0, 0);

    vector<string> subDirs;
    vector<string> files;

    // 1. Buscar subdirectorios
    string searchPatternDirs = string(g_currentDir) + "\\*";
    WIN32_FIND_DATAA ffd;
    HANDLE hFindDirs = FindFirstFileA(searchPatternDirs.c_str(), &ffd);
    if (hFindDirs != INVALID_HANDLE_VALUE) {
        do {
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (strcmp(ffd.cFileName, ".") != 0 && strcmp(ffd.cFileName, "..") != 0) {
                    subDirs.push_back(ffd.cFileName);
                }
            }
        } while (FindNextFileA(hFindDirs, &ffd));
        FindClose(hFindDirs);
    }

    // 2. Buscar archivos .dzn y .mpl
    auto collectFiles = [&](const string& pattern) {
        string searchPattern = string(g_currentDir) + "\\" + pattern;
        HANDLE hFind = FindFirstFileA(searchPattern.c_str(), &ffd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    files.push_back(ffd.cFileName);
                }
            } while (FindNextFileA(hFind, &ffd));
            FindClose(hFind);
        }
    };
    collectFiles("*.dzn");
    collectFiles("*.mpl");

    // Ordenar carpetas y archivos alfabéticamente (no sensible a mayúsculas)
    sort(subDirs.begin(), subDirs.end(), compareNoCase);
    sort(files.begin(), files.end(), compareNoCase);

    // Si no estamos en el directorio de Datos raíz, añadir opción de subir
    if (_stricmp(g_currentDir, g_dirDatos) != 0) {
        SendMessageA(g_hListArchivos, LB_ADDSTRING, 0, (LPARAM)".. [Subir]");
    }

    // Añadir directorios
    for (const auto& d : subDirs) {
        string dirEntry = "<DIR> " + d;
        SendMessageA(g_hListArchivos, LB_ADDSTRING, 0, (LPARAM)dirEntry.c_str());
    }

    // Añadir archivos
    for (const auto& f : files) {
        SendMessageA(g_hListArchivos, LB_ADDSTRING, 0, (LPARAM)f.c_str());
    }

    char status[512];
    snprintf(status, sizeof(status), "Carpeta actual: %s", g_currentDir);
    SetWindowTextA(g_hStatus, status);
}

// ─── Diálogo para abrir carpetas personalizadas ────────────
void DialogoAbrirCarpeta(HWND hwnd) {
    BROWSEINFOA bi = { 0 };
    bi.hwndOwner = hwnd;
    bi.lpszTitle = "Seleccione la carpeta que contiene los archivos de datos (.dzn o .mpl):";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (pidl != NULL) {
        char path[MAX_PATH];
        if (SHGetPathFromIDListA(pidl, path)) {
            // Actualizar el directorio de navegación y la base local
            strncpy(g_currentDir, path, MAX_PATH);
            // También redefinimos g_dirDatos para que sea este nuevo directorio base y se pueda subir a él
            strncpy(g_dirDatos, path, MAX_PATH);
            CargarListaArchivos(hwnd);
        }
        CoTaskMemFree(pidl);
    }
}

// ─── Seleccionar y cargar un archivo de la lista ──────────
void SeleccionarArchivo(HWND hwnd, const string& ruta, const string& nombre) {
    // Leer el contenido
    ifstream archivo(ruta, ios::in | ios::binary);
    if (!archivo.is_open()) {
        MessageBoxA(hwnd, "No se pudo abrir el archivo.", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    string content((istreambuf_iterator<char>(archivo)), istreambuf_iterator<char>());
    archivo.close();

    // Actualizar vista previa cruda reemplazando saltos de línea LF a CRLF si es necesario
    string previewStr;
    previewStr.reserve(content.size() * 11 / 10);
    for (char c : content) {
        if (c == '\n') {
            if (previewStr.empty() || previewStr.back() != '\r') {
                previewStr += "\r\n";
            } else {
                previewStr += '\n';
            }
        } else {
            previewStr += c;
        }
    }
    SetWindowTextA(g_hEditPreview, previewStr.c_str());

    // Parsear datos según extensión
    g_parsedData = MinPolData();
    bool parsedOk = false;

    if (ruta.size() > 4 && ruta.substr(ruta.size() - 4) == ".mpl") {
        parsedOk = MinPolParser::parseMPL(content, g_parsedData);
    } else if (ruta.size() > 4 && ruta.substr(ruta.size() - 4) == ".dzn") {
        parsedOk = MinPolParser::parseDZN(content, g_parsedData);
    }

    if (!parsedOk) {
        SetWindowTextA(g_hLabelN, "N: -");
        SetWindowTextA(g_hLabelCT, "CT: -");
        SetWindowTextA(g_hLabelMaxM, "MaxM: -");
        ListView_DeleteAllItems(g_hListTablones);

        char status[512];
        snprintf(status, sizeof(status), "Error al parsear archivo: %s", nombre.c_str());
        SetWindowTextA(g_hStatus, status);
        MessageBoxA(hwnd, "El archivo seleccionado no tiene un formato valido de MinPol (.dzn o .mpl).", 
                    "Error de Parseo", MB_OK | MB_ICONWARNING);
        return;
    }

    g_archivoActual = ruta;
    g_nombreArchivo = nombre;

    // Actualizar etiquetas globales
    char buf[128];
    snprintf(buf, sizeof(buf), "N: %d", g_parsedData.n);
    SetWindowTextA(g_hLabelN, buf);

    snprintf(buf, sizeof(buf), "CT: %.2f", g_parsedData.ct);
    SetWindowTextA(g_hLabelCT, buf);

    snprintf(buf, sizeof(buf), "MaxM: %d", g_parsedData.maxM);
    SetWindowTextA(g_hLabelMaxM, buf);

    LlenarListaOpiniones();

    snprintf(buf, sizeof(buf), "Cargado: %s | %d opiniones", nombre.c_str(), g_parsedData.m);
    SetWindowTextA(g_hStatus, buf);

    SetWindowTextA(g_hEdit, "Archivo cargado con éxito. Seleccione un solver y presione Resolver.");
}

// ─── Llenar ListView con los datos de las opiniones ───────
void LlenarListaOpiniones() {
    ListView_DeleteAllItems(g_hListTablones);
    if (!g_parsedData.valid) return;

    for (int i = 0; i < g_parsedData.m; i++) {
        char buf[32];
        LV_ITEMA item = {};
        item.mask    = LVIF_TEXT;
        item.iItem   = i;
        snprintf(buf, sizeof(buf), "%d", i + 1); // Opinión #i es la opinion con índice 1 a m
        item.pszText = buf;
        ListView_InsertItem(g_hListTablones, &item);

        // Población p
        if (i < (int)g_parsedData.p.size()) {
            snprintf(buf, sizeof(buf), "%d", g_parsedData.p[i]);
            ListView_SetItemText(g_hListTablones, i, 1, buf);
        }

        // Valor v
        if (i < (int)g_parsedData.v.size()) {
            snprintf(buf, sizeof(buf), "%.3f", g_parsedData.v[i]);
            ListView_SetItemText(g_hListTablones, i, 2, buf);
        }

        // Costo Extra ce
        if (i < (int)g_parsedData.ce.size()) {
            snprintf(buf, sizeof(buf), "%.3f", g_parsedData.ce[i]);
            ListView_SetItemText(g_hListTablones, i, 3, buf);
        }
    }
}

// ─── Ejecutar el solver MiniZinc con los parámetros ────────
void EjecutarSolver(HWND hwnd) {
    if (g_archivoActual.empty() || !g_parsedData.valid) {
        MessageBoxA(hwnd, "Por favor, seleccione un archivo de entrada valido primero.",
                    "Sin datos", MB_OK | MB_ICONWARNING);
        return;
    }

    SetWindowTextA(g_hStatus, "Ejecutando solver MiniZinc...");
    UpdateWindow(hwnd);

    SetWindowTextA(g_hEdit, "Iniciando MiniZinc, por favor espere...\r\n");
    UpdateWindow(g_hEdit);

    string dataPath = g_archivoActual;

    // Si es un archivo .mpl, escribimos el DZN convertido a un archivo temporal
    if (g_archivoActual.size() > 4 && 
        g_archivoActual.substr(g_archivoActual.size() - 4) == ".mpl") {
        
        string dznContent = MinPolParser::toDZN(g_parsedData);
        ofstream out(g_tempDznPath);
        if (!out.is_open()) {
            MessageBoxA(hwnd, "No se pudo crear el archivo DZN temporal para MiniZinc.", "Error", MB_OK | MB_ICONERROR);
            SetWindowTextA(g_hStatus, "Error DZN temporal");
            return;
        }
        out << dznContent;
        out.close();

        dataPath = g_tempDznPath;
    }

    // Determinar solver seleccionado (HiGHS es el default — Gecode no soporta float)
    string solverName = "highs";
    if (SendMessage(GetDlgItem(hwnd, ID_RADIO_COINBC), BM_GETCHECK, 0, 0) == BST_CHECKED) {
        solverName = "coin-bc";
    } else if (SendMessage(GetDlgItem(hwnd, ID_RADIO_GECODE), BM_GETCHECK, 0, 0) == BST_CHECKED) {
        solverName = "gecode";
    }

    // Construir comando con --statistics para obtener el tiempo real del solver
    string cmd = "minizinc --solver " + solverName
               + " --statistics"
               + " \"" + g_mznPath + "\""
               + " \"" + dataPath + "\""
               + " 2>&1";

    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) {
        SetWindowTextA(g_hEdit, "Error: No se pudo iniciar el proceso de MiniZinc.\r\n"
                                "Asegurese de que MiniZinc esta en el PATH del sistema.");
        SetWindowTextA(g_hStatus, "Error al iniciar MiniZinc");
        if (dataPath == g_tempDznPath) DeleteFileA(g_tempDznPath);
        return;
    }

    char buffer[512];
    string output    = "Ejecutando: minizinc --solver " + solverName
                     + " \"" + g_mznPath + "\""
                     + " \"" + dataPath + "\"\r\n";
    output += "--------------------------------------------------------\r\n\r\n";

    // Tiempo de solución extraído de %%%mzn-stat: solveTime=X
    double solveTimeSec = -1.0;

    // Prefijo de líneas de estadísticas de MiniZinc
    const string STAT_PREFIX = "%%%mzn-stat:";

    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        string line(buffer);

        // Quitar \r\n al final para procesar
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
            line.pop_back();

        // Detectar y extraer solveTime de las estadísticas
        if (line.rfind(STAT_PREFIX, 0) == 0) {
            // Formato: %%%mzn-stat: solveTime=0.123456
            size_t eqPos = line.find("solveTime=");
            if (eqPos != string::npos) {
                try {
                    solveTimeSec = stod(line.substr(eqPos + 10));
                } catch (...) {}
            }
            // No mostrar líneas de estadísticas en el panel de resultados
            continue;
        }

        // Mostrar líneas normales (resultado, errores, separadores)
        output += line + "\r\n";
        SetWindowTextA(g_hEdit, output.c_str());

        int len = GetWindowTextLengthA(g_hEdit);
        SendMessage(g_hEdit, EM_SETSEL, len, len);
        SendMessage(g_hEdit, EM_SCROLLCARET, 0, 0);
        UpdateWindow(g_hEdit);
    }

    _pclose(pipe);

    // Eliminar archivo temporal si existia
    if (dataPath == g_tempDznPath) DeleteFileA(g_tempDznPath);

    // ─── Resumen de tiempo ────────────────────────────────────
    string timerLine = "\r\n========================================================\r\n";

    char timeBuf[256];
    if (solveTimeSec >= 0.0) {
        // Tiempo exacto reportado por el solver
        double solveMs = solveTimeSec * 1000.0;
        snprintf(timeBuf, sizeof(timeBuf),
            "  Solver          : %s\r\n"
            "  Archivo         : %s\r\n"
            "  Tiempo de solucion: %.3f ms  (%.6f s)\r\n",
            solverName.c_str(),
            g_nombreArchivo.c_str(),
            solveMs,
            solveTimeSec);
    } else {
        // Fallback: solveTime no estuvo disponible en la salida
        snprintf(timeBuf, sizeof(timeBuf),
            "  Solver          : %s\r\n"
            "  Archivo         : %s\r\n"
            "  Tiempo de solucion: (no disponible)\r\n",
            solverName.c_str(),
            g_nombreArchivo.c_str());
    }
    timerLine += timeBuf;
    timerLine += "========================================================\r\n";

    output += timerLine;
    SetWindowTextA(g_hEdit, output.c_str());

    // Auto-scroll al resumen
    {
        int len = GetWindowTextLengthA(g_hEdit);
        SendMessage(g_hEdit, EM_SETSEL, len, len);
        SendMessage(g_hEdit, EM_SCROLLCARET, 0, 0);
    }

    // Status bar con el tiempo de solución
    char statusBuf[256];
    if (solveTimeSec >= 0.0) {
        snprintf(statusBuf, sizeof(statusBuf),
            "Listo. Solver: %s | Tiempo de solucion: %.3f ms",
            solverName.c_str(), solveTimeSec * 1000.0);
    } else {
        snprintf(statusBuf, sizeof(statusBuf),
            "Listo. Solver: %s", solverName.c_str());
    }
    SetWindowTextA(g_hStatus, statusBuf);
}
