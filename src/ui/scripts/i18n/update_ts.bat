@echo off
set QT_BIN_PATH=D:\Qt\Qt5.11.2\5.11.2\msvc2017_64\bin

cd ..\..\

%QT_BIN_PATH%\lupdate .\ -ts i18n\i18n_ru.ts i18n\i18n_uz.ts
