@setlocal

@cd %~dp0
@echo off

@set PROC_ARCHS="x86 x64"
@set CHECK_WIN10=
@if not exist ".\build\driver\x86\" (
  @set PROC_ARCHS="x64"
  @set CHECK_WIN10="Y"
)
@if exist ".\build\driver\ARM64\" (
  @set PROC_ARCHS="x64 arm64"
)

@set INNO_PATH=D:\Utils\Dev\InnoSetup5\ISCC.exe

"%INNO_PATH%" FortFirewall.iss /DPROC_ARCHS=%PROC_ARCHS% /DCHECK_WIN10=%CHECK_WIN10%
