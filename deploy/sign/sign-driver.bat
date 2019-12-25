@cd %~dp0
@echo off

@rem ARCH: 64, 32
@set ARCH=%1

@set DRV_PATH=..\build\driver\fortfw%ARCH%.sys

signtool.exe sign /ac "cert\Certum Trusted Network CA.crt" /n "Open Source Developer, Nodir Temirkhodjaev" /fd sha1 /t http://time.certum.pl/ %DRV_PATH%
signtool.exe sign /as /ac "cert\Certum Trusted Network CA.crt" /n "Open Source Developer, Nodir Temirkhodjaev" /fd sha256 /tr http://time.certum.pl/ %DRV_PATH%
