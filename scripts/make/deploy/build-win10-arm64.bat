@rem Make deploy\out\FortFirewall-<version>-windows10-arm64.exe of build-win10-arm64 and the driver, and its GnuPG signature

@call "%~dp0make-installer.bat" win10-arm64 -windows10-arm64
