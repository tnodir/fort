@rem Open "Visual Studio .NET Command Prompt" to run this script

@setlocal

@cd %~dp0

@rem PLAT: x64, Win32
@set PLAT=%1
@if "%PLAT%"=="" PLAT=x64

@rem CONFIG: win7, win10
@set CONFIG=%2
@if "%CONFIG%"=="" PLAT=win10

@set OutDir=..\..\..\build-driver-loader-%CONFIG%\%PLAT%
@set IntDir=%OutDir%-obj

MSBuild fortdl.vcxproj /p:OutDir=%OutDir%\;IntDir=%IntDir%\;Platform=%PLAT%;Config=%CONFIG%

@rem DumpBin /SYMBOLS "%IntDir%\fortdl.obj" > symbols.txt

@rd /S /Q "%IntDir%"

@rd /S /Q "%OutDir%\fortdl"
@del /Q "%OutDir%\fortfw*.cer"
