@cd %~dp0
@echo off

@rem ARCH: 64, 32
@set ARCH=%1

@set DRV_PATH=..\build\driver\fortfw%ARCH%.sys

signtool.exe verify /v /kp %DRV_PATH%
