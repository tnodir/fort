@rem Install driver as a service

@set B=32
@if defined PROGRAMFILES(X86) @set B=64

@set T=fortfw
@set F=%T%%B%.sys
@set P=%~dp0\..\%F%

sc create %T% binPath= "%P%" type= kernel start= auto DisplayName= "Fort Firewall"
sc start %T%
