@rem The machine-specific paths of the build scripts: call it after setlocal

@for %%I in ("%~dp0..\..\..") do @set "FORT_ROOT=%%~fI"

@rem The secret key of the driver payloads (DriverPayload.exe --secret)
@set "FORT_RSA_KEY=%FORT_ROOT%\build-tmp\fort.rsa"

@rem EWDKs: Old (WDK 10.0.20348, VS 2019) for win7, New (the latest) for win10
@set "EWDK_OLD=E:\EWDK\Old"
@set "EWDK_NEW=E:\EWDK\New"

@rem VS 2026: vcvars*.bat
@set "VCVARS_DIR=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build"

@rem Static Qts: win7 (x86, Qt 6.1), win10 (x64; also the host tools for arm64), win10-arm64
@set "QT_STATIC_WIN7=D:\opt\qt-fort\static"
@set "QT_STATIC_WIN10=D:\opt\qt-fort-lib\static"
@set "QT_STATIC_ARM64=D:\opt\qt-fort-arm\static"

@rem jom for the parallel builds (nmake with cl's /MP if it's missing)
@set "JOM_PATH=D:\Qt\qtcreator-21.0.0-beta2\bin\jom\jom.exe"
