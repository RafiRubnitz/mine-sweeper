// winmine_compilable.c
// Faithful reconstruction of WINMINE_1.EXE (32-bit Windows Minesweeper).
// Decompiled from original binary, made compilable with MSVC 2022.
//
// Compile:
//   cl.exe /nologo /W3 /O2 /DUNICODE /D_UNICODE winmine_compilable.c ^
//       user32.lib gdi32.lib advapi32.lib shell32.lib winmm.lib comctl32.lib ^
//       /link /SUBSYSTEM:WINDOWS
//
// Requires winmine.res built from winmine.rc with the original resources
// (bitmaps, icons, dialogs, string table, menu, accelerators, sounds).

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>     // ShellAboutW
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <commctrl.h>
#include <mmsystem.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "comctl32.lib")

// ============================================================================
// Resource IDs (must match the .rc / .res file)
// ============================================================================
#define IDI_APP                 100
#define IDB_TILES               410
#define IDB_DIGITS              420
#define IDB_FACES               430
#define IDS_TICK                432
#define IDS_REVEAL              433
#define IDS_WIN                 434
#define IDD_CUSTOM              80
#define IDM_MAIN                500
#define IDA_ACCEL               501

// Menu command IDs
#define IDM_NEW                 510
#define IDM_BEGINNER            521
#define IDM_INTERMEDIATE        522
#define IDM_EXPERT              523
#define IDM_CUSTOM              524
#define IDM_MARKS               525
#define IDM_SOUND               526
#define IDM_COLOR               527
#define IDM_BESTTIMES           528
#define IDM_EXIT                529
#define IDM_HELP_CONTENTS       590
#define IDM_HELP_SEARCH         591
#define IDM_HELP_HOWTO          592
#define IDM_ABOUT               593

// String resource IDs
#define IDS_APPNAME             2
#define IDS_ERRORCAPTION        4
#define IDS_ERRORTIMER          5
#define IDS_ERRORMEM            6
#define IDS_ERRORFMT            7
#define IDS_TIMEFMT             8
#define IDS_ANONYMOUS           9
#define IDS_BESTBEGINNER        10
#define IDS_BESTINTERMEDIATE    11
#define IDS_BESTEXPERT          12

// Custom dialog control IDs
#define IDC_HEIGHT              141
#define IDC_WIDTH               142
#define IDC_MINES               143

// ============================================================================
// Game Constants
// ============================================================================
#define MINE_FLAG       0x80   // bit 7: cell contains a mine
#define REVEALED_FLAG   0x40   // bit 6: cell is revealed
#define CELL_STATE_MASK 0x1F   // bits 0-4: display tile index

// Cell display state values (tile index in the DIB strip).
// 0-8:             revealed number (adjacent mine count)
// 9:               empty flag (flagged + no mine, shown at game over)
// 11:              wrong flag (shown at game over for mis-flagged cells)
// 12:              mine revealed (blown-up mine)
// 13:              flagged (cell has flag)
// 14:              question mark
// 15:              unrevealed (normal untouched cell)
#define CELL_EMPTY_FLAG  9
#define CELL_WRONG_FLAG 11
#define CELL_MINE_HIT   12
#define CELL_FLAGGED    13
#define CELL_QUESTION   14
#define CELL_UNREVEALED 15
#define CELL_BORDER     16   // not a tile index; marks border cells

// Face button states (tile index in the face DIB strip)
#define FACE_NORMAL     0
#define FACE_SURPRISED  1
#define FACE_CLICKED    2
#define FACE_DEAD       3
#define FACE_COOL       4

// Sound resource IDs used with PlaySound
#define SOUND_TICK      0x1B0
#define SOUND_REVEAL    0x1B1
#define SOUND_WIN       0x1B2

// Sound enable value (original uses 3 as "on")
#define SOUND_ON        3

// Difficulty levels
#define DIFF_BEGINNER       0
#define DIFF_INTERMEDIATE   1
#define DIFF_EXPERT         2
#define DIFF_CUSTOM         3

// Board limits
#define MAX_ROWS        32
#define MAX_COLS        32
#define CELL_SIZE       16    // pixels per cell

// Timer ID
#define IDT_GAME_TIMER  1

// Game state flags (g_dwGameFlags)
#define GAME_ACTIVE     0x01   // game in progress
#define GAME_OVER       0x10   // game ended
#define GAME_POS_LOCK   0x08   // window position locked (minimized)

// ============================================================================
// Global Variables
// ============================================================================

// Game state
DWORD g_dwGameFlags = 0;

// Difficulty table: {height, width, mines}
int g_difficultyTable[4][3] = {
    {9,  9,  10},    // beginner
    {16, 16, 40},    // intermediate
    {30, 16, 99},    // expert
    {9,  9,  10}     // custom (overwritten at runtime)
};

// Input state
int   g_nCurCellX = 0;
int   g_nCurCellY = 0;
BOOL  g_bMouseDown = FALSE;
BOOL  g_bBothButtons = FALSE;
BOOL  g_bCancelFlag = FALSE;
BOOL  g_bMenuLoop = FALSE;

// Cheat state (xyzzy)
int   g_nCheatState = 0;
static const BYTE g_CheatKeys[5] = { 'X', 'Y', 'Z', 'Z', 'Y' };

int   g_nFaceState = FACE_NORMAL;

BOOL  g_bTimerRunning = FALSE;
BOOL  g_bRedrawSave = FALSE;

// Board dimensions
int   g_nBoardWidth = 9;
int   g_nBoardHeight = 9;
int   g_nMineCount = 10;
int   g_nMineDisplay = 10;

// Board arrays (each 32x32 bytes, row-major: [y][x])
BYTE g_board[MAX_ROWS][MAX_COLS];        // logical state (mines, revealed, flags, numbers)
BYTE g_boardDisplay[MAX_ROWS][MAX_COLS]; // display state (what is drawn on screen)

int g_nSafeCells = 0;
int g_nRevealedCells = 0;
int g_nTimer = 0;

// Flood fill queue (circular buffer, 100 entries)
int g_nFloodQHead = 1;
int g_nFloodQTail = 1;
int g_floodX[100];
int g_floodY[100];

// Settings (persisted to registry)
int   g_nDifficulty = DIFF_BEGINNER;
int   g_nDiffMines  = 10;
int   g_nDiffHeight = 9;
int   g_nDiffWidth  = 9;
int   g_nWindowX    = 80;
int   g_nWindowY    = 80;
int   g_nSound      = 0;       // 0=off, 3=on
BOOL  g_bMarks      = TRUE;
int   g_nDblClk     = 0;
int   g_nMenuState  = 0;
BOOL  g_bColor      = TRUE;
int   g_nBestTimes[3] = { 999, 999, 999 };
WCHAR g_szBestNames[3][32];

// Window / UI
HWND       g_hWnd = NULL;
HINSTANCE  g_hInst = NULL;
HMENU      g_hMenu = NULL;
HICON      g_hIcon = NULL;

int   g_nClientWidth  = 0;
int   g_nClientHeight = 0;
int   g_nBorderTop    = 0;
int   g_nBorderLeft   = 0;
int   g_nCxEdge       = 0;
int   g_nCyEdge       = 0;
int   g_nMenuHeight   = 0;
BOOL  g_bInit         = FALSE;

// DIB Bitmap resources
BITMAPINFO *g_pbmTiles  = NULL;
BITMAPINFO *g_pbmDigits = NULL;
BITMAPINFO *g_pbmFaces  = NULL;

// Per-tile DCs (pre-blited tiles for fast BitBlt)
HDC     g_hdcTiles[16];
HBITMAP g_hbmTiles[16];

// Byte offsets within each DIB strip for each frame
int g_nTileOffsets[16];    // 16 tiles  (16x16 each)
int g_nDigitOffsets[12];   // 12 digits (13x23 each: 0-9, minus, blank)
int g_nFaceOffsets[6];     // 6 faces   (24x24 each)

// Pens for 3D border drawing
HPEN g_hPenWhite = NULL;
HPEN g_hPenLight = NULL;
HPEN g_hPenDark  = NULL;

// Application strings
WCHAR g_szAppName[64];
WCHAR g_szAnonName[32];

// ============================================================================
// Forward Declarations
// ============================================================================
static void        LoadSettings(void);
static void        SaveSettings(void);
static BOOL        LoadBitmaps(void);
static void        ClearBoard(void);
static void        NewGame(void);
static int         CountAdjacentMines(int x, int y);
static int         CountAdjacentFlags(int x, int y);
static void        RevealCell(int x, int y);
static void        RevealCells(int x, int y);
static void        RightClickCell(int x, int y);
static void        ClickCell(int nNewX, int nNewY);
static void        ChordClick(int x, int y);
static void        ClickNumberedCell(int x, int y);
static void        SetCellState(int x, int y, BYTE state);
static int         RandomInt(int nMax);
static void        DrawCell(int x, int y);
static void        DrawAllCells(HDC hdc);
static void        DrawBorder(HDC hdc);
static void        DrawMineCounter(HDC hdc);
static void        DrawTimer(HDC hdc);
static void        DrawDigit(HDC hdc, int x, int nDigit);
static void        Draw3DRect(HDC hdc, int x, int y, int cx, int cy, int nThick, BOOL bRaised);
static void        PaintBoard(HDC hdc);
static void        SetFace(int nFace);
static void        UpdateCounter(int nDelta);
static void        EndGame(BOOL bWon);
static void        SizeWindow(BYTE flags);
static void        StartGame(void);
static void        TimerTick(void);
static void        PlayGameSound(int nSoundId);
static BOOL        FaceButtonClick(LPARAM lParam);
static void        ChangeDifficulty(int nDiff);
static void        ShowBestTimes(void);
static void        UpdateMenu(void);
static void        ShowError(int nErrCode);
static INT_PTR CALLBACK CustomDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK BestTimesDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ============================================================================
// Registry Helpers
// ============================================================================
// Original game stores settings under HKCU\Software\Microsoft\winmine.
// Value names are the string representation of the setting ID.

#define REG_APP_KEY L"Software\\Microsoft\\winmine"

static LSTATUS RegReadDword(int nId, DWORD *pValue)
{
    WCHAR szName[16];
    wsprintfW(szName, L"%d", nId);

    HKEY hKey;
    LSTATUS stat = RegOpenKeyExW(HKEY_CURRENT_USER, REG_APP_KEY, 0, KEY_READ, &hKey);
    if (stat != ERROR_SUCCESS)
        return stat;

    DWORD dwType, cbData = sizeof(DWORD);
    stat = RegQueryValueExW(hKey, szName, NULL, &dwType, (LPBYTE)pValue, &cbData);
    if (stat != ERROR_SUCCESS || dwType != REG_DWORD)
        stat = ERROR_FILE_NOT_FOUND;

    RegCloseKey(hKey);
    return stat;
}

static LSTATUS RegWriteDword(int nId, DWORD dwValue)
{
    WCHAR szName[16];
    wsprintfW(szName, L"%d", nId);

    HKEY hKey;
    DWORD dwDisp;
    LSTATUS stat = RegCreateKeyExW(HKEY_CURRENT_USER, REG_APP_KEY,
        0, NULL, 0, KEY_WRITE, NULL, &hKey, &dwDisp);
    if (stat != ERROR_SUCCESS)
        return stat;

    stat = RegSetValueExW(hKey, szName, 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(dwValue));
    RegCloseKey(hKey);
    return stat;
}

static LSTATUS RegReadStr(int nId, LPWSTR pszOut, int cchOut)
{
    WCHAR szName[16];
    wsprintfW(szName, L"%d", nId);

    HKEY hKey;
    LSTATUS stat = RegOpenKeyExW(HKEY_CURRENT_USER, REG_APP_KEY, 0, KEY_READ, &hKey);
    if (stat != ERROR_SUCCESS) {
        pszOut[0] = L'\0';
        return stat;
    }

    DWORD dwType, cbData = cchOut * sizeof(WCHAR);
    stat = RegQueryValueExW(hKey, szName, NULL, &dwType, (LPBYTE)pszOut, &cbData);
    if (stat != ERROR_SUCCESS || dwType != REG_SZ)
        pszOut[0] = L'\0';

    RegCloseKey(hKey);
    return stat;
}

static LSTATUS RegWriteStr(int nId, LPCWSTR pszValue)
{
    WCHAR szName[16];
    wsprintfW(szName, L"%d", nId);

    HKEY hKey;
    DWORD dwDisp;
    LSTATUS stat = RegCreateKeyExW(HKEY_CURRENT_USER, REG_APP_KEY,
        0, NULL, 0, KEY_WRITE, NULL, &hKey, &dwDisp);
    if (stat != ERROR_SUCCESS)
        return stat;

    size_t cbData = (wcslen(pszValue) + 1) * sizeof(WCHAR);
    stat = RegSetValueExW(hKey, szName, 0, REG_SZ, (LPBYTE)pszValue, (DWORD)cbData);
    RegCloseKey(hKey);
    return stat;
}

// ============================================================================
// Settings Load / Save
// ============================================================================

static int ReadSettingInt(int nId, int nDefault, int nMin, int nMax)
{
    DWORD dwVal;
    if (RegReadDword(nId, &dwVal) != ERROR_SUCCESS)
        return nDefault;
    int val = (int)dwVal;
    if (val < nMin) val = nMin;
    if (val > nMax) val = nMax;
    return val;
}

static void LoadSettings(void)
{
    srand(GetTickCount());

    // Load strings from resources
    LoadStringW(g_hInst, IDS_APPNAME,   g_szAppName, 64);
    LoadStringW(g_hInst, IDS_ANONYMOUS, g_szAnonName, 32);

    // Read all settings from registry
    g_nDiffHeight   = ReadSettingInt(2,  9,   9,   25);
    g_nDiffWidth    = ReadSettingInt(3,  9,   9,   30);
    g_nDifficulty   = ReadSettingInt(0,  0,   0,   3);
    g_nDiffMines    = ReadSettingInt(1,  10,  10,  999);
    g_nWindowX      = ReadSettingInt(4,  80,  0,   2048);
    g_nWindowY      = ReadSettingInt(5,  80,  0,   2048);
    g_nSound        = ReadSettingInt(6,  0,   0,   3);
    g_bMarks        = ReadSettingInt(7,  1,   0,   1) ? TRUE : FALSE;
    g_nMenuState    = ReadSettingInt(8,  0,   0,   3);
    g_nDblClk       = ReadSettingInt(9,  0,   0,   1);
    g_bColor        = ReadSettingInt(10, 1,   0,   1) ? TRUE : FALSE;
    g_nBestTimes[0] = ReadSettingInt(11, 999, 0,   999);
    g_nBestTimes[1] = ReadSettingInt(13, 999, 0,   999);
    g_nBestTimes[2] = ReadSettingInt(15, 999, 0,   999);

    // Read best player names
    RegReadStr(12, g_szBestNames[0], 32);
    RegReadStr(14, g_szBestNames[1], 32);
    RegReadStr(16, g_szBestNames[2], 32);

    // Set defaults for empty best names
    if (g_szBestNames[0][0] == L'\0')
        wcscpy_s(g_szBestNames[0], 32, g_szAnonName);
    if (g_szBestNames[1][0] == L'\0')
        wcscpy_s(g_szBestNames[1], 32, g_szAnonName);
    if (g_szBestNames[2][0] == L'\0')
        wcscpy_s(g_szBestNames[2], 32, g_szAnonName);

    // Load difficulty table defaults
    if (g_nDifficulty >= DIFF_BEGINNER && g_nDifficulty <= DIFF_EXPERT) {
        g_nDiffHeight = g_difficultyTable[g_nDifficulty][0];
        g_nDiffWidth  = g_difficultyTable[g_nDifficulty][1];
        g_nDiffMines  = g_difficultyTable[g_nDifficulty][2];
    }

    if (g_nSound != SOUND_ON)
        g_nSound = 0;
}

static void SaveSettings(void)
{
    RegWriteDword(0,  (DWORD)g_nDifficulty);
    RegWriteDword(1,  (DWORD)g_nDiffMines);
    RegWriteDword(2,  (DWORD)g_nDiffHeight);
    RegWriteDword(3,  (DWORD)g_nDiffWidth);
    RegWriteDword(4,  (DWORD)g_nWindowX);
    RegWriteDword(5,  (DWORD)g_nWindowY);
    RegWriteDword(6,  (DWORD)g_nSound);
    RegWriteDword(7,  g_bMarks ? 1 : 0);
    RegWriteDword(8,  (DWORD)g_nMenuState);
    RegWriteDword(9,  (DWORD)g_nDblClk);
    RegWriteDword(10, g_bColor ? 1 : 0);
    RegWriteDword(11, (DWORD)g_nBestTimes[0]);
    RegWriteDword(13, (DWORD)g_nBestTimes[1]);
    RegWriteDword(15, (DWORD)g_nBestTimes[2]);

    // Write best names (preceded by marker values)
    RegWriteDword(12, 0);
    RegWriteStr(12, g_szBestNames[0]);
    RegWriteDword(14, 0);
    RegWriteStr(14, g_szBestNames[1]);
    RegWriteDword(16, 0);
    RegWriteStr(16, g_szBestNames[2]);
}

// ============================================================================
// DIB Bitmap Loading
// ============================================================================
// The game uses three horizontal strip bitmaps in 4bpp indexed DIB format:
//   Tiles (IDB_TILES 410):   16 frames x 16x16 pixels
//   Digits (IDB_DIGITS 420): 12 frames x 13x23 pixels (0-9, minus, blank)
//   Faces (IDB_FACES 430):   6  frames x 24x24 pixels
//
// stride = ((width * 4 + 31) / 32) * 4    (DWORD-aligned for 4bpp)
// per-frame bytes = stride * height

// Compute DWORD-aligned row stride from actual BITMAPINFO header bpp.
static int DibRowStride(BITMAPINFO *pDib, int nWidth)
{
    if (!pDib) return ((nWidth * 4 + 31) / 32) * 4;
    int bpp = pDib->bmiHeader.biBitCount;
    if (bpp <= 0) bpp = 4;
    return ((nWidth * bpp + 31) / 32) * 4;
}

// Compute per-frame byte count in a DIB strip (row-stride * frame-height).
static int DibFrameBytes(BITMAPINFO *pDib, int nWidth, int nHeight)
{
    return DibRowStride(pDib, nWidth) * nHeight;
}

// Return pointer to the first pixel byte (after BITMAPINFOHEADER + color table).
static BYTE* DibPixelBase(BITMAPINFO *pDib)
{
    if (!pDib) return NULL;
    int nColors = (pDib->bmiHeader.biClrUsed != 0)
        ? (int)pDib->bmiHeader.biClrUsed
        : (1 << pDib->bmiHeader.biBitCount);
    if (nColors > 256) nColors = 256;
    return (LPBYTE)pDib + pDib->bmiHeader.biSize + nColors * sizeof(RGBQUAD);
}

static BITMAPINFO* LoadDibResource(int nResId)
{
    HRSRC hRsrc = FindResourceW(g_hInst, MAKEINTRESOURCEW(nResId), RT_BITMAP);
    if (!hRsrc) return NULL;
    HGLOBAL hRes = LoadResource(g_hInst, hRsrc);
    if (!hRes) return NULL;
    return (BITMAPINFO*)LockResource(hRes);
}

static BOOL LoadBitmaps(void)
{
    int i;

    // Load the three DIB strip resources
    g_pbmTiles  = LoadDibResource(IDB_TILES);
    g_pbmDigits = LoadDibResource(IDB_DIGITS);
    g_pbmFaces  = LoadDibResource(IDB_FACES);
    if (!g_pbmTiles || !g_pbmDigits || !g_pbmFaces)
        return FALSE;

    // Compute per-frame byte offsets for tiles (16 frames, 16x16 each)
    {
        int nPerTile = DibFrameBytes(g_pbmTiles, CELL_SIZE, CELL_SIZE);
        WCHAR dbg[128];
        wsprintfW(dbg, L"DIB TILES: biBitCount=%d biSize=%d biClrUsed=%d stride=%d perTile=%d\r\n",
            g_pbmTiles->bmiHeader.biBitCount, g_pbmTiles->bmiHeader.biSize,
            g_pbmTiles->bmiHeader.biClrUsed,
            DibRowStride(g_pbmTiles, CELL_SIZE), nPerTile);
        OutputDebugStringW(dbg);
        // Also write to file for debugging
        HANDLE hf = CreateFileW(L"debug.log", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
        if (hf != INVALID_HANDLE_VALUE) {
            DWORD written;
            char buf[128]; WideCharToMultiByte(CP_ACP, 0, dbg, -1, buf, 128, NULL, NULL);
            WriteFile(hf, buf, (DWORD)strlen(buf), &written, NULL);
            CloseHandle(hf);
        }
        for (i = 0; i < 16; i++)
            g_nTileOffsets[i] = i * nPerTile;
    }

    // Compute per-frame byte offsets for digits (12 frames, 13x23 each)
    {
        int nPerDigit = DibFrameBytes(g_pbmDigits, 13, 23);
        for (i = 0; i < 12; i++)
            g_nDigitOffsets[i] = i * nPerDigit;
    }

    // Compute per-frame byte offsets for faces (6 frames, 24x24 each)
    {
        int nPerFace = DibFrameBytes(g_pbmFaces, 24, 24);
        for (i = 0; i < 6; i++)
            g_nFaceOffsets[i] = i * nPerFace;
    }

    // Get pixel data start pointers (skip header + color table)
    BYTE *pTilesPixels  = DibPixelBase(g_pbmTiles);
    BYTE *pDigitsPixels = DibPixelBase(g_pbmDigits);
    BYTE *pFacesPixels  = DibPixelBase(g_pbmFaces);

    (void)pDigitsPixels;  // used at draw-time, not at load-time
    (void)pFacesPixels;

    // Create pens for 3D border drawing
    g_hPenWhite = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    g_hPenLight = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));
    g_hPenDark  = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));

    // Create per-tile DCs by pre-blitting each tile from the DIB strip
    HDC hdcScreen = GetDC(NULL);
    for (i = 0; i < 16; i++) {
        g_hdcTiles[i] = CreateCompatibleDC(hdcScreen);
        if (g_hdcTiles[i]) {
            g_hbmTiles[i] = CreateCompatibleBitmap(hdcScreen, CELL_SIZE, CELL_SIZE);
            if (g_hbmTiles[i]) {
                SelectObject(g_hdcTiles[i], g_hbmTiles[i]);
                SetDIBitsToDevice(
                    g_hdcTiles[i], 0, 0, CELL_SIZE, CELL_SIZE,
                    0, 0, 0, CELL_SIZE,
                    pTilesPixels + g_nTileOffsets[i],
                    g_pbmTiles, DIB_RGB_COLORS);
            }
        }
    }
    ReleaseDC(NULL, hdcScreen);

    return TRUE;
}

// ============================================================================
// Board Logic
// ============================================================================

static int RandomInt(int nMax)
{
    if (nMax <= 0) return 0;
    return rand() % nMax;
}

static void ClearBoard(void)
{
    int x, y;

    // Fill entire boards with CELL_UNREVEALED
    for (y = 0; y < MAX_ROWS; y++) {
        for (x = 0; x < MAX_COLS; x++) {
            g_board[y][x] = CELL_UNREVEALED;
            g_boardDisplay[y][x] = CELL_UNREVEALED;
        }
    }

    // Top border row
    for (x = 0; x <= g_nBoardWidth + 1; x++) {
        g_board[0][x] = CELL_BORDER;
        g_boardDisplay[0][x] = CELL_BORDER;
    }
    // Bottom border row
    for (x = 0; x <= g_nBoardWidth + 1; x++) {
        g_board[g_nBoardHeight + 1][x] = CELL_BORDER;
        g_boardDisplay[g_nBoardHeight + 1][x] = CELL_BORDER;
    }
    // Left border column
    for (y = 0; y <= g_nBoardHeight + 1; y++) {
        g_board[y][0] = CELL_BORDER;
        g_boardDisplay[y][0] = CELL_BORDER;
    }
    // Right border column
    for (y = 0; y <= g_nBoardHeight + 1; y++) {
        g_board[y][g_nBoardWidth + 1] = CELL_BORDER;
        g_boardDisplay[y][g_nBoardWidth + 1] = CELL_BORDER;
    }
}

static int CountAdjacentMines(int x, int y)
{
    int count = 0, dy, dx;
    for (dy = y - 1; dy <= y + 1; dy++)
        for (dx = x - 1; dx <= x + 1; dx++)
            if (g_board[dy][dx] & MINE_FLAG)
                count++;
    return count;
}

static int CountAdjacentFlags(int x, int y)
{
    int count = 0, dy, dx;
    for (dy = y - 1; dy <= y + 1; dy++)
        for (dx = x - 1; dx <= x + 1; dx++)
            if ((g_board[dy][dx] & CELL_STATE_MASK) == CELL_FLAGGED)
                count++;
    return count;
}

static void RevealCell(int x, int y)
{
    BYTE *pCell = &g_board[y][x];

    if (*pCell & REVEALED_FLAG)
        return;

    BYTE state = *pCell & CELL_STATE_MASK;
    if (state >= CELL_QUESTION)  // 14, 15, or border (16)
        return;

    g_nRevealedCells++;
    int nMines = CountAdjacentMines(x, y);
    *pCell = (BYTE)nMines | REVEALED_FLAG;
    g_boardDisplay[y][x] = *pCell;
    DrawCell(x, y);

    if (nMines == 0) {
        g_floodX[g_nFloodQTail] = x;
        g_floodY[g_nFloodQTail] = y;
        g_nFloodQTail = (g_nFloodQTail + 1) % 100;
    }
}

static void RevealCells(int x, int y)
{
    g_nFloodQTail = 1;
    g_nFloodQHead = 1;

    RevealCell(x, y);

    while (g_nFloodQHead != g_nFloodQTail) {
        int cx = g_floodX[g_nFloodQHead];
        int cy = g_floodY[g_nFloodQHead];
        g_nFloodQHead = (g_nFloodQHead + 1) % 100;

        RevealCell(cx - 1, cy - 1);
        RevealCell(cx,     cy - 1);
        RevealCell(cx + 1, cy - 1);
        RevealCell(cx - 1, cy);
        RevealCell(cx + 1, cy);
        RevealCell(cx - 1, cy + 1);
        RevealCell(cx,     cy + 1);
        RevealCell(cx + 1, cy + 1);
    }
}

static void SetCellState(int x, int y, BYTE state)
{
    g_board[y][x] = state | (g_board[y][x] & (MINE_FLAG | REVEALED_FLAG));
    g_boardDisplay[y][x] = g_board[y][x];
    DrawCell(x, y);
}

static void RightClickCell(int x, int y)
{
    if (x < 1 || y < 1 || x > g_nBoardWidth || y > g_nBoardHeight)
        return;

    BYTE *pCell = &g_board[y][x];
    if (*pCell & REVEALED_FLAG)
        return;

    BYTE state = *pCell & CELL_STATE_MASK;

    // Cycle: unrevealed(15) -> flag(13) -> question(14) -> unrevealed(15)
    // With marks disabled: unrevealed(15) -> question(14) -> unrevealed(15)
    BYTE newState;
    if (state == CELL_UNREVEALED) {
        if (g_bMarks) {
            newState = CELL_FLAGGED;
            UpdateCounter(-1);  // flag placed -> decrement mine count
        } else {
            newState = CELL_QUESTION;
        }
    } else if (state == CELL_FLAGGED) {
        newState = CELL_QUESTION;
        if (g_bMarks)
            UpdateCounter(1);  // flag removed -> increment mine count
    } else if (state == CELL_QUESTION) {
        newState = CELL_UNREVEALED;
    } else {
        newState = CELL_UNREVEALED;
    }

    SetCellState(x, y, newState);

    // Check win condition after right-click
    if (g_nRevealedCells == g_nSafeCells && g_nMineDisplay == 0)
        EndGame(TRUE);
}

static void ClickCell(int nNewX, int nNewY)
{
    int nPrevX = g_nCurCellX;
    int nPrevY = g_nCurCellY;
    g_nCurCellX = nNewX;
    g_nCurCellY = nNewY;

    BOOL bOldValid = (nPrevX >= 1 && nPrevY >= 1 &&
                      nPrevX <= g_nBoardWidth && nPrevY <= g_nBoardHeight);
    BOOL bNewValid = (nNewX >= 1 && nNewY >= 1 &&
                      nNewX <= g_nBoardWidth && nNewY <= g_nBoardHeight);

    if (g_bBothButtons) {
        // Chord mode: highlight/unhighlight the 3x3 neighborhood
        int dx, dy;

        if (bOldValid) {
            for (dy = nPrevY - 1; dy <= nPrevY + 1; dy++) {
                if (dy < 1 || dy > g_nBoardHeight) continue;
                for (dx = nPrevX - 1; dx <= nPrevX + 1; dx++) {
                    if (dx < 1 || dx > g_nBoardWidth) continue;
                    if (!(g_board[dy][dx] & REVEALED_FLAG))
                        DrawCell(dx, dy);
                }
            }
        }
        if (bNewValid) {
            for (dy = nNewY - 1; dy <= nNewY + 1; dy++) {
                if (dy < 1 || dy > g_nBoardHeight) continue;
                for (dx = nNewX - 1; dx <= nNewX + 1; dx++) {
                    if (dx < 1 || dx > g_nBoardWidth) continue;
                    if (!(g_board[dy][dx] & REVEALED_FLAG))
                        DrawCell(dx, dy);
                }
            }
        }
    } else {
        // Single-button mode: restore old cell, show pressed state on new
        if (bOldValid)
            DrawCell(nPrevX, nPrevY);

        if (bNewValid) {
            BYTE cell = g_board[nNewY][nNewX];
            if (!(cell & REVEALED_FLAG) && (cell & CELL_STATE_MASK) != CELL_QUESTION)
                DrawCell(nNewX, nNewY);
        }
    }
}

static void ChordClick(int x, int y)
{
    BYTE cell = g_board[y][x];
    BOOL bMineTriggered = FALSE;
    int dx, dy;

    if (!(cell & REVEALED_FLAG))
        return;
    int nNum = cell & CELL_STATE_MASK;
    if (nNum > 8)
        return;

    if (CountAdjacentFlags(x, y) != nNum) {
        ClickCell(-2, -2);
        return;
    }

    for (dy = y - 1; dy <= y + 1; dy++) {
        for (dx = x - 1; dx <= x + 1; dx++) {
            BYTE adj = g_board[dy][dx];
            if (adj & REVEALED_FLAG)
                continue;
            if ((adj & CELL_STATE_MASK) == CELL_FLAGGED)
                continue;

            if (adj & MINE_FLAG) {
                bMineTriggered = TRUE;
            } else {
                RevealCell(dx, dy);
            }
        }
    }

    if (bMineTriggered)
        EndGame(FALSE);
    else if (g_nRevealedCells == g_nSafeCells)
        EndGame(TRUE);
}

// ============================================================================
// ClickNumberedCell — exact match to original sub_1003512
// Called by StartGame when clicking on an unrevealed, non-question-mark cell.
// ============================================================================
static void ClickNumberedCell(int x, int y)
{
    BYTE *pCell = &g_board[y][x];               // byte_1005340[32*y + x]

    if (!(*pCell & MINE_FLAG)) {                 // cell does NOT contain mine
        RevealCells(x, y);                       // sub_1003084
        if (g_nRevealedCells == g_nSafeCells)    // dword_10057A4 == dword_10057A0
            EndGame(TRUE);                       // sub_100347C(1)
        return;
    }

    // Cell contains a mine
    if (g_nRevealedCells > 0) {                  // game already in progress
        // Hit mine after first click → GAME OVER
        SetCellState(x, y, CELL_MINE_HIT);       // sub_1002EAB(x, y, 76) → 76 = 'L'? actually 12|0x40
        EndGame(FALSE);                          // sub_100347C(0)
        return;
    }

    // First click hit a mine → relocate it to another cell
    int ny, nx;
    for (ny = 1; ny <= g_nBoardHeight; ny++) {
        BYTE *pRow = &g_board[ny][0];
        for (nx = 1; nx <= g_nBoardWidth; nx++) {
            if (pRow[nx] & MINE_FLAG)           // already has mine
                continue;
            // Found a free cell — move mine here
            *pCell &= ~MINE_FLAG;                // remove mine from clicked cell
            pRow[nx] |= MINE_FLAG;               // place mine in new cell
            RevealCells(x, y);                   // reveal the now-safe clicked cell
            return;
        }
    }
    // Should never reach here unless board is all mines
    RevealCells(x, y);
}

// ============================================================================
// Drawing Functions
// ============================================================================

static void DrawCell(int x, int y)
{
    HDC hdc = GetDC(g_hWnd);
    if (hdc) {
        int nState = g_boardDisplay[y][x] & CELL_STATE_MASK;
        if (nState > 15) nState = 15;
        if (nState >= 0 && nState <= 15) {
            BitBlt(hdc,
                (x - 1) * CELL_SIZE + 12,
                (y - 1) * CELL_SIZE + 55,
                CELL_SIZE, CELL_SIZE,
                g_hdcTiles[nState], 0, 0, SRCCOPY);
        }
        ReleaseDC(g_hWnd, hdc);
    }
}

static void DrawAllCells(HDC hdc)
{
    int x, y;
    for (y = 1; y <= g_nBoardHeight; y++) {
        for (x = 1; x <= g_nBoardWidth; x++) {
            int nState = g_boardDisplay[y][x] & CELL_STATE_MASK;
            if (nState > 15) nState = 15;
            if (nState >= 0 && nState <= 15) {
                BitBlt(hdc,
                    (x - 1) * CELL_SIZE + 12,
                    (y - 1) * CELL_SIZE + 55,
                    CELL_SIZE, CELL_SIZE,
                    g_hdcTiles[nState], 0, 0, SRCCOPY);
            }
        }
    }
}

static void DrawDigit(HDC hdc, int x, int nDigit)
{
    if (nDigit < 0 || nDigit > 11) return;
    BYTE *pPixels = DibPixelBase(g_pbmDigits);
    if (!pPixels) return;
    SetDIBitsToDevice(hdc, x, 16, 13, 23,
        0, 0, 0, 23,
        pPixels + g_nDigitOffsets[nDigit],
        g_pbmDigits, DIB_RGB_COLORS);
}

static void DrawFace(HDC hdc, int nFace)
{
    if (nFace < 0 || nFace > 5) nFace = 0;
    BYTE *pPixels = DibPixelBase(g_pbmFaces);
    if (!pPixels) return;
    int cx = (g_nClientWidth - 24) / 2;
    SetDIBitsToDevice(hdc, cx, 16, 24, 24,
        0, 0, 0, 24,
        pPixels + g_nFaceOffsets[nFace],
        g_pbmFaces, DIB_RGB_COLORS);
}

static void DrawMineCounter(HDC hdc)
{
    int nVal = g_nMineDisplay;
    int nHundreds, nTens, nOnes;

    if (nVal < 0) {
        nHundreds = 10;    // minus-sign digit
        nVal = -nVal;
        if (nVal > 99) nVal = 99;
    } else {
        nHundreds = nVal / 100;
        if (nHundreds > 9) nHundreds = 9;
        nVal = nVal % 100;
    }
    nTens = nVal / 10;
    nOnes = nVal % 10;

    DrawDigit(hdc, 17, nHundreds);
    DrawDigit(hdc, 30, nTens);
    DrawDigit(hdc, 43, nOnes);
}

static void DrawTimer(HDC hdc)
{
    int nTime = g_nTimer;
    if (nTime > 999) nTime = 999;

    int nRight = g_nClientWidth - g_nCyEdge;
    int nHundreds = nTime / 100;
    int nTens     = (nTime % 100) / 10;
    int nOnes     = nTime % 10;

    DrawDigit(hdc, nRight - 56, nHundreds);
    DrawDigit(hdc, nRight - 43, nTens);
    DrawDigit(hdc, nRight - 30, nOnes);
}

// Draw a 3D raised or sunken rectangle border using highlight/shadow pens.
static void Draw3DRect(HDC hdc, int x, int y, int cx, int cy, int nThick, BOOL bRaised)
{
    int i;
    HPEN hOld;
    int x1 = x, y1 = y;
    int x2 = x + cx - 1, y2 = y + cy - 1;

    for (i = 0; i < nThick; i++) {
        // Top edge
        hOld = SelectObject(hdc, bRaised ? g_hPenWhite : g_hPenDark);
        MoveToEx(hdc, x1 + i, y1 + i, NULL);
        LineTo(hdc, x2 - i, y1 + i);

        // Left edge
        MoveToEx(hdc, x1 + i, y1 + i + 1, NULL);
        LineTo(hdc, x1 + i, y2 - i);

        // Bottom edge
        SelectObject(hdc, bRaised ? g_hPenDark : g_hPenWhite);
        MoveToEx(hdc, x2 - i, y2 - i, NULL);
        LineTo(hdc, x1 + i, y2 - i);

        // Right edge
        MoveToEx(hdc, x2 - i, y2 - i - 1, NULL);
        LineTo(hdc, x2 - i, y1 + i);

        SelectObject(hdc, hOld);
    }
}

static void DrawBorder(HDC hdc)
{
    int cw = g_nClientWidth;
    int ch = g_nClientHeight;

    // Outer raised frame (thickness 3)
    Draw3DRect(hdc, 0, 0, cw, ch, 3, TRUE);

    // Inner tray area (sunken) where the board sits
    Draw3DRect(hdc, 9, 52, cw - 18, ch - 61, 3, FALSE);

    // Top panel (sunken) for mine counter, face, timer
    Draw3DRect(hdc, 9, 9, cw - 18, 45, 2, FALSE);

    // Mine counter box (sunken, 1px)
    Draw3DRect(hdc, 15, 15, 46, 31, 1, FALSE);

    // Timer box (sunken, 1px)
    int nTimerRight = cw - g_nCyEdge;
    Draw3DRect(hdc, nTimerRight - 56, 15, 46, 31, 1, FALSE);

    // Face button border (raised, 1px)
    int cxFace = (cw - 24) / 2;
    Draw3DRect(hdc, cxFace - 1, 14, 26, 28, 1, TRUE);
}

// Full paint handler called from WM_PAINT
static void PaintBoard(HDC hdc)
{
    DrawBorder(hdc);
    DrawMineCounter(hdc);
    DrawFace(hdc, g_nFaceState);
    DrawTimer(hdc);
    DrawAllCells(hdc);
}

// Update just the face button
static void SetFace(int nFace)
{
    HDC hdc = GetDC(g_hWnd);
    if (hdc) {
        DrawFace(hdc, nFace);
        ReleaseDC(g_hWnd, hdc);
    }
}

// ============================================================================
// Game Flow
// ============================================================================

static void UpdateCounter(int nDelta)
{
    g_nMineDisplay += nDelta;
    HDC hdc = GetDC(g_hWnd);
    if (hdc) {
        DrawMineCounter(hdc);
        ReleaseDC(g_hWnd, hdc);
    }
}

static void SizeWindow(BYTE flags)
{
    if (!g_hWnd) return;

    int nClientWidth  = g_nBoardWidth * CELL_SIZE + 24;
    int nClientHeight = g_nBoardHeight * CELL_SIZE + 67;
    g_nClientWidth  = nClientWidth;
    g_nClientHeight = nClientHeight;

    // Calculate menu height
    g_nMenuHeight = g_nBorderTop + g_nCyEdge;
    if (g_hMenu && !(g_nMenuState & 1)) {
        RECT rc0, rc1;
        if (GetMenuItemRect(g_hWnd, g_hMenu, 0, &rc0) &&
            GetMenuItemRect(g_hWnd, g_hMenu, 1, &rc1) &&
            rc0.top != rc1.top) {
            g_nMenuHeight += g_nCyEdge;  // two-line menu
        }
    }

    int nWinWidth  = nClientWidth  + g_nCxEdge * 2;
    int nWinHeight = nClientHeight + g_nCyEdge * 2 + g_nMenuHeight;

    // Adjust position to keep window on-screen
    int nScreenW = GetSystemMetrics(SM_CXSCREEN);
    int nScreenH = GetSystemMetrics(SM_CYSCREEN);

    if (g_nWindowX + nWinWidth > nScreenW) {
        flags |= 2;
        g_nWindowX = nScreenW - nWinWidth;
        if (g_nWindowX < 0) g_nWindowX = 0;
    }
    if (g_nWindowY + nWinHeight > nScreenH) {
        flags |= 2;
        g_nWindowY = nScreenH - nWinHeight;
        if (g_nWindowY < 0) g_nWindowY = 0;
    }

    if (!g_bInit) {
        if (flags & 2) {
            SetWindowPos(g_hWnd, NULL, g_nWindowX, g_nWindowY,
                nWinWidth, nWinHeight, SWP_NOZORDER);
        }
        if (flags & 4) {
            RECT rc;
            SetRect(&rc, 0, 0, nClientWidth, nClientHeight);
            InvalidateRect(g_hWnd, &rc, TRUE);
        }
    }
}

static void TimerTick(void)
{
    if (!g_bTimerRunning) return;
    if (g_nTimer >= 999) return;

    g_nTimer++;
    HDC hdc = GetDC(g_hWnd);
    if (hdc) {
        DrawTimer(hdc);
        ReleaseDC(g_hWnd, hdc);
    }
    PlayGameSound(SOUND_TICK);
}

static void PlayGameSound(int nSoundId)
{
    if (g_nSound != SOUND_ON) return;
    PlaySoundW(MAKEINTRESOURCEW(nSoundId), g_hInst,
        SND_RESOURCE | SND_ASYNC | SND_NODEFAULT);
}

// ============================================================================
// StartGame — exact match to original sub_10037E1
// Called on mouse-up if game is active. Reads g_nCurCellX/Y set during MOUSEMOVE.
// ============================================================================
static void StartGame(void)
{
    int x = g_nCurCellX;
    int y = g_nCurCellY;

    // ONE-TIME debug: is StartGame ever reached?
    static int s_bug = 1;
    if (s_bug) {
        WCHAR z[64];
        wsprintfW(z, L"StartGame: x=%d y=%d", x, y);
        MessageBoxW(NULL, z, L"DEBUG", MB_OK);
        s_bug = 0;
    }

    if (x < 1 || y < 1 || x > g_nBoardWidth || y > g_nBoardHeight)
        return;

    // First click: start timer + play tick sound
    if (g_nRevealedCells == 0 && g_nTimer == 0) {
        PlayGameSound(SOUND_TICK);               // sub_10038ED(1)
        g_nTimer = 1;                            // ++dword_100579C
        // sub_10028B5() — redraw timer display
        HDC hdc = GetDC(g_hWnd);
        DrawTimer(hdc);
        ReleaseDC(g_hWnd, hdc);
        g_bTimerRunning = TRUE;                  // dword_1005164 = 1
        if (!SetTimer(g_hWnd, IDT_GAME_TIMER, 1000, NULL))
            ShowError(IDS_ERRORTIMER);           // sub_1003950(4)
    }

    // If game no longer active, bail
    if (!(g_dwGameFlags & GAME_ACTIVE)) {
        g_nCurCellX = -2;
        g_nCurCellY = -2;
        // fall through to SetFace
    } else if (g_bBothButtons) {
        ChordClick(x, y);                        // sub_10035B7
    } else {
        BYTE cell = g_board[y][x];              // byte_1005340[32*y + x]
        if (!(cell & REVEALED_FLAG) && (cell & CELL_STATE_MASK) != CELL_QUESTION)
            ClickNumberedCell(x, y);             // sub_1003512
    }

    SetFace(g_nFaceState);                       // sub_1002913(dword_1005160)
}

static void NewGame(void)
{
    g_bTimerRunning = FALSE;
    KillTimer(g_hWnd, IDT_GAME_TIMER);

    // If dimensions changed, resize window
    if (g_nDiffWidth == g_nBoardWidth && g_nDiffHeight == g_nBoardHeight)
        SizeWindow(4);   // same size, just invalidate
    else
        SizeWindow(6);   // different size: move + invalidate

    g_nBoardWidth  = g_nDiffWidth;
    g_nBoardHeight = g_nDiffHeight;

    ClearBoard();

    g_nTimer       = 0;
    g_nMineCount   = g_nDiffMines;
    g_nMineDisplay = g_nDiffMines;
    g_nRevealedCells = 0;
    g_nSafeCells   = g_nBoardWidth * g_nBoardHeight - g_nDiffMines;

    // Place mines randomly
    int nMinesPlaced = 0;
    while (nMinesPlaced < g_nDiffMines) {
        int mx = RandomInt(g_nBoardWidth) + 1;
        int my = RandomInt(g_nBoardHeight) + 1;
        if (!(g_board[my][mx] & MINE_FLAG) &&
            (g_board[my][mx] & CELL_STATE_MASK) != CELL_BORDER) {
            g_board[my][mx] |= MINE_FLAG;
            nMinesPlaced++;
        }
    }

    g_nMineCount   = g_nDiffMines;
    g_nMineDisplay = g_nDiffMines;
    g_nSafeCells   = g_nBoardWidth * g_nBoardHeight - g_nDiffMines;
    g_nFaceState   = FACE_NORMAL;
    g_dwGameFlags  = GAME_ACTIVE;

    // Copy board to display board
    memcpy(g_boardDisplay, g_board, sizeof(g_board));

    g_nClientWidth  = g_nBoardWidth * CELL_SIZE + 24;
    g_nClientHeight = g_nBoardHeight * CELL_SIZE + 67;

    if (!g_bInit) {
        RECT rc;
        SetRect(&rc, 0, 0, g_nClientWidth, g_nClientHeight);
        InvalidateRect(g_hWnd, &rc, TRUE);
    }

    UpdateCounter(0);
}

static void EndGame(BOOL bWon)
{
    g_bTimerRunning = FALSE;
    KillTimer(g_hWnd, IDT_GAME_TIMER);

    int x, y;

    if (bWon) {
        g_nFaceState = FACE_COOL;
        SetFace(FACE_COOL);
        PlayGameSound(SOUND_WIN);

        if (g_nMineDisplay > 0)
            UpdateCounter(-g_nMineDisplay);

        // Auto-flag remaining unflagged mines
        for (y = 1; y <= g_nBoardHeight; y++) {
            for (x = 1; x <= g_nBoardWidth; x++) {
                if ((g_board[y][x] & MINE_FLAG) && !(g_board[y][x] & REVEALED_FLAG)) {
                    g_board[y][x] = (g_board[y][x] & 0xE0) | CELL_FLAGGED;
                    g_boardDisplay[y][x] = g_board[y][x];
                }
            }
        }

        // Check best time
        if (g_nDifficulty >= DIFF_BEGINNER && g_nDifficulty <= DIFF_EXPERT) {
            if (g_nTimer > 0 && g_nTimer < g_nBestTimes[g_nDifficulty]) {
                g_nBestTimes[g_nDifficulty] = g_nTimer;
                DialogBoxParamW(g_hInst, MAKEINTRESOURCEW(IDD_CUSTOM),
                    g_hWnd, BestTimesDlgProc, (LPARAM)g_nDifficulty);
                g_bRedrawSave = TRUE;
            }
        }
    } else {
        g_nFaceState = FACE_DEAD;
        SetFace(FACE_DEAD);

        // Reveal all mines and mark wrong flags
        for (y = 1; y <= g_nBoardHeight; y++) {
            for (x = 1; x <= g_nBoardWidth; x++) {
                BYTE cell = g_board[y][x];
                if (cell & MINE_FLAG) {
                    if (!(cell & REVEALED_FLAG)) {
                        g_board[y][x] = REVEALED_FLAG | CELL_MINE_HIT;
                        g_boardDisplay[y][x] = g_board[y][x];
                    }
                } else if (!(cell & REVEALED_FLAG) &&
                           (cell & CELL_STATE_MASK) == CELL_FLAGGED) {
                    // Wrong flag mark
                    g_board[y][x] = REVEALED_FLAG | CELL_WRONG_FLAG;
                    g_boardDisplay[y][x] = g_board[y][x];
                }
            }
        }

        memcpy(g_boardDisplay, g_board, sizeof(g_board));
        InvalidateRect(g_hWnd, NULL, FALSE);
        PlayGameSound(SOUND_REVEAL);
    }

    g_dwGameFlags = GAME_OVER;
    memcpy(g_boardDisplay, g_board, sizeof(g_board));
    InvalidateRect(g_hWnd, NULL, FALSE);
}

// ============================================================================
// UI Helpers
// ============================================================================

static BOOL FaceButtonClick(LPARAM lParam)
{
    int cxFace = (g_nClientWidth - 24) / 2;
    RECT rcFace;
    POINT pt;

    SetRect(&rcFace, cxFace, 16, cxFace + 24, 40);
    pt.x = GET_X_LPARAM(lParam);
    pt.y = GET_Y_LPARAM(lParam);

    if (!PtInRect(&rcFace, pt))
        return FALSE;

    SetCapture(g_hWnd);
    g_nFaceState = FACE_SURPRISED;
    SetFace(FACE_SURPRISED);

    MapWindowPoints(g_hWnd, NULL, (LPPOINT)&rcFace, 2);

    // Modal loop tracking mouse over face button
    MSG msg;
    BOOL bInside = TRUE;
    for (;;) {
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                PostQuitMessage((int)msg.wParam);
                ReleaseCapture();
                return TRUE;
            }

            if (msg.hwnd == g_hWnd) {
                switch (msg.message) {
                    case WM_MOUSEMOVE:
                    {
                        BOOL bNowInside = PtInRect(&rcFace, msg.pt);
                        if (bNowInside != bInside) {
                            bInside = bNowInside;
                            SetFace(bInside ? FACE_SURPRISED : g_nFaceState);
                        }
                        break;
                    }
                    case WM_LBUTTONUP:
                        if (bInside && PtInRect(&rcFace, msg.pt)) {
                            ReleaseCapture();
                            g_nFaceState = FACE_NORMAL;
                            NewGame();
                            return TRUE;
                        }
                        ReleaseCapture();
                        SetFace(g_nFaceState);
                        return TRUE;
                }
            }

            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        WaitMessage();
    }
}

static void ChangeDifficulty(int nDiff)
{
    g_nDifficulty = nDiff;
    g_nMenuState  = nDiff;

    if (nDiff >= DIFF_BEGINNER && nDiff <= DIFF_EXPERT) {
        g_nDiffHeight = g_difficultyTable[nDiff][0];
        g_nDiffWidth  = g_difficultyTable[nDiff][1];
        g_nDiffMines  = g_difficultyTable[nDiff][2];
    }

    CheckMenuItem(g_hMenu, IDM_BEGINNER, MF_BYCOMMAND |
        (nDiff == DIFF_BEGINNER ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(g_hMenu, IDM_INTERMEDIATE, MF_BYCOMMAND |
        (nDiff == DIFF_INTERMEDIATE ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(g_hMenu, IDM_EXPERT, MF_BYCOMMAND |
        (nDiff == DIFF_EXPERT ? MF_CHECKED : MF_UNCHECKED));

    if (!g_bInit)
        NewGame();
    g_bRedrawSave = TRUE;
}

static void UpdateMenu(void)
{
    if (!g_hMenu) return;

    CheckMenuItem(g_hMenu, IDM_MARKS, MF_BYCOMMAND |
        (g_bMarks ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(g_hMenu, IDM_SOUND, MF_BYCOMMAND |
        (g_nSound == SOUND_ON ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(g_hMenu, IDM_COLOR, MF_BYCOMMAND |
        (g_bColor ? MF_CHECKED : MF_UNCHECKED));
}

static void ShowBestTimes(void)
{
    DialogBoxParamW(g_hInst, MAKEINTRESOURCEW(IDD_CUSTOM),
        g_hWnd, BestTimesDlgProc, (LPARAM)-1);
}

static void ShowError(int nErrCode)
{
    WCHAR szMsg[256];
    WCHAR szCap[64];
    LoadStringW(g_hInst, nErrCode, szMsg, 256);
    LoadStringW(g_hInst, IDS_APPNAME, szCap, 64);
    MessageBoxW(g_hWnd, szMsg, szCap, MB_OK | MB_ICONERROR);
}

// ============================================================================
// Dialog Procedures
// ============================================================================

// Custom Game dialog (IDD_CUSTOM = 80)
INT_PTR CALLBACK CustomDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    (void)lParam;

    switch (msg) {
        case WM_INITDIALOG:
            SetDlgItemInt(hDlg, IDC_HEIGHT, (UINT)g_nDiffHeight, FALSE);
            SetDlgItemInt(hDlg, IDC_WIDTH,  (UINT)g_nDiffWidth,  FALSE);
            SetDlgItemInt(hDlg, IDC_MINES,  (UINT)g_nDiffMines,  FALSE);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                {
                    BOOL bValid;
                    int h = (int)GetDlgItemInt(hDlg, IDC_HEIGHT, &bValid, FALSE);
                    int w = (int)GetDlgItemInt(hDlg, IDC_WIDTH,  &bValid, FALSE);
                    int m = (int)GetDlgItemInt(hDlg, IDC_MINES,  &bValid, FALSE);

                    if (h < 9)  h = 9;
                    if (h > 24) h = 24;
                    if (w < 9)  w = 9;
                    if (w > 30) w = 30;

                    int nMaxMines = (h - 1) * (w - 1);
                    if (m < 10)       m = 10;
                    if (m > nMaxMines) m = nMaxMines;

                    g_nDiffHeight = h;
                    g_nDiffWidth  = w;
                    g_nDiffMines  = m;

                    g_nDifficulty = DIFF_CUSTOM;
                    g_nMenuState  = DIFF_CUSTOM;

                    CheckMenuItem(g_hMenu, IDM_BEGINNER,     MF_BYCOMMAND | MF_UNCHECKED);
                    CheckMenuItem(g_hMenu, IDM_INTERMEDIATE, MF_BYCOMMAND | MF_UNCHECKED);
                    CheckMenuItem(g_hMenu, IDM_EXPERT,       MF_BYCOMMAND | MF_UNCHECKED);

                    EndDialog(hDlg, IDOK);
                    return TRUE;
                }

                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
            }
            return FALSE;

        case WM_HELP:
            WinHelpW(((LPHELPINFO)lParam)->hItemHandle,
                L"winmine.hlp", HELP_CONTEXT, 0x1005040);
            return TRUE;

        case WM_CONTEXTMENU:
            WinHelpW((HWND)wParam, L"winmine.hlp", HELP_CONTEXT, 0x1005040);
            return TRUE;
    }
    return FALSE;
}

// Best Times dialog (reuses IDD_CUSTOM).
// lParam = -1 for display-only, or 0/1/2 for new best-time name entry.
INT_PTR CALLBACK BestTimesDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int s_nDiff = -1;

    switch (msg) {
        case WM_INITDIALOG:
            s_nDiff = (int)lParam;
            if (s_nDiff >= 0 && s_nDiff <= 2) {
                // New best time entry mode
                WCHAR szTime[32];
                wsprintfW(szTime, L"%d", g_nBestTimes[s_nDiff]);
                SetDlgItemTextW(hDlg, 100 + s_nDiff, szTime);
                SetDlgItemTextW(hDlg, 200 + s_nDiff, g_szBestNames[s_nDiff]);
            } else {
                // Display-only mode
                WCHAR szTime[32];
                int i;
                for (i = 0; i < 3; i++) {
                    wsprintfW(szTime, L"%d", g_nBestTimes[i]);
                    SetDlgItemTextW(hDlg, 100 + i, szTime);
                    SetDlgItemTextW(hDlg, 200 + i, g_szBestNames[i]);
                }
            }
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                    if (s_nDiff >= 0 && s_nDiff <= 2) {
                        GetDlgItemTextW(hDlg, 200 + s_nDiff,
                            g_szBestNames[s_nDiff], 32);
                        if (g_szBestNames[s_nDiff][0] == L'\0')
                            wcscpy_s(g_szBestNames[s_nDiff], 32, g_szAnonName);
                    }
                    EndDialog(hDlg, IDOK);
                    return TRUE;

                case IDCANCEL:
                    if (s_nDiff >= 0 && s_nDiff <= 2) {
                        g_nBestTimes[s_nDiff] = 999;
                        wcscpy_s(g_szBestNames[s_nDiff], 32, g_szAnonName);
                    }
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
            }
            return FALSE;
    }
    return FALSE;
}

// ============================================================================
// Window Procedure
// ============================================================================

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_CREATE:
            return 0;

        case WM_DESTROY:
            KillTimer(hWnd, IDT_GAME_TIMER);
            if (g_bRedrawSave)
                SaveSettings();
            PostQuitMessage(0);
            return 0;

        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE)
                g_bCancelFlag = TRUE;
            return 0;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            PaintBoard(hdc);
            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_WINDOWPOSCHANGED:
        {
            LPWINDOWPOS pwp = (LPWINDOWPOS)lParam;
            if (!(pwp->flags & SWP_NOMOVE))
                if (!(g_dwGameFlags & GAME_POS_LOCK)) {
                    g_nWindowX = pwp->x;
                    g_nWindowY = pwp->y;
                }
            break;
        }

        case WM_KEYDOWN:
            switch (wParam) {
                case VK_SHIFT:
                    if (g_nCheatState >= 5)
                        g_nCheatState ^= 0x14;  // toggle xyzzy pixel
                    break;

                case 'X': case 'x':
                case 'Y': case 'y':
                case 'Z': case 'z':
                {
                    BYTE ch = (BYTE)(wParam & 0xFF);
                    if (ch >= 'a' && ch <= 'z')
                        ch -= 32;
                    if (g_nCheatState < 5) {
                        if (ch == g_CheatKeys[g_nCheatState])
                            g_nCheatState++;
                        else
                            g_nCheatState = 0;
                    }
                    break;
                }

                case VK_F2:
                    g_nSound = (g_nSound == SOUND_ON) ? 0 : SOUND_ON;
                    UpdateMenu();
                    break;

                case VK_F3:
                    ChangeDifficulty(DIFF_INTERMEDIATE);
                    break;

                case VK_F4:
                    ChangeDifficulty(DIFF_EXPERT);
                    break;
            }
            break;

        case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);

            if (wmId >= IDM_BEGINNER && wmId <= IDM_CUSTOM) {
                if (wmId == IDM_CUSTOM) {
                    INT_PTR result = DialogBoxW(g_hInst,
                        MAKEINTRESOURCEW(IDD_CUSTOM), hWnd, CustomDlgProc);
                    if (result == IDOK) {
                        NewGame();
                        g_bRedrawSave = TRUE;
                    }
                } else {
                    ChangeDifficulty(wmId - IDM_BEGINNER);
                }
                return 0;
            }

            switch (wmId) {
                case IDM_NEW:
                    NewGame();
                    break;

                case IDM_BESTTIMES:
                    ShowBestTimes();
                    break;

                case IDM_MARKS:
                    g_bMarks = !g_bMarks;
                    UpdateMenu();
                    break;

                case IDM_SOUND:
                    g_nSound = (g_nSound == SOUND_ON) ? 0 : SOUND_ON;
                    UpdateMenu();
                    break;

                case IDM_COLOR:
                    g_bColor = !g_bColor;
                    UpdateMenu();
                    break;

                case IDM_EXIT:
                    DestroyWindow(hWnd);
                    break;

                case IDM_HELP_CONTENTS:
                    WinHelpW(hWnd, L"winmine.hlp", HELP_CONTENTS, 0);
                    break;

                case IDM_HELP_SEARCH:
                    WinHelpW(hWnd, L"winmine.hlp", HELP_PARTIALKEY, 0);
                    break;

                case IDM_HELP_HOWTO:
                    WinHelpW(hWnd, L"winmine.hlp", HELP_CONTEXT, 0x1005000);
                    break;

                case IDM_ABOUT:
                {
                    WCHAR szCap[64];
                    LoadStringW(g_hInst, IDS_APPNAME, szCap, 64);
                    ShellAboutW(hWnd, szCap, L"",
                        LoadIconW(g_hInst, MAKEINTRESOURCEW(IDI_APP)));
                    break;
                }

                default:
                    return DefWindowProcW(hWnd, msg, wParam, lParam);
            }
            break;
        }

        case WM_SYSCOMMAND:
        {
            UINT uCmd = wParam & 0xFFF0;
            if (uCmd == SC_MINIMIZE)
                g_dwGameFlags |= GAME_POS_LOCK;
            else if (uCmd == SC_RESTORE) {
                g_dwGameFlags &= ~GAME_POS_LOCK;
                g_bCancelFlag = FALSE;
            }
            return DefWindowProcW(hWnd, msg, wParam, lParam);
        }

        case WM_TIMER:
            if (wParam == IDT_GAME_TIMER) {
                TimerTick();
                return 0;
            }
            break;

        // ── WM_LBUTTONDOWN (0x201) ──
        // Exact match to original: sub_1001BC9 case 0x201
        case WM_LBUTTONDOWN:
            if (g_bCancelFlag) {
                g_bCancelFlag = FALSE;           // LABEL_76
                return 0;
            }
            if (FaceButtonClick(lParam))         // sub_100140C
                return 0;
            if (!(g_dwGameFlags & GAME_ACTIVE))  // (dword_1005000 & 1) == 0
                break;                           // DefWindowProc
            // LABEL_91:
            g_bBothButtons = (wParam & 6) != 0;  // MK_LBUTTON|MK_RBUTTON = 6
            SetCapture(hWnd);
            g_nCurCellX = -1;
            g_nCurCellY = -1;
            g_bMouseDown = TRUE;
            SetFace(FACE_CLICKED);               // sub_1002913(1)
            return 0;

        // ── WM_RBUTTONDOWN (0x204) ──
        case WM_RBUTTONDOWN:
            if (g_bCancelFlag) {
                g_bCancelFlag = FALSE;           // LABEL_76
                return 0;
            }
            if (!(g_dwGameFlags & GAME_ACTIVE))
                break;
            if (g_bMouseDown) {                  // chord: left already down
                ClickCell(-3, -3);              // sub_10031D4(-3, -3)
                g_bBothButtons = TRUE;           // dword_1005144 = 1
                PostMessageW(g_hWnd, WM_MOUSEMOVE, wParam, lParam);
                return 0;
            }
            if (!(wParam & MK_LBUTTON)) {        // right only (no left)
                if (!g_bMenuLoop) {
                    int cx = ((WORD)lParam + 4) >> 4;
                    int cy = ((int)(short)HIWORD(lParam) - 39) >> 4;
                    RightClickCell(cx, cy);      // sub_100374F
                }
                return 0;
            }
            // Left also down: treat as chord
            g_bBothButtons = TRUE;
            SetCapture(hWnd);
            g_nCurCellX = -1;
            g_nCurCellY = -1;
            g_bMouseDown = TRUE;
            SetFace(FACE_CLICKED);
            return 0;

        // ── WM_MBUTTONDOWN (0x207) ──
        case WM_MBUTTONDOWN:
            if (g_bCancelFlag) {
                g_bCancelFlag = FALSE;           // LABEL_76
                return 0;
            }
            if (!(g_dwGameFlags & GAME_ACTIVE))
                break;
            g_bBothButtons = TRUE;               // chord mode
            // LABEL_91:
            SetCapture(hWnd);
            g_nCurCellX = -1;
            g_nCurCellY = -1;
            g_bMouseDown = TRUE;
            SetFace(FACE_CLICKED);
            return 0;

        // ── WM_MOUSEMOVE (0x200 = 512) ──
        // Exact match to original: LABEL_92
        case WM_MOUSEMOVE:
        {
            if (g_bMouseDown) {                  // dword_1005140
                if (g_dwGameFlags & GAME_ACTIVE) {
                    int cx = ((WORD)lParam + 4) >> 4;
                    int cy = ((int)(short)HIWORD(lParam) - 39) >> 4;
                    ClickCell(cx, cy);           // sub_10031D4
                } else {
                    // LABEL_84
                    g_bMouseDown = FALSE;
                    ReleaseCapture();
                    if (g_dwGameFlags & GAME_ACTIVE)
                        StartGame();             // sub_10037E1
                    else
                        ClickCell(-2, -2);       // sub_10031D4(-2, -2)
                }
            }
            // xyzzy cheat pixel
            if (g_nCheatState >= 5 && g_nCheatState != 5) {
                if (!g_bMenuLoop) {
                    int cx = ((WORD)lParam + 4) >> 4;
                    int cy = ((int)(short)HIWORD(lParam) - 39) >> 4;
                    if (cx > 0 && cy > 0 && cx <= g_nBoardWidth && cy <= g_nBoardHeight) {
                        HDC hdc = GetDC(NULL);
                        SetPixel(hdc, 0, 0, (g_board[cy][cx] & MINE_FLAG) ? 0 : 0xFFFFFF);
                        ReleaseDC(NULL, hdc);
                    }
                }
            }
            break;                               // DefWindowProc
        }

        // ── WM_LBUTTONUP (0x202) / WM_RBUTTONUP (0x205) / WM_MBUTTONUP (0x208) ──
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
            if (g_bMouseDown) {
                // LABEL_84
                g_bMouseDown = FALSE;
                ReleaseCapture();
                if (g_dwGameFlags & GAME_ACTIVE)
                    StartGame();                 // sub_10037E1
                else
                    ClickCell(-2, -2);           // sub_10031D4(-2, -2)
            }
            return DefWindowProcW(hWnd, msg, wParam, lParam);

        case WM_ENTERMENULOOP:
            g_bMenuLoop = TRUE;
            return 0;

        case WM_EXITMENULOOP:
            g_bMenuLoop = FALSE;
            return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ============================================================================
// WinMain -- Application Entry Point
// ============================================================================

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    HACCEL hAccel;

    (void)hPrevInstance;
    (void)lpCmdLine;

    g_hInst = hInstance;

    // Get system metrics for border/frame sizes
    g_nBorderTop  = GetSystemMetrics(SM_CYCAPTION) + 1;
    g_nBorderLeft = GetSystemMetrics(SM_CYEDGE) + 1;
    g_nCxEdge     = GetSystemMetrics(SM_CXEDGE) + 1;
    g_nCyEdge     = GetSystemMetrics(SM_CYEDGE) + 1;

    // Load settings from registry
    LoadSettings();

    if (nCmdShow == SW_SHOWMINNOACTIVE || nCmdShow == SW_SHOWMINIMIZED)
        g_bInit = TRUE;

    // Initialize common controls
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC  = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);

    // Register window class
    WNDCLASSW wc;
    wc.style         = 0;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APP));
    wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = g_szAppName;

    if (!RegisterClassW(&wc)) {
        ShowError(IDS_ERRORTIMER);
        return 1;
    }

    // Load menu and accelerators
    g_hMenu = LoadMenuW(hInstance, MAKEINTRESOURCEW(IDM_MAIN));
    hAccel  = LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(IDA_ACCEL));

    if (g_hMenu)
        UpdateMenu();

    // Set board dimensions from current difficulty
    if (g_nDifficulty >= DIFF_BEGINNER && g_nDifficulty <= DIFF_EXPERT) {
        g_nDiffHeight = g_difficultyTable[g_nDifficulty][0];
        g_nDiffWidth  = g_difficultyTable[g_nDifficulty][1];
        g_nDiffMines  = g_difficultyTable[g_nDifficulty][2];
    }

    g_nBoardWidth  = g_nDiffWidth;
    g_nBoardHeight = g_nDiffHeight;

    g_nClientWidth  = g_nBoardWidth * CELL_SIZE + 24;
    g_nClientHeight = g_nBoardHeight * CELL_SIZE + 67;
    g_nMenuHeight   = g_nBorderTop + g_nCyEdge;

    int nWinWidth  = g_nClientWidth  + g_nCxEdge * 2;
    int nWinHeight = g_nClientHeight + g_nCyEdge * 2 + g_nMenuHeight;

    // Create the window
    g_hWnd = CreateWindowExW(0,
        g_szAppName, g_szAppName,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        g_nWindowX, g_nWindowY,
        nWinWidth, nWinHeight,
        NULL, g_hMenu, hInstance, NULL);

    if (!g_hWnd) {
        MessageBoxW(NULL, L"Failed to create window.",
            L"Minesweeper", MB_OK | MB_ICONERROR);
        return 1;
    }

    SizeWindow(1);  // initial size calculation (no move, no invalidate)

    // Load bitmap resources (must happen after window creation for DC)
    if (!LoadBitmaps()) {
        ShowError(IDS_ERRORMEM);
        return 1;
    }

    // Set difficulty menu checkmarks
    if (g_nDifficulty >= DIFF_BEGINNER && g_nDifficulty <= DIFF_CUSTOM) {
        CheckMenuItem(g_hMenu, IDM_BEGINNER, MF_BYCOMMAND |
            (g_nDifficulty == DIFF_BEGINNER ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(g_hMenu, IDM_INTERMEDIATE, MF_BYCOMMAND |
            (g_nDifficulty == DIFF_INTERMEDIATE ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(g_hMenu, IDM_EXPERT, MF_BYCOMMAND |
            (g_nDifficulty == DIFF_EXPERT ? MF_CHECKED : MF_UNCHECKED));
    }

    // Start initial game
    NewGame();

    // Show window
    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    g_bInit = FALSE;

    // Main message loop
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (!TranslateAcceleratorW(g_hWnd, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return (int)msg.wParam;
}
