
@set WINDBG_PATH="C:\Program Files (x86)\Windows Kits\10\Debuggers\x86"

@set PDB_PATH=%~dp0
@set DUMP_FILE=%1

%WINDBG_PATH%\windbg.exe -y "%PDB_PATH%" -i "%PDB_PATH%" -z "%DUMP_FILE%" -sflags 0x40 -c ".ecxr;k"
