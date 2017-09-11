@echo off
set QT_BIN_PATH=D:\opt\qt5\qtbase\bin

%QT_BIN_PATH%\windeployqt --release --qmldir ..\src\ui\qml --dir build build\FortFirewall.exe
