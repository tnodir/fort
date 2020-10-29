@rem Open "Visual Studio .NET Command Prompt" to run this script

@setlocal

@cd %~dp0
@echo off

@set TARGET_PATH=build-qt

md %TARGET_PATH%
cd %TARGET_PATH%

../../../qt/configure -release -optimize-size -qmake -opensource -confirm-license -no-opengl ^
	-nomake examples -nomake tests -nomake tools -mp -ltcg ^
	-skip qt3d -skip qtactiveqt -skip qtandroidextras -skip qtcanvas3d ^
	-skip qtcharts -skip qtconnectivity -skip qtdatavis3d ^
	-skip qtdoc -skip qtdeclarative -skip qtdocgallery ^
	-skip qtfeedback -skip qtgamepad -skip qtgraphicaleffects ^
	-skip qtlocation -skip qtmacextras -skip qtnetworkauth ^
	-skip qtmultimedia -skip qtpim -skip qtpurchasing -skip qtqa ^
	-skip qtquickcontrols -skip qtquickcontrols2 ^
	-skip qtremoteobjects -skip qtrepotools ^
	-skip qtscxml -skip qtsensors -skip qtserialbus -skip qtserialport -skip qtspeech ^
	-skip qtsvg -skip qtsystems -skip qttools -skip qtvirtualkeyboard ^
	-skip qtwayland -skip qtwebchannel -skip qtwebengine -skip qtwebglplugin ^
	-skip qtwebsockets -skip qtwebview ^
	-skip qtwinextras -skip qtx11extras -skip qtxmlpatterns ^
	^
	-no-feature-columnview -no-feature-commandlinkbutton ^
	-no-feature-concatenatetablesproxymodel ^
	-no-feature-concurrent -no-feature-cups -no-feature-datawidgetmapper ^
	-no-feature-dial -no-feature-dockwidget ^
	-no-feature-filesystemwatcher -no-feature-fontcombobox ^
	-no-feature-fontdialog -no-feature-hijricalendar -no-feature-identityproxymodel ^
	-no-feature-imageformat_jpeg -no-feature-imageformat_ppm ^
	-no-feature-islamiccivilcalendar -no-feature-jalalicalendar ^
	-no-feature-itemmodeltester -no-feature-lcdnumber -no-feature-listwidget ^
	-no-feature-mdiarea -no-feature-movie -no-feature-pdf -no-feature-picture ^
	-no-feature-printer -no-feature-printpreviewdialog -no-feature-printpreviewwidget ^
	-no-feature-raster-64bit -no-feature-splashscreen ^
	-no-feature-sqlmodel -no-feature-textbrowser -no-feature-textodfwriter ^
	-no-feature-undocommand -no-feature-undogroup -no-feature-undostack -no-feature-undoview ^
	-no-feature-whatsthis -no-feature-wizard ^
	^
	-no-feature-style-android -no-feature-style-mac -no-feature-style-windowsvista ^
	^
	-no-feature-network -no-feature-sql -no-feature-sql-odbc ^
	-no-feature-xml -no-feature-xmlstream ^
	^
	-no-feature-gif -no-feature-jpeg -no-libjpeg -no-feature-tiff -no-feature-webp ^
	-no-feature-dbus -no-feature-vulkan -no-feature-vkgen

nmake
