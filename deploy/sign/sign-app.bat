@cd %~dp0
@echo off

@set APP_PATH=..\build\FortFirewall.exe

signtool.exe sign /ac "cert\Certum Trusted Network CA.crt" /n "Open Source Developer, Nodir Temirkhodjaev" /fd sha256 /tr http://time.certum.pl/ %APP_PATH%
