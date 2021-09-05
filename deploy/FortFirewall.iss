
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
; TODO: COMPAT: Remove {#APP_NAME} from AppMutex after v4.1.0
AppMutex={#APP_NAME},{#APP_BASE}
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
OutputDir=out
OutputBaseFilename={#APP_BASE}-{#APP_VERSION_STR}
Uninstallable=not IsTaskSelected('portable')
UninstallFilesDir={app}\uninst
UninstallDisplayIcon={uninstallexe}
SetupIconFile={#SRC_PATH}\ui_bin\{#APP_ICO_NAME}
ArchitecturesInstallIn64BitMode=x64
Compression=lzma2/ultra
SolidCompression=yes

[Languages]
Name: en; MessagesFile: "compiler:Default.isl"
Name: ru; MessagesFile: "compiler:Languages\Russian.isl"
Name: fr; MessagesFile: "compiler:Languages\French.isl"
Name: de; MessagesFile: "compiler:Languages\German.isl"
Name: ko; MessagesFile: "compiler:Languages\Korean.isl"
Name: pt; MessagesFile: "compiler:Languages\Portuguese.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"
Name: "explorer"; Description: "Windows Explorer"
Name: "service"; Description: "Windows Service"
Name: "portable"; Description: "Portable"; Flags: unchecked

[Files]
Source: "build\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#APP_EXE_NAME}.example.ini"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
; Start menu shortcut
Name: "{group}\{#APP_NAME}"; Filename: "{#APP_EXE}"; WorkingDir: "{app}"; Parameters: "--lang {code:LanguageName}"
; Uninstaller shortcut
Name: "{group}\{cm:UninstallProgram,{#APP_NAME}}"; Filename: "{uninstallexe}"
; Desktop shortcut
Name: "{commondesktop}\{#APP_NAME}"; Filename: "{#APP_EXE}"; WorkingDir: "{app}"; \
  Parameters: "--lang {code:LanguageName}"; Tasks: desktopicon

[Run]
; 1. Uninstall -> 2. Install Driver -> 3. Portable -> 4. Service
Filename: "{#APP_EXE}"; Parameters: "-u"
Filename: "{app}\driver\scripts\reinstall.bat"; Description: "Re-install driver"

Filename: "{#APP_EXE}"; Parameters: "-i portable"; Tasks: portable
Filename: "{#APP_EXE}"; Parameters: "-i service"; Tasks: service
Filename: "{#APP_EXE}"; Parameters: "-i explorer"; Tasks: explorer

Filename: "sc.exe"; Parameters: "start {#APP_SVC_NAME}"; Description: "Start service"; \
  Flags: nowait; Tasks: service

Filename: "https://support.microsoft.com/en-us/help/2977003/the-latest-supported-visual-c-downloads"; \
  Description: "Install the latest Visual C++ x86 redistributable!"; Flags: shellexec postinstall; \
  Check: not VCRedist86Exists()

[UninstallRun]
Filename: "{#APP_EXE}"; Parameters: "-u"; RunOnceId: "Uninstall"
Filename: "{app}\driver\scripts\uninstall.bat"; RunOnceId: "DelDriver"

[InstallDelete]
Type: filesandordirs; Name: "{app}\driver"
Type: filesandordirs; Name: "{app}\i18n"
; TODO: COMPAT: Remove the following 4 lines after v4.1.0
Type: filesandordirs; Name: "{app}\plugins"
Type: files; Name: "{app}\curl*.*"
Type: files; Name: "{app}\qt*.*"
Type: files; Name: "{app}\ChangeLog"
Type: files; Name: "{app}\README*.*"

[Code]
function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ResultCode: Integer;
begin
  if Exec('sc.exe', ExpandConstant('stop {#APP_SVC_NAME}'), '', SW_HIDE, ewWaitUntilTerminated, ResultCode) then
    if ResultCode = 0 then Sleep(100); // Let the service to stop
  Result := '';
end;

function LanguageName(Param: String): String;
begin
  Result := ActiveLanguage;
end;

function VCRedist86Exists(): Boolean;
begin
  Result := FileExists(ExpandConstant('{syswow64}\vcruntime140.dll'));
end;
