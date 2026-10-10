@rem Make deploy\out\FortFirewall-<version>-windows-x86.exe of build-win7 and the drivers, and its GnuPG signature

@call "%~dp0make-installer.bat" win7 -windows-x86
