# Mine Sweeper — Reverse Engineered

Reverse-engineered reconstruction of the classic **Microsoft Minesweeper** (`WINMINE_1.EXE`), the version bundled with Windows XP.

## What This Is

Decompiled and annotated C source from the original 32-bit Minesweeper binary. Every function (~85 total) has been identified, named, and reconstructed — from `WinMain` and `WndProc` down to the flood-fill engine, cheat code handler, and registry persistence.

## What This Is Not

A port or rewrite. This is a **line-by-line reconstruction** of the original binary's logic, preserving the exact game behavior, board encoding, timing quirks, and even the original `rand()`-based mine placement.

## Structure

| File | Purpose |
|------|---------|
| `WINMINE_1.EXE` | Original binary (reference, not built) |
| `winmine_reconstructed.c` | Human-readable reconstruction with named functions and comments |
| `winmine_compilable.c` | Buildable version that compiles with MSVC and links against the resource file |
| `winmine.rc` | Reconstructed resource script (menus, dialogs, strings, bitmaps, sounds) |
| `resource.h` | Shared resource ID definitions |
| `build.bat` | Build script (MSVC 2022, 32-bit) |
| `res/` | Extracted resources — tile/face/digit bitmaps, WAV sounds, manifest |

## Build

```bat
build.bat
```

Requires Visual Studio 2022 with 32-bit toolchain. Produces `winmine_rebuilt.exe`.

## Key Technical Details

- **Board encoding**: 32-byte-per-row byte array. Bit 7 = mine, bit 6 = revealed, bits 0–4 = display state
- **Flood fill**: 100-entry circular queue (not recursive)
- **First click**: Always safe — mine relocated if hit
- **Cheat code**: `xyzzy` keyboard sequence reveals mines via desktop pixel
- **Registry**: All settings saved to `HKCU\Software\Microsoft\winmine`
- **No C++**: Pure C, ~85 functions, zero classes or vtables

## Why

Educational reverse engineering project. Understanding how a classic Windows game was built at the binary level — no source available, just the EXE and an IDA disassembler.
