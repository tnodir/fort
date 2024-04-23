@rem Delayed start of the Fort Firewall client

@cd "%~dp0"
@echo off

@set SVC_NAME=FortFirewallSvc

@for /l %%i in (0,1,9) do (
    timeout /t 1 > NUL
    sc query %SVC_NAME% | find /I "RUNNING" > NUL && goto EXIT
)

:EXIT
start FortFirewall.exe %*
