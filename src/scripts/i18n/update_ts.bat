@echo off
@rem set QT_HOME=D:\Qt\Qt6.5.2\6.5.2\msvc2019_64

cd ..\..\ui

for /r %%f in (i18n\*.ts) do %QT_HOME%\bin\lupdate -noobsolete -locations none .\ -ts %%f
