
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
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "provBoot"; Description: "Block access to network when Fort Firewall is not running"; Flags: unchecked

[Files]
Source: ".\build\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
; Start menu shortcut
Name: "{group}\{#APP_NAME}"; Filename: "{app}\{#APP_EXE_NAME}"; IconFilename: "{app}\fort.ico"
; Uninstaller shortcut
Name: "{group}\{cm:UninstallProgram,{#APP_NAME}}"; Filename: "{uninstallexe}"; IconFilename: "{app}\fort.ico"

[Run]
Filename: "{app}\{#APP_EXE_NAME}"; Parameters: "-b=1"; Description: "Provider boot"; Flags: runascurrentuser; Tasks: provBoot
Filename: "{app}\driver\scripts\install.bat"; Description: "Install driver"; Flags: runascurrentuser

[UninstallRun]
Filename: "{app}\driver\scripts\uninstall.bat"; Flags: runascurrentuser
Filename: "{app}\{#APP_EXE_NAME}"; Parameters: "-b=0"; Flags: runascurrentuser

[UninstallDelete]
Type: filesandordirs; Name: "{app}"
