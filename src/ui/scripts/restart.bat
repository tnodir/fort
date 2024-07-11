@rem Restart the UI process after Installer

@echo Waiting for Fort Firewall Installer completion...

@rem Timeout seconds
@set seconds=30

:LOOP

@rem Delay for 1 second
@timeout /t 1 >NUL

@if not exist inst.tmp @goto END

@set /a seconds=%seconds%-1
@if "%seconds%" == "0" @goto END

@goto LOOP


:END
start FortFirewall.exe --launch
