@setlocal

@cd %~dp0
@echo off

@set PROC_ARCHS="x86os x64os"
@set FILE_SUFFIX=-windows-x86
@set CHECK_WIN10=
@if not exist ".\build\driver\x86\" (
  @set PROC_ARCHS="x64os"
  @set FILE_SUFFIX=-windows10-x86_64
  @set CHECK_WIN10="Y"
)
@if exist ".\build\driver\ARM64\" (
  @set PROC_ARCHS="arm64"
  @set FILE_SUFFIX=-windows10-arm64
)

@if not defined INNO_HOME @set "INNO_HOME=C:\Program Files\Inno Setup 7"

"%INNO_HOME%\ISCC.exe" FortFirewall.iss /DPROC_ARCHS=%PROC_ARCHS% /DFILE_SUFFIX=%FILE_SUFFIX% /DCHECK_WIN10=%CHECK_WIN10%
