
#include AddBackslash(SourcePath) + "..\src\common\version.h"

#define APP_PUBLISHER	"Nodir Temirkhodjaev"
#define APP_URL		"https://github.com/tnodir"
#define APP_EXE_NAME	"FortFirewall.exe"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppMutex={#APP_NAME}
AppName={#APP_NAME}
AppVersion={#APP_VERSION_STR}
AppVerName={#APP_NAME}
AppCopyright=Copyright (C) 2015 Nodir Temirkhodjaev
AppPublisher={#APP_PUBLISHER}
AppPublisherURL={#APP_URL}
AppSupportURL={#APP_URL}
AppUpdatesURL={#APP_UPDATES_URL}
DefaultGroupName={#APP_NAME}
DefaultDirName={pf32}\{#APP_NAME}
AlwaysShowDirOnReadyPage=yes
AlwaysShowGroupOnReadyPage=yes
AllowNoIcons=yes
OutputBaseFilename=FortFirewall-{#APP_VERSION_STR}
UninstallFilesDir={app}\uninst
ArchitecturesInstallIn64BitMode=x64
Compression=lzma2/ultra
SolidCompression=yes

[Languages]
Name: en; MessagesFile: "compiler:Default.isl"
Name: ru; MessagesFile: "compiler:Languages\Russian.isl"

[Files]
Source: ".\build\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
; Start menu shortcut
Name: "{group}\{#APP_NAME}"; Filename: "{app}\{#APP_EXE_NAME}"; IconFilename: "{app}\FortFirewall.ico"
; Uninstaller shortcut
Name: "{group}\{cm:UninstallProgram,{#APP_NAME}}"; Filename: "{uninstallexe}"; IconFilename: "{app}\FortFirewall.ico"

[Run]
Filename: "{app}\driver\scripts\uninstall.bat"; Description: "Uninstall driver"; Flags: runascurrentuser
Filename: "{app}\driver\scripts\install.bat"; Description: "Install driver"; Flags: runascurrentuser

[UninstallRun]
Filename: "{app}\driver\scripts\uninstall.bat"; Flags: runascurrentuser
Filename: "{app}\{#APP_EXE_NAME}"; Parameters: "-b=0"; Flags: runascurrentuser

[InstallDelete]
Type: files; Name: "{app}\*.dll"
Type: files; Name: "{app}\*.exe"
Type: files; Name: "{app}\driver\*.sys"
Type: filesandordirs; Name: "{app}\i18n"
Type: filesandordirs; Name: "{app}\imports"
Type: filesandordirs; Name: "{app}\plugins"
Type: filesandordirs; Name: "{app}\scripts"

[UninstallDelete]
Type: filesandordirs; Name: "{app}"
