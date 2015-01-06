@rem Open "Visual Studio .NET Command Prompt" to run this script

@setlocal

MSBuild wipfdrv.vcxproj

@rd /S /Q ..\..\build\out\
