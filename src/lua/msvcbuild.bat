@rem Open "Visual Studio .NET Command Prompt" to run this script

@setlocal
@set LUA=../../../luajit-2.0
@set LSCOMPILE=cl /nologo /c /LD /MD /O2 /W3 /D_CRT_SECURE_NO_DEPRECATE /I%LUA%/src
@set LSLINK=link /nologo

%LSCOMPILE% /DWIN32 /DLUA_BUILD_AS_DLL wipflua.c
@if errorlevel 1 goto :END
%LSLINK% /DLL /OUT:wipflua.dll *.obj %LUA%/src/lua*.lib fwpuclnt.lib kernel32.lib user32.lib uuid.lib
@if errorlevel 1 goto :END

:END

@del *.obj *.manifest *.lib *.exp
@move /Y *.dll ../../bin
