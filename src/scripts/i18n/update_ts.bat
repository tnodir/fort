@echo off
set QT_BIN_PATH=D:\Qt\Qt6.5.0\6.5.0\msvc2019_64\bin

cd ..\..\ui

for /r %%f in (i18n\*.ts) do %QT_BIN_PATH%\lupdate -noobsolete -locations none .\ -ts %%f
