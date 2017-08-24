@rem Uninstall driver

@set T=fortfw

sc stop %T%
sc delete %T%
