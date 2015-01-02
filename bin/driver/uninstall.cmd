@rem Uninstall driver

@set T=wipf

sc stop %T%
sc delete %T%
