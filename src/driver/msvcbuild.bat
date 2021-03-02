@rem Open "Visual Studio .NET Command Prompt" to run this script

@setlocal

@cd %~dp0

@rem PLAT: x64, Win32
@set PLAT=%1

@set OutDir=..\..\deploy\build\driver
@set IntDir=.\build-%PLAT%

MSBuild fortdrv.vcxproj /p:OutDir=%OutDir%\;IntDir=%IntDir%\;Platform=%PLAT%

@rem DumpBin /SYMBOLS %IntDir%\fortdrv.obj > symbols.txt

@rd /S /Q %IntDir%

@rd /S /Q %OutDir%\fortdrv
@del /Q %OutDir%\fortfw*.cer %OutDir%\fortfw*.pdb
