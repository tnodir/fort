@echo off
@rem set QT_HOME=D:\Qt\Qt6.6.1\6.6.1\msvc2019_64

cd ..\..\ui

for %%L in (ar,de,es,fr,it,ko,pl,pt_BR,ru,sl,zh_CN) do (
	%QT_HOME%\bin\lrelease -removeidentical i18n\i18n_%%L.ts i18n\qt\qtbase_%%L.ts -qm i18n\i18n_%%L.qm
)
