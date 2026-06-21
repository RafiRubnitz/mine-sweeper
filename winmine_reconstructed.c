// Reconstructed Minesweeper - WINMINE_1.EXE
// Image base: 0x01000000
// PDB: winmine.pdb (not available)
// Compiled: MSVC, 32-bit Windows GUI

#include <windows.h>
#include <stdlib.h>
#include <time.h>
#include <commctrl.h>

// ============================================================================
// Constants
// ============================================================================
#define MINE_FLAG       0x80  // bit 7: cell contains a mine
#define REVEALED_FLAG   0x40  // bit 6: cell is revealed
#define CELL_STATE_MASK 0x1F  // bits 0-4: cell display state
#define CELL_BORDER     16    // border cell marker
#define CELL_UNREVEALED 15    // untouched cell
#define CELL_QUESTION   14    // question mark
#define CELL_FLAGGED    13    // flagged
#define CELL_MINE_HIT   12    // (not in original - note: 12 not used)
#define CELL_WRONG_FLAG 11    // (not in original - note: uses question flag variant)
#define CELL_EMPTY_FLAG 9     // flagged but empty
#define CELL_0          0     // 0 adjacent mines
#define CELL_1          1     // 1 adjacent mine
// ... CELL_2 through CELL_8 = 2-8

#define FACE_NORMAL     0
#define FACE_CLICKED    1
#define FACE_SURPRISED  2
#define FACE_DEAD        3
#define FACE_COOL        4

#define SOUND_TICK      1  // resource 0x1B0
#define SOUND_REVEAL    2  // resource 0x1B1
#define SOUND_WIN       3  // resource 0x1B2

#define DIFF_BEGINNER   0
#define DIFF_INTERMEDIATE 1
#define DIFF_EXPERT     2
#define DIFF_CUSTOM     3

#define MAX_ROWS        32  // max board height (uses 32-byte rows)
#define MAX_COLS        32  // max board width

#define IDT_TIMER       1

// Menu command IDs
#define IDM_NEW         510
#define IDM_BEGINNER    521
#define IDM_INTERMEDIATE 522
#define IDM_EXPERT      523
#define IDM_CUSTOM      524
#define IDM_MARKS       525  // 0x20C - toggle marks (?)
#define IDM_COLOR       527  // 0x20E - toggle color
#define IDM_SOUND       526  // 0x20D - toggle sound (not directly, uses sub_10038C2)
#define IDM_BESTTIMES   528
#define IDM_EXIT        529
#define IDM_ABOUT       530  // (check actual - about is 0x251/593)
#define IDM_HELP        0x251  // help contents

// ============================================================================
// Global Variables - reconstructed from .data section
// ============================================================================

// --- Game state flags (at 0x1005000) ---
DWORD g_dwGameFlags = 0;       // 0x1005000
// bit 0: game active (1 = game in progress)
// bit 1: ?
// bit 3: ?
// bit 4: game ended (0x10 = game over)

// --- Difficulty settings table (at 0x1005010) ---
// [difficulty][3]: {width, height, mines}
int g_difficultyTable[4][3] = {  // 0x1005010
    {9, 9, 10},     // beginner
    {16, 16, 40},   // intermediate
    {30, 16, 99},   // expert
    {0, 0, 0}       // custom (set at runtime)
};

// --- Cheat code string "xyzzy" (at 0x1005034) ---
// word_1005034 = L"xyzzy" (key codes for shift+x, y, z, z, y)

// --- Mouse tracking ---
int g_nCurCellX = 0;           // 0x1005118  - current cell x under mouse
int g_nCurCellY = 0;           // 0x100511C  - current cell y under mouse
BOOL g_bMouseDown = 0;         // 0x1005140  - mouse button held
BOOL g_bBothButtons = 0;       // 0x1005144  - both buttons active (chord)
BOOL g_bCancelFlag = 0;        // 0x1005148  - game cancel/abort flag
BOOL g_bMenuLoop = 0;          // 0x100514C  - menu loop active
int  g_nCheatState = 0;        // 0x1005154  - cheat code input progress
int  g_nFaceState = 0;         // 0x1005160  - face button state
BOOL g_bTimerRunning = 0;      // 0x1005164  - timer running
int  g_bRedrawSave = 0;        // 0x100515C  - needs redraw/save

// --- Board dimensions ---
int g_nBoardWidth = 0;         // 0x1005334
int g_nBoardHeight = 0;        // 0x1005338
int g_nMineCount = 0;          // 0x1005330  - total mines
int g_nMineDisplay = 0;        // 0x1005194  - mine counter display value

// --- Board data ---
// Board is 32 columns per row (powers of 2 alignment)
// byte_1005340: internal board state
// byte_1005360: display board state
BYTE g_board[MAX_ROWS][MAX_COLS] = {0};     // 0x1005340
BYTE g_boardDisplay[MAX_ROWS][MAX_COLS] = {0}; // 0x1005360

// --- Game progress ---
int g_nSafeCells = 0;          // 0x10057A0  - total cells without mines
int g_nRevealedCells = 0;      // 0x10057A4  - count of revealed safe cells
int g_nTimer = 0;              // 0x100579C  - timer seconds
int g_nFloodQueueIdx = 0;      // 0x1005798  - flood fill queue index

// --- Flood fill queue (circular buffer, 100 entries) ---
int g_floodX[100];             // 0x10051A0
int g_floodY[100];             // 0x10057C0

// --- Settings ---
int g_nDifficulty = 0;         // 0x10056A0  // actually WORD at this address
int g_nDiffMines = 0;          // 0x10056A4
int g_nDiffHeight = 0;         // 0x10056A8  // uValue
int g_nDiffWidth = 0;          // 0x10056AC
int g_nWindowX = 0;            // 0x10056B0  // X
int g_nWindowY = 0;            // 0x10056B4  // Y
BOOL g_bSound = 0;             // 0x10056B8  // sound state
BOOL g_bMarks = 0;             // 0x10056BC  // marks (question mark) toggle
int  g_nTickCount = 0;         // 0x10056C0
int  g_nMenuState = 0;         // 0x10056C4  // difficulty level cached for menu
BOOL g_bColor = 0;             // 0x10056C8  // color/mono flag
int  g_nBestBeginner = 0;      // 0x10056CC
int  g_nBestIntermediate = 0;  // 0x10056D0
int  g_nBestExpert = 0;        // 0x10056D4
WCHAR g_szBestBeginner[32];    // 0x10056D8
WCHAR g_szBestIntermediate[32]; // 0x1005718
WCHAR g_szBestExpert[32];      // 0x1005758

// --- Window/UI ---
HWND  g_hWnd = NULL;           // 0x1005B24
HINSTANCE g_hModule = NULL;    // 0x1005B30
HMENU g_hMenu = NULL;          // 0x1005A94
HICON g_hIcon = NULL;          // 0x1005B28
int   g_nBorderTop = 0;        // 0x1005B80
int   g_nBorderLeft = 0;       // 0x1005B34
int   g_nBorderWidth = 0;      // 0x1005B84
int   g_nBorderHeight = 0;     // 0x1005A90
int   g_nClientWidth = 0;      // 0x1005B2C  // xRight
int   g_nClientHeight = 0;     // 0x1005B20  // yBottom
int   g_nMenuOffset = 0;       // 0x1005B88
BOOL  g_bInit = 0;             // 0x1005B38

// --- Bitmap resources ---
BITMAPINFO* g_pbmTiles = NULL;      // 0x1005A04 (lpbmi) - resource 410
BITMAPINFO* g_pbmDigits = NULL;     // 0x100595C - resource 420
BITMAPINFO* g_pbmFaces = NULL;      // 0x1005A00 - resource 430

// Tile DCs (16 tile types x 16x16 pixels)
HDC   g_hdcTiles[16];          // 0x1005A20
HBITMAP g_hbmTiles[16];        // 0x1005980
int   g_nTileOffsets[16];      // 0x10059C0

// Digit DCs (11 digits x 13x23 pixels)
int   g_nDigitOffsets[12];     // 0x1005A60

// Face DCs (6 faces x 24x24 pixels)
int   g_nFaceOffsets[6];       // 0x1005960

HPEN  g_hPen;                  // 0x1005158

HKEY  g_hKey = NULL;           // 0x1005950

WCHAR g_szAppName[32];         // 0x1005AA0 (Buffer)
WCHAR g_szPlayerName[32];      // 0x1005AE0

// ============================================================================
// Forward Declarations
// ============================================================================

// Registry helpers
LSTATUS ReadRegInt(int nId, int nDefault, int nMin, int nMax);
LSTATUS ReadRegStr(int nId, LPWSTR pszOut, int nMaxLen);
LSTATUS WriteRegInt(int nId, int nValue);
LSTATUS WriteRegStr(int nId, LPCWSTR pszValue);

// Resource loading
HRSRC FindResourceById(int nResId);
int ComputeBitmapOffset(int nWidth, int nHeight);
BOOL LoadBitmaps(void);

// Board logic
void ClearBoard(void);
void NewGame(void);
int  CountAdjacentMines(int x, int y);
int  CountAdjacentFlags(int x, int y);
void RevealCell(int x, int y);
void RevealCells(int x, int y);
void ClickCell(int x, int y);
void RightClickCell(int x, int y);
void ChordClick(int x, int y);
void ClickNumberedCell(int x, int y);
void SetCellState(int x, int y, BYTE state);
void ToggleFlagState(int x, int y);
void FlagToQuestion(int x, int y);

// Drawing
void DrawCell(int x, int y);
void DrawAllCells(HDC hdc);
void DrawBorder(HDC hdc);
void DrawMineCounter(HDC hdc);
void DrawTimer(HDC hdc);
void DrawFace(HDC hdc, int nFace);
void DrawDigit(HDC hdc, int x, int nDigit);
void Draw3DRect(HDC hdc, int x, int y, int w, int h, int nThick, int nStyle);
void SetPen(int nStyle);
void PaintBoard(HDC hdc);
void SetFace(int nFace);

// Game flow
void EndGame(BOOL bWon);
void UpdateCounter(int nDelta);
void SizeWindow(BYTE flags);
void StartGame(void);
void TimerTick(void);

// Sound
void PlayGameSound(int nSound);

// Helpers
int  RandomInt(int nMax);
BOOL FaceButtonClick(LPARAM lParam);
void ChangeDifficulty(int nDiff);
void ShowError(int nErrCode);

// Dialog procedures
INT_PTR CALLBACK CustomDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK BestTimesDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

// ============================================================================
// Registry Helpers
// ============================================================================

// sub_1003A12
LSTATUS ReadRegInt(int nId, int nDefault, int nMin, int nMax) {
    // Implementation uses RegQueryValueExW with value name from string table
    // Returns value clamped to [nMin, nMax], writes to corresponding global
    return ERROR_SUCCESS;
}

// sub_10039E7
LSTATUS ReadRegStr(int nId, LPWSTR pszOut, int nMaxLen) {
    // Reads string from registry, defaults from string table resource
    return ERROR_SUCCESS;
}

// sub_1002D55
LSTATUS WriteRegInt(int nId, int nValue) {
    // Writes integer to registry
    return ERROR_SUCCESS;
}

// sub_1002D7A
LSTATUS WriteRegStr(int nId, LPCWSTR pszValue) {
    // Writes string to registry
    return ERROR_SUCCESS;
}

// ============================================================================
// Resource Loading
// ============================================================================

// sub_10023CD - FindResource wrapper
HRSRC FindResourceById(int nResId) {
    return FindResourceW(g_hModule, MAKEINTRESOURCEW(nResId), RT_BITMAP);
}

// sub_10023F1 - Compute bitmap offset for stride alignment
int ComputeBitmapOffset(int nWidth, int nHeight) {
    // Computes DWORD-aligned stride: ((nWidth * bitsPerPixel + 31) / 32) * 4 * nHeight
    return ((nWidth * 8 + 31) / 32) * 4;
}

// sub_1002414 - Load all bitmaps from resources
BOOL LoadBitmaps(void) {
    HRSRC hRsrc;
    HGLOBAL hRes;

    // Load tile bitmap (resource 410)
    hRsrc = FindResourceById(410);
    if (hRsrc) {
        hRes = LoadResource(g_hModule, hRsrc);
        g_pbmTiles = (BITMAPINFO*)LockResource(hRes);
    }

    // Load digit bitmap (resource 420)
    hRsrc = FindResourceById(420);
    if (hRsrc) {
        hRes = LoadResource(g_hModule, hRsrc);
        g_pbmDigits = (BITMAPINFO*)LockResource(hRes);
    }

    // Load face bitmap (resource 430)
    hRsrc = FindResourceById(430);
    if (hRsrc) {
        hRes = LoadResource(g_hModule, hRsrc);
        g_pbmFaces = (BITMAPINFO*)LockResource(hRes);
    }

    if (!g_pbmTiles || !g_pbmDigits || !g_pbmFaces)
        return FALSE;

    // Create pen for 3D borders
    if (g_bColor)
        g_hPen = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
    else
        g_hPen = GetStockObject(BLACK_PEN);

    // Compute tile bitmap offsets (16 tiles, each 16x16)
    int nTileStride = ComputeBitmapOffset(16, 16);
    for (int i = 0; i < 16; i++)
        g_nTileOffsets[i] = i * nTileStride;

    // Compute digit bitmap offsets (11 digits, each 13x23)
    int nDigitStride = ComputeBitmapOffset(13, 23);
    for (int i = 0; i < 12; i++)
        g_nDigitOffsets[i] = i * nDigitStride;

    // Compute face bitmap offsets (6 faces, each 24x24)
    int nFaceStride = ComputeBitmapOffset(24, 24);
    for (int i = 0; i < 6; i++)
        g_nFaceOffsets[i] = i * nFaceStride;

    // Create tile DCs and bitmaps
    HDC hdc = GetDC(g_hWnd);
    for (int i = 0; i < 16; i++) {
        g_hdcTiles[i] = CreateCompatibleDC(hdc);
        if (!g_hdcTiles[i])
            OutputDebugStringA("FLoad failed to create compatible dc\n");

        g_hbmTiles[i] = CreateCompatibleBitmap(hdc, 16, 16);
        if (!g_hbmTiles[i])
            OutputDebugStringA("Failed to create Bitmap\n");

        SelectObject(g_hdcTiles[i], g_hbmTiles[i]);
        SetDIBitsToDevice(g_hdcTiles[i], 0, 0, 16, 16, 0, 0, 0, 16,
            (BYTE*)g_pbmTiles + g_nTileOffsets[i], g_pbmTiles, DIB_RGB_COLORS);
    }
    ReleaseDC(g_hWnd, hdc);

    return TRUE;
}

// ============================================================================
// Board Logic
// ============================================================================

// sub_1002ED5 - Clear board arrays
void ClearBoard(void) {
    // Fill g_board with CELL_UNREVEALED (15)
    for (int i = 0; i < MAX_ROWS * MAX_COLS; i++)
        *((BYTE*)g_board + i) = CELL_UNREVEALED;

    // Fill borders with CELL_BORDER (16)
    // Top and bottom edge row (row 0 and row height+1) in display board
    for (int x = 0; x <= g_nBoardWidth + 1; x++) {
        g_board[0][x] = CELL_BORDER;
        g_boardDisplay[g_nBoardHeight + 1][x] = CELL_BORDER;
    }

    // Left and right edge columns
    for (int y = 0; y <= g_nBoardHeight + 1; y++) {
        g_board[y][0] = CELL_BORDER;
        g_board[y][g_nBoardWidth + 1] = CELL_BORDER;
    }
}

// sub_1002F3B - Count adjacent mines
int CountAdjacentMines(int x, int y) {
    int nCount = 0;
    if (y - 1 <= y + 1) {  // always true check for valid bounds
        BYTE* pRow = &g_board[y - 1][0];
        for (int dy = 0; dy < 3; dy++) {
            for (int dx = x - 1; dx <= x + 1; dx++) {
                if (pRow[dx] & MINE_FLAG)
                    nCount++;
            }
            pRow += MAX_COLS;
        }
    }
    return nCount;
}

// sub_1003119 - Count adjacent flags
int CountAdjacentFlags(int x, int y) {
    int nCount = 0;
    if (y - 1 <= y + 1) {
        BYTE* pRow = &g_board[y - 1][0];
        for (int dy = 0; dy < 3; dy++) {
            for (int dx = x - 1; dx <= x + 1; dx++) {
                if ((pRow[dx] & CELL_STATE_MASK) == CELL_FLAGGED)
                    nCount++;
            }
            pRow += MAX_COLS;
        }
    }
    return nCount;
}

// sub_1003008 - Reveal single cell
void RevealCell(int x, int y) {
    BYTE* pCell = &g_board[y][x];
    if (pCell[0] & REVEALED_FLAG)
        return;

    BYTE state = pCell[0] & CELL_STATE_MASK;
    if (state == CELL_BORDER || state == CELL_QUESTION)
        return;

    g_nRevealedCells++;

    int nMines = CountAdjacentMines(x, y);
    pCell[0] = nMines | REVEALED_FLAG;
    DrawCell(x, y);

    // If zero mines adjacent, add to flood fill queue
    if (nMines == 0) {
        int idx = g_nFloodQueueIdx;
        g_floodX[idx] = x;
        g_floodY[idx] = y;
        g_nFloodQueueIdx++;
        if (g_nFloodQueueIdx >= 100)
            g_nFloodQueueIdx = 0;
    }
}

// sub_1003084 - Flood fill empty cells
void RevealCells(int x, int y) {
    int nQueuePos = 1;
    g_nFloodQueueIdx = 1;

    RevealCell(x, y);

    while (g_nFloodQueueIdx != nQueuePos) {
        int cx = g_floodX[nQueuePos];
        int cy = g_floodY[nQueuePos];

        RevealCell(cx - 1, cy - 1);
        RevealCell(cx, cy - 1);
        RevealCell(cx + 1, cy - 1);
        RevealCell(cx - 1, cy);
        RevealCell(cx + 1, cy);
        RevealCell(cx - 1, cy + 1);
        RevealCell(cx, cy + 1);
        RevealCell(cx + 1, cy + 1);

        nQueuePos++;
        if (nQueuePos >= 100)
            nQueuePos = 0;
    }
}

// sub_100316B - Toggle between flag/question/normal states
void ToggleFlagState(int x, int y) {
    BYTE* pCell = &g_board[y][x];
    BYTE state = pCell[0] & CELL_STATE_MASK;

    if (state == CELL_FLAGGED)
        state = CELL_QUESTION;      // flag → question
    else if (state == CELL_QUESTION)
        state = CELL_UNREVEALED;    // question → normal (value 0)
    // CELL_EMPTY_FLAG (9) → stays? Actually state 9 → becomes 13 (flag)

    pCell[0] = state | (pCell[0] & 0xE0);
    DrawCell(x, y);
}

// sub_10031A0 - Toggle flag states (used in right-click cycle)
void FlagToQuestion(int x, int y) {
    BYTE* pCell = &g_board[y][x];
    BYTE state = pCell[0] & CELL_STATE_MASK;

    if (state == CELL_EMPTY_FLAG)  // 9 = flagged empty
        state = CELL_FLAGGED;       // → 13 = flagged
    else if (state == CELL_UNREVEALED) // 0 = normal
        state = CELL_QUESTION;       // → 14 = question

    pCell[0] = state | (pCell[0] & 0xE0);
    DrawCell(x, y);
}

// sub_1002EAB - Set cell to specific display state
void SetCellState(int x, int y, BYTE state) {
    g_board[y][x] = state | (g_board[y][x] & 0xE0);
    DrawCell(x, y);
}

// sub_100374F - Right click on cell
void RightClickCell(int x, int y) {
    if (x <= 0 || y <= 0 || x > g_nBoardWidth || y > g_nBoardHeight)
        return;

    BYTE* pCell = &g_board[y][x];
    if (pCell[0] & REVEALED_FLAG)
        return;

    BYTE state = pCell[0] & CELL_STATE_MASK;
    BYTE newState;

    if (state == CELL_QUESTION) {
        newState = (g_bMarks == 0) ? 15 : 13; // 15 if marks off, 13 if on
        UpdateCounter(1);
    } else if (state == CELL_FLAGGED) {
        newState = 14;  // question mark
    } else {
        newState = 13;  // flag
        UpdateCounter(-1);
    }

    SetCellState(x, y, newState);

    // Check win condition
    if ((pCell[0] & CELL_STATE_MASK) == CELL_QUESTION &&
        g_nRevealedCells == g_nSafeCells)
        EndGame(TRUE);
}

// sub_10031D4 - Main cell click handler
void ClickCell(int nOldX, int nOldY) {
    int nX = g_nCurCellX;
    int nY = g_nCurCellY;

    if (nOldX == g_nCurCellX && nOldY == g_nCurCellY)
        return;

    int nPrevX = g_nCurCellX;  // save current
    int nPrevY = g_nCurCellY;
    g_nCurCellX = nOldX;
    g_nCurCellY = nOldY;

    if (g_bBothButtons) {
        // Both buttons - chord mode
        // ... (complex logic for highlighting adjacent cells)
        BOOL bOldValid = (nPrevX > 0 && nPrevY > 0 && nPrevX <= g_nBoardWidth && nPrevY <= g_nBoardHeight);
        BOOL bNewValid = (nOldX > 0 && nOldY > 0 && nOldX <= g_nBoardWidth && nOldY <= g_nBoardHeight);

        if (bOldValid) {
            // Un-highlight old cells
            for (int dy = max(nPrevY-1, 1); dy <= min(nPrevY+1, g_nBoardHeight); dy++)
                for (int dx = max(nPrevX-1, 1); dx <= min(nPrevX+1, g_nBoardWidth); dx++)
                    if (!(g_board[dy][dx] & REVEALED_FLAG))
                        DrawCell(dx, dy);  // restore
        }
        if (bNewValid) {
            // Highlight new cells
            for (int dy = max(nOldY-1, 1); dy <= min(nOldY+1, g_nBoardHeight); dy++)
                for (int dx = max(nOldX-1, 1); dx <= min(nOldX+1, g_nBoardWidth); dx++)
                    if (!(g_board[dy][dx] & REVEALED_FLAG))
                        DrawCell(dx, dy);  // highlight
        }
    } else {
        // Single button - highlight tracking
        BOOL bOldValid = (nPrevX > 0 && nPrevY > 0 && nPrevX <= g_nBoardWidth && nPrevY <= g_nBoardHeight);
        BOOL bNewValid = (nOldX > 0 && nOldY > 0 && nOldX <= g_nBoardWidth && nOldY <= g_nBoardHeight);

        if (bOldValid) {
            if (!(g_board[nPrevY][nPrevX] & REVEALED_FLAG))
                RevealCell(nPrevX, nPrevY);  // restore
            DrawCell(nPrevX, nPrevY);
        }
        if (bNewValid) {
            BYTE cell = g_board[nOldY][nOldX];
            if (!(cell & REVEALED_FLAG) && (cell & CELL_STATE_MASK) != CELL_QUESTION)
                // show pressed state
                DrawCell(nOldX, nOldY);
        }
    }
}

// sub_10035B7 - Chord (both buttons) click
void ChordClick(int x, int y) {
    BOOL bMineTriggered = FALSE;
    BYTE cell = g_board[y][x];

    // Must be revealed and flag count must match cell number
    if (!(cell & REVEALED_FLAG) || (cell & CELL_STATE_MASK) != CountAdjacentFlags(x, y)) {
        ClickCell(-2, -2);
        return;
    }

    // Click all non-flagged adjacent cells
    for (int dy = y - 1; dy <= y + 1; dy++) {
        for (int dx = x - 1; dx <= x + 1; dx++) {
            BYTE adj = g_board[dy][dx];
            if ((adj & CELL_STATE_MASK) == CELL_QUESTION || (adj & MINE_FLAG)) {
                RevealCell(dx, dy);
            } else {
                bMineTriggered = TRUE;
                SetCellState(dx, dy, 76);  // wrong flag state
            }
        }
    }

    if (bMineTriggered)
        EndGame(FALSE);
    else if (g_nRevealedCells == g_nSafeCells)
        EndGame(TRUE);
}

// sub_1003512 - Click on revealed numbered cell
void ClickNumberedCell(int x, int y) {
    BYTE* pCell = &g_board[y][x];

    if (!(pCell[0] & MINE_FLAG)) {
        // Cell has no mine - do flood fill
        RevealCells(x, y);
        if (g_nRevealedCells == g_nSafeCells)
            EndGame(TRUE);
    } else if (g_nRevealedCells > 0) {
        // Mine! First click already happened, this is death
        SetCellState(x, y, 76);
        EndGame(FALSE);
    } else {
        // First click hit a mine - move it
        // Find first non-mine cell
        for (int newY = 1; newY <= g_nBoardHeight; newY++) {
            BYTE* pRow = &g_board[newY][0];
            for (int newX = 1; newX <= g_nBoardWidth; newX++) {
                if (!(pRow[newX] & MINE_FLAG)) {
                    // Swap: remove mine from here, put at (newX, newY)
                    pCell[0] = CELL_UNREVEALED;  // remove mine
                    g_board[newY][newX] |= MINE_FLAG;  // add mine to new cell
                    RevealCells(x, y);
                    return;
                }
            }
        }
    }
}

// sub_10037E1 - Start game (handle first click)
void StartGame(void) {
    int x = g_nCurCellX;
    int y = g_nCurCellY;

    // Validate coordinates
    if (x <= 0 || y <= 0 || x > g_nBoardWidth || y > g_nBoardHeight)
        return;

    // Start timer if this is first click
    if (g_nRevealedCells == 0 && g_nTimer == 0) {
        PlayGameSound(SOUND_TICK);
        g_nTimer++;
        g_bTimerRunning = TRUE;
        SetTimer(g_hWnd, IDT_TIMER, 1000, NULL);
    }

    // If game is over, ignore
    if (!(g_dwGameFlags & 1)) {
        g_nCurCellX = -2;
        g_nCurCellY = -2;
        return;
    }

    if (g_bBothButtons) {
        ChordClick(x, y);
    } else {
        BYTE cell = g_board[y][x];
        if (!(cell & REVEALED_FLAG) && (cell & CELL_STATE_MASK) != CELL_QUESTION)
            ClickNumberedCell(x, y);
    }

    SetFace(g_nFaceState);
}

// sub_100367A - Initialize new game board
void NewGame(void) {
    g_bTimerRunning = FALSE;

    // Set board dimensions from difficulty
    if (g_nDiffWidth == g_nBoardWidth && g_nDiffHeight == g_nBoardHeight)
        SizeWindow(4);  // same size board
    else
        SizeWindow(6);  // different size board

    g_nBoardWidth = g_nDiffWidth;
    g_nBoardHeight = g_nDiffHeight;  // uValue

    ClearBoard();

    g_nTimer = 0;
    g_nMineCount = g_nDiffMines;
    g_nMineDisplay = g_nDiffMines;

    // Place mines randomly
    for (int nMinesPlaced = 0; nMinesPlaced < g_nMineCount; ) {
        int mx = RandomInt(g_nBoardWidth) + 1;
        int my = RandomInt(g_nBoardHeight) + 1;

        if (!(g_board[my][mx] & MINE_FLAG)) {
            g_board[my][mx] ^= 0x85;  // set mine flag
            nMinesPlaced++;
        }
    }

    g_nTimer = 0;
    g_nMineCount = g_nDiffMines;
    g_nMineDisplay = g_nDiffMines;
    g_nRevealedCells = 0;
    g_nSafeCells = g_nBoardWidth * g_nBoardHeight - g_nDiffMines;
    g_dwGameFlags = 1;  // game active

    UpdateCounter(0);
    SizeWindow(0);  // finalize
}

// sub_100347C - End game (win or lose)
void EndGame(BOOL bWon) {
    g_bTimerRunning = FALSE;

    if (bWon) {
        g_nFaceState = FACE_COOL;  // 2+2=4
        SetFace(FACE_COOL);
        PlayGameSound(SOUND_WIN);  // 4*1 + 10 = 14, sound 3

        if (g_nMineDisplay > 0)
            UpdateCounter(-g_nMineDisplay);
    } else {
        g_nFaceState = FACE_DEAD;  // 2+1=3
        SetFace(FACE_DEAD);
        PlayGameSound(0);  // sound for lose
    }

    g_dwGameFlags = 0x10;  // game over

    // Check best times for win
    if (bWon && g_nDifficulty != DIFF_CUSTOM) {
        int* pBestTime = &g_nBestBeginner + g_nDifficulty;  // 4 bytes per int
        if (g_nTimer < *pBestTime) {
            *pBestTime = g_nTimer;
            // Update best time display
            // ShowBestTimes called if time improved
        }
    }
}

// ============================================================================
// Drawing Functions
// ============================================================================

// sub_1002646 - Draw single cell
void DrawCell(int x, int y) {
    HDC hdc = GetDC(g_hWnd);
    int nState = g_board[y][x] & CELL_STATE_MASK;
    BitBlt(hdc, 16 * x - 4, 16 * y + 39, 16, 16,
           g_hdcTiles[nState], 0, 0, SRCCOPY);
    ReleaseDC(g_hWnd, hdc);
}

// sub_10026A7 - Draw all board cells
void DrawAllCells(HDC hdc) {
    for (int y = 1; y <= g_nBoardHeight; y++) {
        for (int x = 1; x <= g_nBoardWidth; x++) {
            int nState = g_boardDisplay[y][x] & CELL_STATE_MASK;
            BitBlt(hdc, 12 + (x-1)*16, 55 + (y-1)*16, 16, 16,
                   g_hdcTiles[nState], 0, 0, SRCCOPY);
        }
    }
}

// sub_1002A22 - Draw 3D outer border
void DrawBorder(HDC hdc) {
    int w = g_nClientWidth - 1;
    int h = g_nClientHeight - 1;

    // Outer border (thickness 3, raised)
    Draw3DRect(hdc, 0, 0, w, h, 3, 1);

    // Inner tray (thickness 3, sunken)
    w -= 9;
    Draw3DRect(hdc, 9, 52, w, h - 9, 3, 0);

    // Top panel area (thickness 2)
    Draw3DRect(hdc, 9, 9, w, 45, 2, 0);

    // Mine counter box (thickness 1, sunken)
    Draw3DRect(hdc, 16, 15, 56, 39, 1, 0);

    // Timer box (thickness 1, sunken)
    int nRight = g_nClientWidth - g_nBorderHeight;
    Draw3DRect(hdc, nRight - 57, 15, nRight - 57 + 40, 39, 1, 0);

    // Face button area (thickness 1, raised)
    int cx = (g_nClientWidth - 24) / 2;
    Draw3DRect(hdc, cx - 1, 15, cx - 1 + 25, 40, 1, 2);
}

// sub_1002785 - Draw mine counter (left side)
void DrawMineCounter(HDC hdc) {
    DWORD dwLayout = GetLayout(hdc);
    if (dwLayout & 1) SetLayout(hdc, 0);

    int nDisplay;
    int nHundreds, nTens;

    if (g_nMineDisplay >= 0) {
        nHundreds = g_nMineDisplay / 100;
        nDisplay = g_nMineDisplay % 100;
    } else {
        nHundreds = 11;  // negative sign
        nDisplay = -g_nMineDisplay % 100;
    }

    nTens = nDisplay / 10;
    DrawDigit(hdc, 17, nHundreds);
    DrawDigit(hdc, 30, nTens);
    DrawDigit(hdc, 43, nDisplay % 10);

    if (dwLayout & 1) SetLayout(hdc, dwLayout);
}

// sub_1002825 - Draw timer (right side)
void DrawTimer(HDC hdc) {
    DWORD dwLayout = GetLayout(hdc);
    if (dwLayout & 1) SetLayout(hdc, 0);

    int nTime = g_nTimer;
    int nRight = g_nClientWidth - g_nBorderHeight;

    DrawDigit(hdc, nRight - 56, nTime / 100);
    DrawDigit(hdc, nRight - 43, (nTime % 100) / 10);
    DrawDigit(hdc, nRight - 30, nTime % 10);

    if (dwLayout & 1) SetLayout(hdc, dwLayout);
}

// sub_10028D9 - Draw face button
void DrawFace(HDC hdc, int nFace) {
    int cx = (g_nClientWidth - 24) / 2;
    SetDIBitsToDevice(hdc, cx, 16, 24, 24, 0, 0, 0, 24,
        (BYTE*)g_pbmFaces + g_nFaceOffsets[nFace], g_pbmFaces, DIB_RGB_COLORS);
}

// sub_1002752 - Draw single LED digit
void DrawDigit(HDC hdc, int x, int nDigit) {
    SetDIBitsToDevice(hdc, x, 16, 13, 23, 0, 0, 0, 23,
        (BYTE*)g_pbmDigits + g_nDigitOffsets[nDigit], g_pbmDigits, DIB_RGB_COLORS);
}

// sub_1002971 - Draw 3D rectangle border
void Draw3DRect(HDC hdc, int x, int y, int w, int h, int nThick, int nStyle) {
    SetPen(nStyle);

    // Top/left highlight (raised) or shadow (sunken)
    if (nThick > 0) {
        for (int i = nThick; i > 0; i--) {
            MoveToEx(hdc, x, --h, NULL);
            LineTo(hdc, x++, y);
            LineTo(hdc, w--, y++);
        }
    }

    // Bottom/right shadow
    if (nStyle < 2)
        SetPen(nStyle ^ 1);  // swap highlight/shadow

    for (int i = nThick; i > 0; i--) {
        MoveToEx(hdc, x--, ++h, NULL);
        LineTo(hdc, ++w, h);
        LineTo(hdc, w, --y);
    }
}

// sub_100293D - Set pen color for 3D effects
void SetPen(int nStyle) {
    // nStyle 0 = shadow (dark), 1 = highlight (light), 2 = raised (light top)
    // Uses SelectObject to set appropriate pen
    // Actual colors mapped via pen color table
}

// sub_1002AC3 - Main paint handler
void PaintBoard(HDC hdc) {
    DrawBorder(hdc);
    DrawMineCounter(hdc);
    DrawFace(hdc, g_nFaceState);
    DrawTimer(hdc);
    DrawAllCells(hdc);
}

// sub_1002913 - Change face button state
void SetFace(int nFace) {
    HDC hdc = GetDC(g_hWnd);
    DrawFace(hdc, nFace);
    ReleaseDC(g_hWnd, hdc);
}

// ============================================================================
// Game Flow Helpers
// ============================================================================

// sub_100346A - Update mine counter display
void UpdateCounter(int nDelta) {
    g_nMineDisplay += nDelta;
    // Redraw mine counter
    HDC hdc = GetDC(g_hWnd);
    DrawMineCounter(hdc);
    ReleaseDC(g_hWnd, hdc);
}

// sub_1001950 - Size and position window
void SizeWindow(BYTE flags) {
    if (!g_hWnd) return;

    g_nMenuOffset = g_nBorderTop;
    if (!(g_nMenuState & 1)) {
        g_nMenuOffset = g_nBorderTop + g_nBorderLeft;
        if (g_hMenu) {
            RECT rcItem, rc;
            if (GetMenuItemRect(g_hWnd, g_hMenu, 0, &rcItem) &&
                GetMenuItemRect(g_hWnd, g_hMenu, 1, &rc) &&
                rcItem.top != rc.top) {
                g_nMenuOffset += g_nBorderLeft;
            }
        }
    }

    g_nClientWidth = 16 * g_nBoardWidth + 24;
    g_nClientHeight = 16 * g_nBoardHeight + 67;

    // Adjust X position if window would be off-screen
    int nScreenW = GetSystemMetrics(SM_CXSCREEN);
    if (g_nWindowX + g_nClientWidth - nScreenW > 0) {
        flags |= 2;
        g_nWindowX -= (g_nWindowX + g_nClientWidth - nScreenW);
    }

    // Adjust Y position
    int nScreenH = GetSystemMetrics(SM_CYSCREEN);
    if (g_nWindowY + g_nClientHeight - nScreenH > 0) {
        flags |= 2;
        g_nWindowY -= (g_nWindowY + g_nClientHeight - nScreenH);
    }

    if (!g_bInit) {
        if (flags & 2)
            MoveWindow(g_hWnd, g_nWindowX, g_nWindowY,
                       g_nClientWidth + g_nBorderHeight,
                       g_nMenuOffset + g_nClientHeight, TRUE);

        if (flags & 4) {
            RECT rc;
            SetRect(&rc, 0, 0, g_nClientWidth, g_nClientHeight);
            InvalidateRect(g_hWnd, &rc, TRUE);
        }
    }
}

// sub_1002FE0 - Timer tick handler
void TimerTick(void) {
    if (!g_bTimerRunning) return;
    if (g_nTimer >= 999) return;

    g_nTimer++;
    // Redraw timer
    HDC hdc = GetDC(g_hWnd);
    DrawTimer(hdc);
    ReleaseDC(g_hWnd, hdc);
    PlayGameSound(SOUND_TICK);
}

// sub_10038ED - Play game sound
void PlayGameSound(int nSound) {
    if (g_bSound != 3) return;  // sound must be enabled

    switch (nSound) {
        case 1: PlaySoundW(MAKEINTRESOURCEW(0x1B0), g_hModule, SND_RESOURCE | SND_ASYNC | SND_NODEFAULT); break;
        case 2: PlaySoundW(MAKEINTRESOURCEW(0x1B1), g_hModule, SND_RESOURCE | SND_ASYNC | SND_NODEFAULT); break;
        case 3: PlaySoundW(MAKEINTRESOURCEW(0x1B2), g_hModule, SND_RESOURCE | SND_ASYNC | SND_NODEFAULT); break;
    }
}

// sub_1003940 - Random number generator
int RandomInt(int nMax) {
    return rand() % nMax;
}

// sub_100140C - Handle face button click (new game)
BOOL FaceButtonClick(LPARAM lParam) {
    int nFaceX = (g_nClientWidth - 24) / 2;
    RECT rcFace = { nFaceX, 16, nFaceX + 24, 40 };
    POINT pt = { LOWORD(lParam), HIWORD(lParam) };

    if (!PtInRect(&rcFace, pt))
        return FALSE;

    SetCapture(g_hWnd);
    SetFace(FACE_CLICKED);  // face state 4 = clicked

    MapWindowPoints(g_hWnd, NULL, (LPPOINT)&rcFace, 2);

    // Wait for mouse release
    MSG msg;
    BOOL bInside = TRUE;
    do {
        while (!PeekMessageW(&msg, g_hWnd, WM_MOUSEMOVE, WM_MBUTTONUP, PM_REMOVE))
            ;

        if (msg.message == WM_MOUSEMOVE) {
            BOOL bNowInside = PtInRect(&rcFace, msg.pt);
            if (bNowInside && !bInside) {
                bInside = TRUE;
                SetFace(FACE_CLICKED);
            } else if (!bNowInside && bInside) {
                bInside = FALSE;
                SetFace(g_nFaceState);
            }
        }
    } while (msg.message != WM_LBUTTONUP);

    if (bInside && PtInRect(&rcFace, msg.pt)) {
        g_nFaceState = FACE_NORMAL;
        SetFace(FACE_NORMAL);
        NewGame();
    }

    ReleaseCapture();
    return TRUE;
}

// sub_1003CE5 - Change difficulty
void ChangeDifficulty(int nDiff) {
    g_nDifficulty = nDiff;
    g_nMenuState = nDiff;

    // Set board params from difficulty table
    g_nDiffHeight = g_difficultyTable[nDiff][0];  // height
    g_nDiffWidth = g_difficultyTable[nDiff][1];   // width
    g_nDiffMines = g_difficultyTable[nDiff][2];   // mines

    // Update menu checkmarks
    CheckMenuItem(g_hMenu, IDM_BEGINNER,
        (nDiff == DIFF_BEGINNER) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(g_hMenu, IDM_INTERMEDIATE,
        (nDiff == DIFF_INTERMEDIATE) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(g_hMenu, IDM_EXPERT,
        (nDiff == DIFF_EXPERT) ? MF_CHECKED : MF_UNCHECKED);

    NewGame();
    g_bRedrawSave = TRUE;
}

// sub_1003950 - Show error message
void ShowError(int nErrCode) {
    // Loads error string by ID and shows MessageBox
    WCHAR szMsg[256];
    LoadStringW(g_hModule, nErrCode, szMsg, 256);
    MessageBoxW(g_hWnd, szMsg, g_szAppName, MB_OK | MB_ICONERROR);
}

// ============================================================================
// Dialog Procedures
// ============================================================================

// DialogFunc (0x10015A6) - Custom game dialog
INT_PTR CALLBACK CustomDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG:  // 0x110
            SetDlgItemInt(hDlg, 141, g_nDiffHeight, FALSE);
            SetDlgItemInt(hDlg, 142, g_nDiffWidth, FALSE);
            SetDlgItemInt(hDlg, 143, g_nDiffMines, FALSE);
            return TRUE;

        case WM_COMMAND:  // 0x111
            switch (LOWORD(wParam)) {
                case IDOK:  // 1
                case IDCANCEL:  // 2
                case 100:  // OK
                case 109:  // Apply?
                    EndDialog(hDlg, 1);
                    return TRUE;
                default:
                    return FALSE;
            }
            // Read values from dialog
            g_nDiffHeight = GetDlgItemInt(hDlg, 141, NULL, FALSE);
            g_nDiffWidth = GetDlgItemInt(hDlg, 142, NULL, FALSE);
            g_nDiffMines = GetDlgItemInt(hDlg, 143, NULL, FALSE);

            // Clamp mines
            int nMaxMines = (g_nDiffHeight - 1) * (g_nDiffWidth - 1);
            if (g_nDiffMines > nMaxMines)
                g_nDiffMines = nMaxMines;

            return TRUE;

        case 0x53:  // Help
            WinHelpW(((LPHELPINFO)lParam)->hItemHandle, L"winmine.hlp", HELP_CONTEXT, 0x1005040);
            break;

        case 0x7B:  // WM_CONTEXTMENU
            WinHelpW((HWND)wParam, L"winmine.hlp", HELP_CONTEXT, 0x1005040);
            break;
    }
    return FALSE;
}

// ============================================================================
// Window Procedure
// ============================================================================

// sub_1001BC9 - Main window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            // Not explicitly handled (handled after CreateWindowEx returns)
            break;

        case WM_DESTROY:
            KillTimer(g_hWnd, IDT_TIMER);
            PostQuitMessage(0);
            break;

        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE)
                g_bCancelFlag = TRUE;
            break;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            PaintBoard(hdc);
            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_WINDOWPOSCHANGED:
            if (!(g_dwGameFlags & 8)) {
                g_nWindowX = ((LPWINDOWPOS)lParam)->x;
                g_nWindowY = ((LPWINDOWPOS)lParam)->y;
            }
            break;

        case WM_KEYDOWN:
            switch (wParam) {
                case VK_SHIFT:
                    if (g_nCheatState >= 5)
                        g_nCheatState ^= 0x14;  // toggle cheat
                    break;
                case 'Y':
                case 'Z':
                case 'X':
                    // xyzzy cheat code check
                    if (g_nCheatState < 5) {
                        // Check against cheat sequence
                        // word_1005034 contains the cheat code values
                    }
                    break;
                case VK_F2:
                    if (g_nRevealedCells > 1) {
                        if (g_bSound == 3) {
                            // Toggle sound off
                            g_bSound = 2;
                        } else {
                            g_bSound = 3;
                        }
                    }
                    break;
                case VK_F3:
                    if (g_nMenuState)
                        ChangeDifficulty(1);  // intermediate
                    break;
                case VK_F4:
                    if (g_nMenuState)
                        ChangeDifficulty(2);  // expert
                    break;
            }
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) >= IDM_BEGINNER && LOWORD(wParam) <= IDM_CUSTOM) {
                // Difficulty selection
                g_nDifficulty = LOWORD(wParam) - IDM_BEGINNER;
                g_nDiffHeight = g_difficultyTable[g_nDifficulty][0];
                g_nDiffWidth = g_difficultyTable[g_nDifficulty][1];
                g_nDiffMines = g_difficultyTable[g_nDifficulty][2];
                NewGame();
                g_bRedrawSave = TRUE;
                ChangeDifficulty(g_nDifficulty);
            } else {
                switch (LOWORD(wParam)) {
                    case IDM_NEW:   // 510 - New game
                        NewGame();
                        break;

                    case IDM_BESTTIMES:  // 528
                        ShowBestTimes();
                        break;

                    case IDM_EXIT:  // 529
                        g_bMarks = !g_bMarks;
                        UpdateMenu();
                        if (LoadBitmaps()) {
                            InvalidateRect(g_hWnd, NULL, TRUE);
                        } else {
                            ShowError(5);  // bitmap load failure
                        }
                        break;

                    case 0x20C:  // 524 - toggle marks
                        NewGame();
                        break;

                    case 0x20E:  // 526 - toggle sound
                        if (g_bSound) {
                            g_bSound = 0;
                        } else {
                            g_bSound = 3;  // enabled
                        }
                        break;

                    case 0x20F:  // 527 - toggle color
                        g_bColor = !g_bColor;
                        break;

                    case 0x24E:  // About
                        ShowBestTimes();  // ? Actually calls BestTimesDialog with param
                        break;

                    case 0x24F:  // ?
                        break;

                    case 0x250:  // ?
                        break;

                    case 0x251:  // Help contents
                        // Launch help
                        return 0;
                }
            }
            break;

        case WM_SYSCOMMAND:
            if ((wParam & 0xFFF0) == SC_MINIMIZE) {
                // Minimize - save window position
                g_dwGameFlags |= 0x0A;
            } else if ((wParam & 0xFFF0) == SC_RESTORE) {
                g_dwGameFlags &= ~0x0A;
                g_bCancelFlag = FALSE;
            }
            break;

        case WM_TIMER:
            TimerTick();
            return 0;

        case WM_LBUTTONDOWN:
            if (g_bCancelFlag) {
                g_bCancelFlag = FALSE;
                return 0;
            }
            if (FaceButtonClick(lParam))
                return 0;
            if (!(g_dwGameFlags & 1))
                break;  // game not active
            g_bBothButtons = (wParam & MK_SHIFT) != 0;  // actually checks MK_LBUTTON & MK_RBUTTON
            SetCapture(hWnd);
            g_nCurCellX = -1;
            g_nCurCellY = -1;
            g_bMouseDown = TRUE;
            SetFace(FACE_CLICKED);
            break;

        case WM_RBUTTONDOWN:
            if (g_bCancelFlag) {
                g_bCancelFlag = FALSE;
                return 0;
            }
            if (!(g_dwGameFlags & 1))
                break;
            if (g_bMouseDown) {
                // Chord: both buttons
                ClickCell(-3, -3);  // special chord handling
                g_bBothButtons = TRUE;
                PostMessageW(g_hWnd, WM_MOUSEMOVE, wParam, lParam);
                return 0;
            }
            if (!(wParam & MK_LBUTTON)) {
                if (!g_bMenuLoop)
                    RightClickCell((LOWORD(lParam) + 4) >> 4, (HIWORD(lParam) - 39) >> 4);
                return 0;
            }
            break;

        case WM_MBUTTONDOWN:
            if (g_bCancelFlag) {
                g_bCancelFlag = FALSE;
                return 0;
            }
            if (!(g_dwGameFlags & 1))
                break;
            g_bBothButtons = TRUE;
            SetCapture(hWnd);
            g_nCurCellX = -1;
            g_nCurCellY = -1;
            g_bMouseDown = TRUE;
            SetFace(FACE_CLICKED);
            break;

        case WM_MOUSEMOVE:
            if (g_bMouseDown) {
                if (g_dwGameFlags & 1) {
                    ClickCell((LOWORD(lParam) + 4) >> 4, (HIWORD(lParam) - 39) >> 4);
                } else {
                    // Game not active
                    g_bMouseDown = FALSE;
                    ReleaseCapture();
                    if (g_dwGameFlags & 1)
                        StartGame();
                    else
                        ClickCell(-2, -2);
                }
            }
            // Cheat code: if game over and cheat active, show/hide mine under cursor
            if (!g_bMenuLoop && g_bMenuLoop != 5) {
                // xyzzy active
                int cx = (LOWORD(lParam) + 4) >> 4;
                int cy = (HIWORD(lParam) - 39) >> 4;
                if (cx > 0 && cy > 0 && cx <= g_nBoardWidth && cy <= g_nBoardHeight) {
                    HDC hdc = GetDC(NULL);
                    BYTE cell = g_board[cy][cx];
                    SetPixel(hdc, 0, 0, (cell & MINE_FLAG) ? 0 : 0xFFFFFF);
                    ReleaseDC(NULL, hdc);
                }
            }
            break;

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
            if (g_bMouseDown) {
                g_bMouseDown = FALSE;
                ReleaseCapture();
                if (g_dwGameFlags & 1)
                    StartGame();
                else
                    ClickCell(-2, -2);
            }
            break;

        case WM_ENTERMENULOOP:
            g_bMenuLoop = TRUE;
            break;

        case WM_EXITMENULOOP:
            g_bMenuLoop = FALSE;
            break;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ============================================================================
// WinMain
// ============================================================================

// sub_1003AB0 - Initialize settings from registry
BOOL InitSettings(void) {
    // Seed random with tick count
    srand(GetTickCount());

    // Read display strings from resources
    LoadStringW(g_hModule, 1, g_szAppName, 32);     // "Minesweeper"
    LoadStringW(g_hModule, 7, g_szPlayerName, 32);   // player name

    // Get system metrics
    g_nBorderTop = GetSystemMetrics(SM_CYCAPTION) + 1;
    g_nBorderLeft = GetSystemMetrics(SM_CYEDGE) + 1;
    g_nBorderWidth = GetSystemMetrics(SM_CXEDGE) + 1;
    g_nBorderHeight = GetSystemMetrics(SM_CYEDGE) + 1;

    // Open registry key
    DWORD dwDisp;
    if (RegCreateKeyExW(HKEY_CURRENT_USER,
            L"Software\\Microsoft\\winmine", 0, NULL, 0,
            KEY_READ | KEY_WRITE, NULL, &g_hKey, &dwDisp))
        return FALSE;

    // Read all settings with defaults
    g_nDiffHeight = ReadRegInt(2, 9, 9, 25);
    g_nDiffWidth = ReadRegInt(3, 9, 9, 30);
    g_nDifficulty = ReadRegInt(0, 0, 0, 3);
    g_nDiffMines = ReadRegInt(1, 10, 10, 999);
    g_nWindowX = ReadRegInt(4, 80, 0, 1024);
    g_nWindowY = ReadRegInt(5, 80, 0, 1024);
    g_bSound = ReadRegInt(6, 0, 0, 3);
    g_bMarks = ReadRegInt(7, 1, 0, 1);
    g_nTickCount = ReadRegInt(9, 0, 0, 1);
    g_nMenuState = ReadRegInt(8, 0, 0, 2);
    g_nBestBeginner = ReadRegInt(11, 999, 0, 999);
    g_nBestIntermediate = ReadRegInt(13, 999, 0, 999);
    g_nBestExpert = ReadRegInt(15, 999, 0, 999);

    ReadRegStr(12, g_szBestBeginner, 32);
    ReadRegStr(14, g_szBestIntermediate, 32);
    ReadRegStr(16, g_szBestExpert, 32);

    // Detect color capability
    HDC hdc = GetDC(GetDesktopWindow());
    int nPlanes = GetDeviceCaps(hdc, PLANES);
    g_bColor = ReadRegInt(10, nPlanes != 2, 0, 1);
    ReleaseDC(GetDesktopWindow(), hdc);

    // Sound state: 3 = enabled
    if (g_bSound == 3)
        g_bSound = 3;  // stays

    RegCloseKey(g_hKey);

    // Save settings back
    // (sub_1002DAB)
    return TRUE;
}

// sub_10021F0 - WinMain
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    g_hModule = hInstance;

    // Initialize settings
    InitSettings();

    // Check if nCmdShow should be overridden
    if (nCmdShow == SW_SHOWMINNOACTIVE || nCmdShow == SW_SHOWMINIMIZED)
        g_bInit = TRUE;  // don't auto-size

    // Init common controls
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = 5885;  // ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES etc.
    InitCommonControlsEx(&icc);

    // Load icon
    g_hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(100));

    // Register window class
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = g_hIcon;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = g_szAppName;  // "Minesweeper"

    if (!RegisterClassW(&wc))
        return 0;

    // Load menu and accelerators
    g_hMenu = LoadMenuW(hInstance, MAKEINTRESOURCEW(500));
    HACCEL hAccel = LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(501));

    // Read registry settings (again - sub_1002BC2)
    // ... (same keys as InitSettings)

    // Create window
    g_hWnd = CreateWindowExW(0, g_szAppName, g_szAppName,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        g_nWindowX - g_nBorderHeight,
        g_nWindowY - g_nMenuOffset,
        g_nBorderHeight + g_nClientWidth,
        g_nMenuOffset + g_nClientHeight,
        NULL, g_hMenu, hInstance, NULL);

    if (!g_hWnd) {
        ShowError(1000);
        return 0;
    }

    SizeWindow(1);

    // Load bitmaps
    if (!LoadBitmaps()) {
        ShowError(5);
        return 0;
    }

    ChangeDifficulty(g_nMenuState);
    NewGame();

    ShowWindow(g_hWnd, SW_SHOW);
    UpdateWindow(g_hWnd);

    g_bInit = FALSE;

    // Message loop
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (!TranslateAcceleratorW(g_hWnd, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    // Cleanup
    if (g_bRedrawSave)
        SaveSettings();  // sub_1002DAB

    return msg.wParam;
}

// ============================================================================
// Entry Point
// ============================================================================

// start (0x1003E21) - CRT entry point
void start(void) {
    // Check if this is a valid PE (MZ + PE header check)
    // Sets up console/no-console flag

    __set_app_type(_crt_gui_app);

    // Initialize CRT
    *_p__fmode() = 0;   // binary mode
    *_p__commode() = 0;
    _adjust_fdiv();
    // Set user math error handler
    __setusermatherr(UserMathErrorFunction);

    // Call C++ static constructors
    _initterm(&__xc_a, &__xc_z);

    // Get command line args
    int argc;
    char** argv;
    char* envp;
    _getmainargs(&argc, &argv, &envp, 0, NULL);

    // Call more constructors
    _initterm(&__xi_a, &__xi_z);

    // Parse command line for quoted args
    char* cmdline = _acmdln;
    // ... skip quotes and leading spaces ...

    // Get startup info
    STARTUPINFOA si;
    si.dwFlags = 0;
    GetStartupInfoA(&si);
    int nShow = (si.dwFlags & STARTF_USESHOWWINDOW) ? si.wShowWindow : SW_SHOWDEFAULT;

    // Call WinMain
    int nRet = WinMain(GetModuleHandleA(NULL), NULL, cmdline, nShow);

    exit(nRet);
}
