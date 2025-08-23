
#define SRC_PATH	AddBackslash(SourcePath) + "..\src"

#include SRC_PATH + "\version\fort_version.h"

#define APP_EXE_NAME	APP_BASE + ".exe"
#define APP_ICO_NAME	APP_BASE + ".ico"
#define APP_SVC_NAME	APP_BASE + "Svc"

#define APP_EXE		StringChange("{app}\%exe%", "%exe%", APP_EXE_NAME)

#define LANG_PATH	AddBackslash(SourcePath) + "languages"
#define LANG_CUSTOM	LANG_PATH + "\custom"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
SetupMutex=Global\{#APP_BASE}Setup
; Don't use AppMutex
RestartApplications=no
AppName={#APP_NAME}
AppVersion={#APP_VERSION_STR}
VersionInfoVersion={#APP_VERSION_STR}
VersionInfoProductTextVersion={#APP_VERSION_STR}{#APP_VERSION_BUILD_STR}
AppVerName={#APP_NAME}
AppCopyright={#APP_LEGALCOPYRIGHT}
AppPublisher={#APP_PUBLISHER}
AppPublisherURL={#APP_URL}
AppSupportURL={#APP_URL}
AppUpdatesURL={#APP_UPDATES_URL}
DefaultGroupName={#APP_NAME}
DefaultDirName={param:PATH|{pf}\{#APP_NAME}}
AlwaysShowDirOnReadyPage=yes
AlwaysShowGroupOnReadyPage=yes
AllowNoIcons=yes
OutputDir=out
OutputBaseFilename={#APP_BASE}-{#APP_VERSION_STR}{#APP_VERSION_BUILD_STR}
Uninstallable=not IsTaskSelected('portable')
UninstallFilesDir={app}\uninst
UninstallDisplayIcon={uninstallexe}
SetupIconFile={#SRC_PATH}\ui_bin\{#APP_ICO_NAME}
ArchitecturesInstallIn64BitMode=x64
ArchitecturesAllowed={#PROC_ARCHS}
Compression=lzma/normal
SolidCompression=yes

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl,{#LANG_CUSTOM}\Default.isl"
Name: "de"; MessagesFile: "compiler:Languages\German.isl,{#LANG_CUSTOM}\German.isl"
Name: "es"; MessagesFile: "compiler:Languages\Spanish.isl,{#LANG_CUSTOM}\Spanish.isl"
Name: "fr"; MessagesFile: "compiler:Languages\French.isl,{#LANG_CUSTOM}\French.isl"
Name: "it"; MessagesFile: "compiler:Languages\Italian.isl,{#LANG_CUSTOM}\Italian.isl"
Name: "ja"; MessagesFile: "compiler:Languages\Japanese.isl,{#LANG_CUSTOM}\Japanese.isl"
Name: "ko"; MessagesFile: "compiler:Languages\Korean.isl,{#LANG_CUSTOM}\Korean.isl"
Name: "pl"; MessagesFile: "compiler:Languages\Polish.isl,{#LANG_CUSTOM}\Polish.isl"
Name: "pt"; MessagesFile: "compiler:Languages\Portuguese.isl,{#LANG_CUSTOM}\Portuguese.isl"
Name: "pt_BR"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl,{#LANG_CUSTOM}\BrazilianPortuguese.isl"
Name: "ru"; MessagesFile: "compiler:Languages\Russian.isl,{#LANG_CUSTOM}\Russian.isl"
Name: "sl"; MessagesFile: "compiler:Languages\Slovenian.isl,{#LANG_CUSTOM}\Slovenian.isl"
Name: "zh_CN"; MessagesFile: "{#LANG_PATH}\ChineseSimplified.isl,{#LANG_CUSTOM}\ChineseSimplified.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"
Name: "explorer"; Description: "{cm:WindowsExplorerIntegration}"
Name: "service"; Description: "{cm:WindowsService}"
Name: "portable"; Description: "{cm:Portable}"; Flags: unchecked

[Files]
Source: "build\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "data\*.ini"; DestDir: "{app}"; Flags: ignoreversion
Source: "data\qt.conf"; DestDir: "{app}"; Flags: ignoreversion
Source: "data\inst.tmp"; DestDir: "{app}"; Flags: ignoreversion deleteafterinstall

[Dirs]
Name: "{app}\Data"; Flags: uninsneveruninstall; Permissions: users-modify; Tasks: portable

[Icons]
; Start menu shortcut
Name: "{group}\{#APP_NAME}"; Filename: "{#APP_EXE}"; WorkingDir: "{app}"; \
  Parameters: "--lang {code:LanguageName}"

; Uninstaller shortcut
Name: "{group}\{cm:UninstallProgram,{#APP_NAME}}"; Filename: "{uninstallexe}"

; Desktop shortcut
Name: "{commondesktop}\{#APP_NAME}"; Filename: "{#APP_EXE}"; WorkingDir: "{app}"; \
  Parameters: "--lang {code:LanguageName}"; Tasks: desktopicon

[Run]
; 1. Uninstall -> 2. Install Driver -> 3. Portable -> 4. Service

Filename: "{#APP_EXE}"; Parameters: "-u explorer"; Flags: runasoriginaluser
Filename: "{#APP_EXE}"; Parameters: "-u most"

Filename: "{app}\driver\scripts\reinstall.bat"; Parameters: {code:DriverInstallArgs}; \
  Description: "Re-install driver"; \
  Check: not IsTaskSelected('portable') or IsTaskSelected('service')

Filename: "{#APP_EXE}"; Parameters: "-i portable"; Tasks: portable
Filename: "{#APP_EXE}"; Parameters: "-i service"; Tasks: service
Filename: "{#APP_EXE}"; Parameters: "-i auto-run"; Tasks: service; Check: (not IsUpgrade)
Filename: "{#APP_EXE}"; Parameters: "-i explorer"; Flags: runasoriginaluser; Tasks: explorer

Filename: "sc.exe"; Parameters: "start {#APP_SVC_NAME}"; Description: "Start service"; \
  Flags: nowait; Tasks: service

Filename: "{#APP_EXE}"; Parameters: "--launch --lang {code:LanguageName}"; \
  Description: {cm:LaunchProgram,{#APP_NAME}}; Flags: nowait postinstall; Check: ShouldLaunch

Filename: "notepad"; Parameters: "{app}\README.portable"; Description: "README.portable"; \
  Flags: nowait postinstall skipifsilent runasoriginaluser; Tasks: portable

[UninstallRun]
Filename: "{#APP_EXE}"; Parameters: "-u"; RunOnceId: "Uninstall"
Filename: "{app}\driver\scripts\uninstall.bat"; RunOnceId: "UninsDriver"

[InstallDelete]
Type: filesandordirs; Name: "{app}\driver"
Type: filesandordirs; Name: "{app}\i18n"
Type: files; Name: "{app}\README*.*"; Check: not IsTaskSelected('portable')
Type: files; Name: "{app}\qt*.*"
; TODO: COMPAT: Remove the following 4 lines after v4.1.0
Type: filesandordirs; Name: "{app}\plugins"
Type: files; Name: "{app}\curl*.*"
Type: files; Name: "{app}\ChangeLog"
Type: files; Name: "{app}\delay-start.bat"

[UninstallDelete]
Type: files; Name: "{app}\restart.bat"

[Registry]
Root: HKLM; Subkey: "System\CurrentControlSet\Services\EventLog\System\fortfw"; Flags: uninsdeletekey
Root: HKLM; Subkey: "System\CurrentControlSet\Services\EventLog\System\fortfw"; \
  ValueType: string; ValueName: "EventMessageFile"; ValueData: "{#APP_EXE}"
Root: HKLM; Subkey: "System\CurrentControlSet\Services\EventLog\System\fortfw"; \
  ValueType: dword; ValueName: "TypesSupported"; ValueData: "7"

Root: HKLM; Subkey: "SOFTWARE\{#APP_NAME}"; Flags: dontcreatekey uninsdeletekeyifempty
Root: HKLM; Subkey: "SOFTWARE\{#APP_NAME}"; ValueName: "passwordHash"; Flags: dontcreatekey uninsdeletevalue

[Code]
var 
  IsUpgradeVar: boolean;

function ParamExists(const Value: string): Boolean;
var
  I: Integer;  
begin
  Result := False;
  for I := 1 to ParamCount do
    if CompareText(ParamStr(I), Value) = 0 then
    begin
      Result := True;
      Exit;
    end;
end;

function DriverInstallArgs(Value: string): string;
begin
  if ParamExists('/VERYSILENT') then
  begin
    Result := '/SILENT';
  end;
end;

function IsSilentInstall: Boolean;
begin
  Result := ParamExists('/SILENT') or ParamExists('/VERYSILENT');
end;

function IsNoCancel: Boolean;
begin
  Result := ParamExists('/NOCANCEL');
end;

function ShouldLaunch: Boolean;
begin
  Result := ParamExists('/LAUNCH') or not IsSilentInstall;
end;

#if CHECK_WIN10 == "Y"
function IsProcessorFeaturePresent(Feature: Integer): Boolean;
external 'IsProcessorFeaturePresent@kernel32.dll stdcall';

function IsSSE4Supported(): Boolean;
begin
  Result := IsProcessorFeaturePresent(38); // PF_SSE4_2_INSTRUCTIONS_AVAILABLE
end;
#endif

function LanguageName(Param: String): String;
begin
  Result := ActiveLanguage;
end;

function GetUninstallString(): String;
var
  UninstallKey: String;
begin
  UninstallKey := ExpandConstant('Software\Microsoft\Windows\CurrentVersion\Uninstall\{#emit SetupSetting("AppName")}_is1');

  if not RegQueryStringValue(HKLM, UninstallKey, 'UninstallString', Result) then
    RegQueryStringValue(HKCU, UninstallKey, 'UninstallString', Result);
end;

procedure SetupIsUpgrade();
begin
  IsUpgradeVar := GetUninstallString() <> '';
end;

function IsUpgrade(): Boolean;
begin
  Result := IsUpgradeVar;
end;

function VCRedist86Exists(): Boolean;
begin
  Result := FileExists(ExpandConstant('{syswow64}\vcruntime140.dll'));
end;

function IsWindows10OrNewer(): Boolean;
var
  Version: TWindowsVersion;
begin
  GetWindowsVersionEx(Version);
  Result := (Version.Major > 10) or ((Version.Major = 10) and (Version.Build >= 17763));
end;

function IsWindows7(): Boolean;
var
  Version: TWindowsVersion;
begin
  GetWindowsVersionEx(Version);
  Result := (Version.Major = 6) and (Version.Minor = 1);
end;

function IsDriverSignatureValid(): Boolean;
var
  DriverPath: String;
  DriverName: String;
  WorkingDir: String;
  Command: String;
  ResultCode: Integer;
begin
  DriverPath := '{app}\driver\x86';
  DriverName := 'fortfw.sys';
  WorkingDir := ExpandConstant('{tmp}\') + DriverPath;

  ExtractTemporaryFiles(DriverPath + '\' + DriverName);

  Command := '-Command "if ((Get-AuthenticodeSignature ' + DriverName
      + ').status -eq ''UnknownError'') {exit 2} else {exit 0}"';

  Exec('powershell.exe', Command, WorkingDir, SW_HIDE, ewWaitUntilTerminated, ResultCode)
  Result := ResultCode = 0;
end;

function HVCIEnabled(): Boolean;
var
  EnabledValue: Cardinal;
begin
  Result := RegQueryDWordValue(HKEY_LOCAL_MACHINE,
      'SYSTEM\CurrentControlSet\Control\DeviceGuard\Scenarios\HypervisorEnforcedCodeIntegrity',
      'Enabled', EnabledValue)
    and (EnabledValue = 1);
end;

function AskPassword(): String;
var
  Form: TSetupForm;
  OKButton, CancelButton: TButton;
  PwdEdit: TPasswordEdit;
begin
  Form := CreateCustomForm();
  try
    Form.ClientWidth := ScaleX(256);
    Form.ClientHeight := ScaleY(100);
    Form.Caption := ExpandConstant('{cm:UninstallPassword}');
    Form.BorderIcons := [biSystemMenu];
    Form.BorderStyle := bsDialog;
    Form.Center;

    OKButton := TButton.Create(Form);
    OKButton.Parent := Form;
    OKButton.Width := ScaleX(75);
    OKButton.Height := ScaleY(23);
    OKButton.Left := Form.ClientWidth - ScaleX(75 + 6 + 75 + 50);
    OKButton.Top := Form.ClientHeight - ScaleY(23 + 10);
    OKButton.Caption := ExpandConstant('{cm:UninstallPasswordOK}');
    OKButton.ModalResult := mrOk;
    OKButton.Default := true;

    CancelButton := TButton.Create(Form);
    CancelButton.Parent := Form;
    CancelButton.Width := ScaleX(75);
    CancelButton.Height := ScaleY(23);
    CancelButton.Left := Form.ClientWidth - ScaleX(75 + 50);
    CancelButton.Top := Form.ClientHeight - ScaleY(23 + 10);
    CancelButton.Caption := ExpandConstant('{cm:UninstallPasswordCancel}');
    CancelButton.ModalResult := mrCancel;
    CancelButton.Cancel := True;

    PwdEdit := TPasswordEdit.Create(Form);
    PwdEdit.Parent := Form;
    PwdEdit.Width := ScaleX(210);
    PwdEdit.Height := ScaleY(23);
    PwdEdit.Left := ScaleX(23);
    PwdEdit.Top := ScaleY(23);

    Form.ActiveControl := PwdEdit;

    if Form.ShowModal() = mrOk then
    begin
      Result := PwdEdit.Text;
    end;
  finally
    Form.Free();
  end;
end;

function CheckPasswordHash(): Boolean;
var
  passwordHash: String;
begin
  { Skip on auto-update }
  if IsSilentInstall() and IsNoCancel() then
  begin
    Result := True;
    Exit;
  end;

  RegQueryStringValue(HKEY_LOCAL_MACHINE, ExpandConstant('SOFTWARE\{#APP_NAME}'),
      'passwordHash', passwordHash);

  if passwordHash = '' then
  begin
    Result := True;
    Exit;
  end;

  if GetSHA1OfString(AskPassword()) <> passwordHash then
  begin
    SuppressibleMsgBox(ExpandConstant('{cm:WrongPassword}'), mbError, MB_OK, IDOK);

    Result := False;
    Exit;
  end;

  Result := True;
end;

function StopFortService(const Restarting: Boolean): Boolean;
var
  path: String;
  params: String;
  ResultCode: Integer;
begin
  path := ExpandConstant('{#APP_EXE}');

  if not FileExists(path) then
  begin
    Result := True;
    Exit;
  end;

  params := '-s';
  if Restarting then
  begin
    params := params + ' restart';
  end;

  Exec(path, params, '', SW_HIDE, ewWaitUntilTerminated, ResultCode)

  Result := (ResultCode = 0);
end;

function Unpack(): Boolean;
var
  path: String;
  params: String;
  ResultCode: Integer;
begin
  path := ExpandConstant('{param:UNPACK}');

  if path = '' then
  begin
    Result := False;
    Exit;
  end;

  path := path + ExpandConstant('\{#APP_BASE}');

  if DirExists(path) then
  begin
    SuppressibleMsgBox('UNPACK Error: Path already exists: ' + path, mbCriticalError, MB_OK, IDOK);

    Result := True;
    Exit;
  end;

  ExtractTemporaryFiles('*.*');

  params := ExpandConstant('"{tmp}') + '\{app}\*" "' + path + '\" /Q /Y /E /H';

  Exec('xcopy.exe', params, GetCurrentDir(), SW_HIDE, ewWaitUntilTerminated, ResultCode)

  Result := True;
end;

function OpenUrl(Url: String): Boolean;
var
  ErrCode: Integer;
begin
  Result := ShellExec('open', Url, '', '', SW_SHOW, ewNoWait, ErrCode);
end;

function InitializeSetup: Boolean;
begin
  if Unpack() then
  begin
    Result := False;
    Exit;
  end;

  if not CheckPasswordHash() then
  begin
    Result := False;
    Exit;
  end;

  SetupIsUpgrade();

  if IsUpgrade() then
  begin
    Result := True;
    Exit;
  end;

#if CHECK_WIN10 == "Y"
  if not IsWindows10OrNewer() then
  begin
    SuppressibleMsgBox(ExpandConstant('{cm:NotCompatibleWithWindows}'), mbCriticalError, MB_OK, IDOK);

    Result := False;
    Exit;
  end;

#if 0
  if not IsSSE4Supported() then
  begin
    SuppressibleMsgBox(ExpandConstant('{cm:NotCompatibleWithSSE4}'), mbCriticalError, MB_OK, IDOK);

    Result := False;
    Exit;
  end;
#endif
#else
  if not VCRedist86Exists() then
  begin
    if SuppressibleMsgBox(ExpandConstant('{cm:InstallVCRedist}'), mbCriticalError, MB_OKCANCEL, IDCANCEL) = IDOK then
      OpenUrl('https://aka.ms/vs/17/release/vc_redist.x86.exe');

    Result := False;
    Exit;
  end;

  if IsWindows7() and not IsDriverSignatureValid() then
  begin
    SuppressibleMsgBox(ExpandConstant('{cm:NotCompatibleWithWindows7}'), mbCriticalError, MB_OK, IDOK);

    OpenUrl('https://www.catalog.update.microsoft.com/Search.aspx?q=KB4474419');

    Result := False;
    Exit;
  end;
#endif

  if HVCIEnabled() then
  begin
    SuppressibleMsgBox(ExpandConstant('{cm:NotCompatibleWithHVCI}'), mbCriticalError, MB_OK, IDOK);

    Result := False;
    Exit;
  end;

  Result := True;
end;

function InitializeUninstall(): Boolean;
begin
  Result := CheckPasswordHash();
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  StopFortService(True);

  Result := '';
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  case CurUninstallStep of
    usPostUninstall:
      begin
        if MsgBox(ExpandConstant('{cm:IsDeleteData}'), mbConfirmation, MB_YESNO or MB_DEFBUTTON2) = IDYES then
          begin
            DelTree(ExpandConstant('{%ProgramData}\{#APP_NAME}'), True, True, True);
            DelTree(ExpandConstant('{localappdata}\{#APP_NAME}'), True, True, True);
          end;
      end;
  end;
end;
