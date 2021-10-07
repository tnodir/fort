@rem Open "Visual Studio .NET Command Prompt" to run this script

@setlocal

@cd %~dp0

@rem PLAT: x64, Win32
@set PLAT=%1
@if "%PLAT%"=="" PLAT=x64

@rem CONFIG: win7, win10
@set CONFIG=%2
@if "%CONFIG%"=="" PLAT=win10

@set OutDir=..\..\build-driver-%CONFIG%\%PLAT%
@set IntDir=%OutDir%-%PLAT%

MSBuild fortdrv.vcxproj /p:OutDir=%OutDir%\;IntDir=%IntDir%\;Platform=%PLAT%;Config=%CONFIG%

@rem DumpBin /SYMBOLS "%IntDir%\fortdrv.obj" > symbols.txt

@rd /S /Q "%IntDir%"

@rd /S /Q "%OutDir%\fortdrv"
@del /Q "%OutDir%\fortfw*.cer"
