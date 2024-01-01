@echo off
@rem set QT_HOME=D:\Qt\Qt6.6.1\6.6.1\msvc2019_64

cd ..\..\ui

for /r %%f in (i18n\*.ts) do %QT_HOME%\bin\lrelease %%f

del i18n\i18n_en.qm
