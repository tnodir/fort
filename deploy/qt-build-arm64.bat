@rem Open "Visual Studio .NET Command Prompt for ARM64" to run this script

@setlocal

@cd %~dp0
@echo off

@set QT_PATH=../../../qt
@set TARGET_PATH=build-qt-arm64

md %TARGET_PATH%
cd %TARGET_PATH%

%QT_PATH%/configure -release -force-debug-info -optimize-size -c++std c++20 ^
	-static -unity-build -feature-relocatable -prefix "%TARGET_PATH%\static" ^
	-opensource -confirm-license ^
	%* ^
	-platform win32-arm64-msvc TARGET_CONFIGURE_ARGS=-release ^
	-qt-host-path "%~dp0build-qt\static"
	^
	-nomake examples -nomake tests ^
	^
	-submodules qtbase ^
	^
	-no-feature-columnview -no-feature-commandlinkbutton ^
	-no-feature-concatenatetablesproxymodel ^
	-no-feature-concurrent -no-feature-datawidgetmapper -no-feature-dial ^
	-no-feature-dockwidget -no-feature-filesystemwatcher -no-feature-fontcombobox ^
	-no-feature-fontdialog -no-feature-inputdialog ^
	-no-feature-hijricalendar -no-feature-identityproxymodel ^
	-no-feature-islamiccivilcalendar -no-feature-jalalicalendar ^
	-no-feature-itemmodeltester -no-feature-lcdnumber -no-feature-listwidget ^
	-no-feature-mdiarea -no-feature-movie -no-feature-pdf -no-feature-picture ^
	-no-feature-printsupport -no-feature-raster-64bit ^
	-no-feature-textbrowser -no-feature-textodfwriter ^
	-no-feature-undocommand -no-feature-undogroup -no-feature-undostack -no-feature-undoview ^
	-no-feature-whatsthis -no-feature-wizard ^
	^
	-no-feature-style-android -no-feature-style-mac -no-feature-style-windowsvista ^
	^
	-no-feature-mimetype-database -no-feature-sql ^
	^
	-no-feature-getifaddrs -no-feature-ipv6ifname -no-feature-libproxy ^
	-no-feature-openssl -no-feature-openssl-hash ^
	-schannel -ssl -no-feature-sctp -no-feature-socks5 -no-feature-udpsocket ^
	-no-feature-networkproxy -no-feature-networkdiskcache ^
	-no-feature-dnslookup -no-feature-sspi -no-feature-networklistmanager ^
	^
	-no-opengl -no-feature-opengl -no-feature-dynamicgl -no-feature-directfb ^
	^
	-no-feature-gif -no-feature-jpeg ^
	^
	-no-feature-androiddeployqt -no-feature-dbus -no-feature-macdeployqt ^
	-no-feature-vkgen -no-feature-vulkan -no-feature-windeployqt

cmake --build . --parallel && cmake --install .
