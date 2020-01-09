@echo off
set QT_BIN_PATH=D:\Qt\Qt5.13.2\5.13.2\msvc2017_64\bin

cd ..\..\

for /r %%f in (i18n\*.ts) do %QT_BIN_PATH%\lupdate -noobsolete .\ -ts %%f
