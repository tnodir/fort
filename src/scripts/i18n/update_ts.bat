@echo off
@rem set QT_HOME=D:\Qt\Qt6.9.1\6.9.1\msvc2022_64

cd %~dp0..\..\ui

for /r %%f in (i18n\*.ts) do %QT_HOME%\bin\lupdate -no-obsolete -locations none .\ -ts %%f

sed -i 's/ type="unfinished"//g' i18n/i18n_en.ts
sed -i "s/<translation type=\x22unfinished\x22>\([^<]\)/<translation>\1/" i18n/*.ts
