@echo off
@rem set QT_HOME=D:\Qt\Qt6.9.1\6.9.1\msvc2022_64

cd %~dp0..\..\ui

for %%L in (ar,de,es,fr,it,ja,ko,pl,pt_BR,ru,sl,zh_CN) do (
	%QT_HOME%\bin\lrelease -removeidentical i18n\i18n_%%L.ts i18n\qt\qtbase_%%L.ts -qm i18n\i18n_%%L.qm
)
