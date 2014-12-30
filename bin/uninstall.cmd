@rem Uninstall driver

@if [%1] == [] exit

@set T=wipf
@set F=wipfdrv%1.sys
@set W=%WINDIR%\System32\Drivers

sc stop %T%
sc delete %T%

del /F %W%\%F%
