@echo off
set QT_BIN_PATH=D:\Qt\Qt5.14.2\5.14.2\msvc2017_64\bin

cd ..\..\

for /r %%f in (i18n\*.ts) do %QT_BIN_PATH%\lupdate -noobsolete -locations none .\ -ts %%f
