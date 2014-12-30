@rem Install driver as service

@if [%1] == [] exit

@set T=wipf
@set F=wipfdrv%1.sys
@set W=%WINDIR%\System32\Drivers

copy /Y "%~dp0%F%" %W%

sc create %T% binPath="%W%\%F%" type=kernel start=boot DisplayName="Windows IP Filter"
sc start %T%
