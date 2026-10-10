@rem Make deploy\out\FortFirewall-<version>-windows10-x86_64.exe of build-win10 and the drivers, and its GnuPG signature

@call "%~dp0make-installer.bat" win10 -windows10-x86_64
