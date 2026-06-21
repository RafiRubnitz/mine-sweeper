@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat" > nul 2>&1
cd /d "C:\Users\Rubnitz\Desktop\claude projects\reversing\minesweeper version3"
cl /nologo /W3 /O2 /DUNICODE /D_UNICODE /c winmine_compilable.c 2>&1
if %ERRORLEVEL% neq 0 (echo CL FAILED & exit /b 1)
echo CL OK
link /nologo /SUBSYSTEM:WINDOWS /OUT:winmine_rebuilt.exe winmine_compilable.obj winmine.res user32.lib gdi32.lib advapi32.lib shell32.lib winmm.lib comctl32.lib 2>&1
if %ERRORLEVEL% neq 0 (echo LINK FAILED & exit /b 1)
echo LINK OK
