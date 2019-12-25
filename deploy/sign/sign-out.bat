@cd %~dp0
@echo off

@set OUT_PATH=..\Output\*.exe

signtool.exe sign /ac "cert\Certum Trusted Network CA.crt" /n "Open Source Developer, Nodir Temirkhodjaev" /fd sha256 /tr http://time.certum.pl/ %OUT_PATH%
