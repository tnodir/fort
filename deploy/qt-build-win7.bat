@rem Open "Visual Studio .NET Command Prompt" to run this script

@setlocal

@cd %~dp0
@echo off

@set QT_PATH=../../../qt
@set TARGET_PATH=build-qt-win7

md %TARGET_PATH%
cd %TARGET_PATH%

%QT_PATH%/configure -release -force-debug-info -optimize-size ^
	-static -feature-relocatable -prefix "%TARGET_PATH%\static" ^
	-opensource -confirm-license -mp -no-opengl ^
	%* ^
	-nomake examples -nomake tests ^
	^
	-skip qt3d -skip qt5compat -skip qtactiveqt -skip qtandroidextras ^
	-skip qtcanvas3d -skip qtcharts -skip qtconnectivity -skip qtdatavis3d ^
	-skip qtdeclarative -skip qtdoc -skip qtfeedback ^
	-skip qtgamepad -skip qtgraphicaleffects -skip qtgrpc -skip qtimageformats ^
	-skip qtlocation -skip qtlottie -skip qtmacextras ^
	-skip qtmultimedia -skip qtnetworkauth -skip qtpim -skip qtpurchasing ^
	-skip qtqa -skip qtquick3d -skip qtquickcontrols -skip qtquickcontrols2 ^
	-skip qtquicktimeline -skip qtremoteobjects -skip qtrepotools -skip qtscxml ^
	-skip qtsensors -skip qtserialbus -skip qtserialport -skip qtshadertools ^
	-skip qtspeech -skip qtsvg -skip qtsystems -skip qttools -skip qttranslations ^
	-skip qtvirtualkeyboard -skip qtwayland -skip qtwebchannel -skip qtwebengine ^
	-skip qtwebglplugin -skip qtwebsockets -skip qtwebview ^
	-skip qtwinextras -skip qtx11extras -skip qtxmlpatterns ^
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
	-no-feature-printer -no-feature-raster-64bit ^
	-no-feature-sqlmodel -no-feature-textbrowser -no-feature-textodfwriter ^
	-no-feature-undocommand -no-feature-undogroup -no-feature-undostack -no-feature-undoview ^
	-no-feature-whatsthis -no-feature-wizard ^
	^
	-no-feature-style-android -no-feature-style-mac -no-feature-style-windowsvista ^
	^
	-no-feature-mimetype-database -no-feature-sql -no-feature-sql-odbc ^
	^
	-no-feature-getifaddrs -no-feature-ipv6ifname -no-feature-libproxy ^
	-no-feature-openssl -schannel -ssl -no-feature-sctp -no-feature-udpsocket ^
	-no-feature-networkproxy -no-feature-socks5 -no-feature-networkdiskcache ^
	-no-feature-dnslookup -no-feature-sspi -no-feature-networklistmanager ^
	^
	-no-feature-gif -no-feature-jpeg ^
	-no-feature-dbus -no-feature-vulkan -no-feature-vkgen

cmake --build . --parallel && cmake --install .
