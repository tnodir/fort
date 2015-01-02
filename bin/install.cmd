@rem Install driver as service

@if [%1] == [] exit

@set T=wipf
@set F=wipfdrv%1.sys
@set P=%~dp0%F%

sc create %T% binPath="%P%" type=kernel start=demand DisplayName="Windows IP Filter"
