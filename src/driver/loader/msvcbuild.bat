@rem Open "Visual Studio .NET Command Prompt" to run this script

@setlocal

@cd %~dp0

@rem PLAT: x64, Win32
@set PLAT=%1
@if "%PLAT%"=="" @set PLAT=x64

@rem CONFIG: win7, win10
@set CONFIG=%2
@if "%CONFIG%"=="" @set CONFIG=win10

@rem ARCH: x86, arm64
@set ARCH=%3
@if "%ARCH%"=="" @set ARCH=x86

@set OutDir=..\..\..\build-driver-loader-%CONFIG%\%PLAT%
@set IntDir=%OutDir%-obj

MSBuild fortdl.vcxproj /p:OutDir=%OutDir%\;IntDir=%IntDir%\;Platform=%PLAT%;Config=%CONFIG%;Arch=%ARCH%

@rem DumpBin /SYMBOLS "%IntDir%\fortdl.obj" > symbols.txt

@rd /S /Q "%IntDir%"

@rd /S /Q "%OutDir%\fortdl"
@del /Q "%OutDir%\fortfw*.cer"
