@rem Restart the UI process after Installer

@rem Timeout seconds
set timeout=30

:LOOP

@rem Delay for 1 second
ping -n 2 127.0.0.1 >NUL

@if not exist inst.tmp @goto END

@set /a timeout=%timeout%-1
@if "%timeout%" == "0" @goto END

@goto LOOP


:END
start FortFirewall.exe --launch
