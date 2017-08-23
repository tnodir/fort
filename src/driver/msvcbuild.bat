@rem Open "Visual Studio .NET Command Prompt" to run this script

@setlocal

MSBuild fortdrv.vcxproj

@rd /S /Q ..\..\build\out\
