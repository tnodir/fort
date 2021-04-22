
#define SRC_PATH	AddBackslash(SourcePath) + "..\src"

#include SRC_PATH + "\version\fort_version.h"

#define APP_EXE_NAME	APP_BASE + ".exe"
#define APP_ICO_NAME	APP_BASE + ".ico"
#define APP_SVC_NAME	APP_BASE + "Svc"

#define APP_EXE		StringChange("{app}\%exe%", "%exe%", APP_EXE_NAME)

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
SetupMutex=Global\{#APP_BASE}Setup
; TODO: Remove {#APP_NAME} from AppMutex after v4.1.0
AppMutex={#APP_NAME},{#APP_BASE},Global\{#APP_BASE}
AppName={#APP_NAME}
AppVersion={#APP_VERSION_STR}
VersionInfoVersion={#APP_VERSION_STR}
AppVerName={#APP_NAME}
AppCopyright={#APP_LEGALCOPYRIGHT}
AppPublisher={#APP_PUBLISHER}
AppPublisherURL={#APP_URL}
AppSupportURL={#APP_URL}
AppUpdatesURL={#APP_UPDATES_URL}
DefaultGroupName={#APP_NAME}
DefaultDirName={pf32}\{#APP_NAME}
AlwaysShowDirOnReadyPage=yes
AlwaysShowGroupOnReadyPage=yes
AllowNoIcons=yes
OutputBaseFilename={#APP_BASE}-{#APP_VERSION_STR}
Uninstallable=not IsTaskSelected('portable')
UninstallFilesDir={app}\uninst
UninstallDisplayIcon={uninstallexe}
SetupIconFile={#SRC_PATH}\ui\{#APP_ICO_NAME}
ArchitecturesInstallIn64BitMode=x64
Compression=lzma2/ultra
SolidCompression=yes

[Languages]
Name: en; MessagesFile: "compiler:Default.isl"
Name: ru; MessagesFile: "compiler:Languages\Russian.isl"
Name: fr; MessagesFile: "compiler:Languages\French.isl"
Name: ko; MessagesFile: "compiler:Languages\Korean.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; Flags: unchecked
Name: "explorer"; Description: "File Explorer integration"; Flags: unchecked
Name: "portable"; Description: "Portable"; Flags: unchecked

[Files]
Source: "build\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#APP_EXE_NAME}.example.ini"; DestDir: "{app}"
Source: "README.portable"; DestDir: "{app}"; Tasks: portable

[Registry]
; Explorer's Context Menu
#define REG_SHELL	"SystemFileAssociations\.exe\Shell"
#define REG_SHELL_MENU	REG_SHELL + "\Fort Firewall"
Root: HKCR; Subkey: "{#REG_SHELL}"; Flags: uninsdeletekeyifempty; Tasks: explorer
Root: HKCR; Subkey: "{#REG_SHELL_MENU}"; Flags: deletekey uninsdeletekey
Root: HKCR; Subkey: "{#REG_SHELL_MENU}"; ValueType: string; ValueName: "icon"; ValueData: "{#APP_EXE}"; Tasks: explorer
Root: HKCR; Subkey: "{#REG_SHELL_MENU}"; ValueType: string; ValueName: "MUIVerb"; ValueData: "Fort Firewall ..."; Tasks: explorer
Root: HKCR; Subkey: "{#REG_SHELL_MENU}\command"; ValueType: string; ValueData: """{#APP_EXE}"" -w -c prog add ""%1"""; Tasks: explorer

[Icons]
; Start menu shortcut
Name: "{group}\{#APP_NAME}"; Filename: "{#APP_EXE}"; WorkingDir: "{app}"; Parameters: "--lang {code:LanguageName}"
; Uninstaller shortcut
Name: "{group}\{cm:UninstallProgram,{#APP_NAME}}"; Filename: "{uninstallexe}"
; Desktop shortcut
Name: "{commondesktop}\{#APP_NAME}"; Filename: "{#APP_EXE}"; WorkingDir: "{app}"; \
  Parameters: "--lang {code:LanguageName}"; Tasks: desktopicon

[Run]
Filename: "{app}\driver\scripts\reinstall.bat"; Description: "Re-install driver"; Flags: runascurrentuser
Filename: "sc.exe"; Parameters: "start {#APP_SVC_NAME}"; Description: "Start service"; Flags: runascurrentuser nowait
Filename: "https://support.microsoft.com/en-us/help/2977003/the-latest-supported-visual-c-downloads"; \
  Description: "Visual C++ x86 redistributable"; Flags: shellexec postinstall

[UninstallRun]
Filename: "{#APP_EXE}"; Parameters: "-u"; RunOnceId: "DelProvider"; Flags: runascurrentuser
Filename: "{app}\driver\scripts\uninstall.bat"; RunOnceId: "DelDriver"; Flags: runascurrentuser

[InstallDelete]
Type: filesandordirs; Name: "{app}\driver"
Type: filesandordirs; Name: "{app}\i18n"
Type: filesandordirs; Name: "{app}\plugins"
Type: files; Name: "{app}\curl*.*"
Type: files; Name: "{app}\qt*.*"
Type: files; Name: "{app}\README*.*"

[Code]
function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ResultCode: Integer;
begin
  Exec('sc.exe', ExpandConstant('stop {#APP_SVC_NAME}'), '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Result := '';
end;

function LanguageName(Param: String): String;
begin
  Result := ActiveLanguage;
end;
