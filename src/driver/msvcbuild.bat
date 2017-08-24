@rem Open "Visual Studio .NET Command Prompt" to run this script

@setlocal

@set PLAT=%1

@set OutDir=.\
@set IntDir=.\build\

MSBuild fortdrv.vcxproj /p:OutDir=%OutDir%;IntDir=%IntDir%;Platform=%PLAT%

@rd /S /Q %IntDir%
