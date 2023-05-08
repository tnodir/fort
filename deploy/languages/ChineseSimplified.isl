; *** Inno Setup version 5.5.3+ Chinese Simplified messages ***
;
; To download user-contributed translations of this file, go to:
;   https://jrsoftware.org/files/istrans/
;
; Note: When translating this text, do not add periods (.) to the end of
; messages that didn't have them already, because on those messages Inno
; Setup adds the periods automatically (appending a period would result in
; two periods being displayed).
;
; Maintained by Zhenghan Yang
; Email: 847320916@QQ.com
; Translation based on network resource
; The latest Translation is on https://github.com/kira-96/Inno-Setup-Chinese-Simplified-Translation
;

[LangOptions]
; The following three entries are very important. Be sure to read and 
; understand the '[LangOptions] section' topic in the help file.
LanguageName=<7B80><4F53><4E2D><6587>
; About LanguageID, to reference link:
; https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-lcid/a9eac961-e77d-41a6-90a5-ce1a8b0cdb9c
LanguageID=$0804
; About CodePage, to reference link:
; https://docs.microsoft.com/en-us/windows/win32/intl/code-page-identifiers
LanguageCodePage=936
; If the language you are translating to requires special font faces or
; sizes, uncomment any of the following entries and change them accordingly.
;DialogFontName=
;DialogFontSize=8
;WelcomeFontName=Verdana
;WelcomeFontSize=12
;TitleFontName=Arial
;TitleFontSize=29
;CopyrightFontName=Arial
;CopyrightFontSize=8

[Messages]

; *** Ӧ�ó������
SetupAppTitle=��װ
SetupWindowTitle=��װ - %1
UninstallAppTitle=ж��
UninstallAppFullTitle=%1 ж��

; *** Misc. common
InformationTitle=��Ϣ
ConfirmTitle=ȷ��
ErrorTitle=����

; *** SetupLdr messages
SetupLdrStartupMessage=���ڽ���װ %1������Ҫ������
LdrCannotCreateTemp=�޷�������ʱ�ļ�����װ��������ֹ
LdrCannotExecTemp=�޷�ִ����ʱĿ¼�е��ļ�����װ��������ֹ

; *** ����������Ϣ
LastErrorMessage=%1��%n%n���� %2: %3
SetupFileMissing=��װĿ¼��ȱ���ļ� %1�����������������߻�ȡ������¸�����
SetupFileCorrupt=��װ�ļ����𻵡����ȡ������¸�����
SetupFileCorruptOrWrongVer=��װ�ļ����𻵣������������װ����İ汾�����ݡ����������������ȡ�µĳ��򸱱���
InvalidParameter=��Ч�������в�����%n%n%1
SetupAlreadyRunning=��װ�����������С�
WindowsVersionNotSupported=�������֧�ֵ�ǰ��������е�Windows�汾��
WindowsServicePackRequired=���������Ҫ %1 ����� %2 ����ߡ�
NotOnThisPlatform=������򽫲��������� %1��
OnlyOnThisPlatform=���������������� %1��
OnlyOnTheseArchitectures=�������ֻ����Ϊ���д������ṹ��Ƶ�Windows�汾�н��а�װ��%n%n%1
MissingWOW64APIs=���������е�Windows�汾��������װ����ִ��64λ��װ����Ĺ��ܡ���Ҫ���������⣬�밲װService Pack %1��
WinVersionTooLowError=���������Ҫ %1 �汾 %2 ����ߡ�
WinVersionTooHighError=��������ܰ�װ�� %1 �汾 %2 ����ߡ�
AdminPrivilegesRequired=�ڰ�װ�������ʱ�������Թ���Ա��ݵ�¼��
PowerUserPrivilegesRequired=�ڰ�װ�������ʱ�������Թ���Ա��ݻ���Ȩ�޵��û�����ݵ�¼��
SetupAppRunningError=��װ������ %1 ��ǰ�������С�%n%n���ȹر��������еĳ���Ȼ������ȷ����������������ȡ�����˳���
UninstallAppRunningError=ж�س����� %1 ��ǰ�������С�%n%n���ȹر��������еĳ���Ȼ������ȷ����������������ȡ�����˳���

; *** ��������
ErrorCreatingDir=��װ�����ܴ���Ŀ¼��%1����
ErrorTooManyFilesInDir=�޷���Ŀ¼��%1���д����ļ�����Ϊ�������̫���ļ�

; *** ��װ���򹫹���Ϣ
ExitSetupTitle=�˳���װ����
ExitSetupMessage=��װ����δ��ɰ�װ������������˳������򽫲��ܰ�װ��%n%n�������Ժ������а�װ������ɰ�װ��%n%n�����˳���װ������
AboutSetupMenuItem=���ڰ�װ����(&A)...
AboutSetupTitle=���ڰ�װ����
AboutSetupMessage=%1 �汾 %2%n%3%n%n%1 ��ҳ��%n%4
AboutSetupNote=
TranslatorNote=

; *** ��ť
ButtonBack=< ��һ��(&B)
ButtonNext=��һ��(&N) >
ButtonInstall=��װ(&I)
ButtonOK=ȷ��
ButtonCancel=ȡ��
ButtonYes=��(&Y)
ButtonYesToAll=ȫ��(&A)
ButtonNo=��(&N)
ButtonNoToAll=ȫ��(&O)
ButtonFinish=���(&F)
ButtonBrowse=���(&B)...
ButtonWizardBrowse=���(&R)...
ButtonNewFolder=�½��ļ���(&M)

; *** ��ѡ�����ԡ��Ի�����Ϣ
SelectLanguageTitle=ѡ��װ����
SelectLanguageLabel=ѡ��װʱҪʹ�õ����ԡ�

; *** ����������
ClickNext=�������һ����������������ȡ�����˳���װ����
BeveledLabel=
BrowseDialogTitle=����ļ���
BrowseDialogLabel=��������б���ѡ��һ���ļ��У�Ȼ������ȷ������
NewFolderName=�½��ļ���

; *** ����ӭ����ҳ
WelcomeLabel1=��ӭʹ�� [name] ��װ��
WelcomeLabel2=���ڽ���װ [name/ver] �����ĵ����С�%n%n�������ڼ�����װǰ�ر���������Ӧ�ó���

; *** �����롱��ҳ
WizardPassword=����
PasswordLabel1=�����װ���������뱣����
PasswordLabel3=���������룬Ȼ��������һ�����������������ִ�Сд��
PasswordEditLabel=����(&P)��
IncorrectPassword=������������벻��ȷ�����������롣

; *** �����Э�顱��ҳ
WizardLicense=���Э��
LicenseLabel=���ڼ�����װǰ�Ķ�������Ҫ��Ϣ��
LicenseLabel3=����ϸ�Ķ��������Э�顣�ڼ�����װǰ������ͬ����ЩЭ�����
LicenseAccepted=��ͬ���Э��(&A)
LicenseNotAccepted=�Ҳ�ͬ���Э��(&D)

; *** ����Ϣ����ҳ
WizardInfoBefore=��Ϣ
InfoBeforeLabel=���ڼ�����װǰ�Ķ�������Ҫ��Ϣ��
InfoBeforeClickLabel=׼���ü�����װ�󣬵������һ������
WizardInfoAfter=��Ϣ
InfoAfterLabel=���ڼ�����װǰ�Ķ�������Ҫ��Ϣ��
InfoAfterClickLabel=׼���ü�����װ�󣬵������һ������

; *** ���û���Ϣ����ҳ
WizardUserInfo=�û���Ϣ
UserInfoDesc=������������Ϣ��
UserInfoName=�û���(&U)��
UserInfoOrg=��֯(&O)��
UserInfoSerial=���к�(&S)��
UserInfoNameRequired=�����������û�����

; *** ��ѡ��Ŀ��Ŀ¼����ҳ
WizardSelectDir=ѡ��Ŀ��λ��
SelectDirDesc=���뽫 [name] ��װ�����
SelectDirLabel3=��װ���򽫰�װ [name] �������ļ����С�
SelectDirBrowseLabel=�������һ�����������������ѡ�������ļ��У�������������
DiskSpaceMBLabel=������Ҫ�� [mb] MB �Ŀ��ô��̿ռ䡣
CannotInstallToNetworkDrive=��װ�����޷���װ��һ��������������
CannotInstallToUNCPath=��װ�����޷���װ��һ��UNC·����
InvalidPath=����������һ������������������·�������磺%n%nC:\APP%n%n��������ʽ��UNC·����%n%n\\server\share
InvalidDrive=��ѡ������������ UNC �������ڻ��ܷ��ʡ���ѡ������λ�á�
DiskSpaceWarningTitle=���̿ռ䲻��
DiskSpaceWarning=��װ����������Ҫ %1 KB �Ŀ��ÿռ���ܰ�װ����ѡ��������ֻ�� %2 KB �Ŀ��ÿռ䡣%n%n��һ��Ҫ������
DirNameTooLong=�ļ������ƻ�·��̫����
InvalidDirName=�ļ���������Ч��
BadDirName32=�ļ������Ʋ��ܰ��������κ��ַ���%n%n%1
DirExistsTitle=�ļ����Ѵ���
DirExists=�ļ��У�%n%n%1%n%n�Ѿ����ڡ���һ��Ҫ��װ������ļ�������
DirDoesntExistTitle=�ļ��в�����
DirDoesntExist=�ļ��У�%n%n%1%n%n�����ڡ�����Ҫ�������ļ�����

; *** ��ѡ���������ҳ
WizardSelectComponents=ѡ�����
SelectComponentsDesc=���밲װ��Щ����������
SelectComponentsLabel2=ѡ������Ҫ��װ���������������밲װ�������Ȼ��������һ����������
FullInstallation=��ȫ��װ
; if possible don't translate 'Compact' as 'Minimal' (I mean 'Minimal' in your language)
CompactInstallation=��లװ
CustomInstallation=�Զ��尲װ
NoUninstallWarningTitle=����Ѵ���
NoUninstallWarning=��װ�����⵽��������������ĵ����а�װ��%n%n%1%n%nȡ��ѡ����Щ���������ж�����ǡ�%n%n��һ��Ҫ������
ComponentSize1=%1 KB
ComponentSize2=%1 MB
ComponentsDiskSpaceMBLabel=��ǰѡ������������Ҫ [mb] MB �Ĵ��̿ռ䡣

; *** ��ѡ�񸽼�������ҳ
WizardSelectTasks=ѡ�񸽼�����
SelectTasksDesc=����Ҫ��װ����ִ����Щ��������
SelectTasksLabel2=ѡ������Ҫ��װ�����ڰ�װ [name] ʱִ�еĸ�������Ȼ��������һ������

; *** ��ѡ��ʼ�˵��ļ��С���ҳ
WizardSelectProgramGroup=ѡ��ʼ�˵��ļ���
SelectStartMenuFolderDesc=��װ����Ӧ����������ó���Ŀ�ݷ�ʽ��
SelectStartMenuFolderLabel3=��װ�������ڽ������п�ʼ�˵��ļ����д�������Ŀ�ݷ�ʽ��
SelectStartMenuFolderBrowseLabel=�������һ�����������������ѡ�������ļ��У�������������
MustEnterGroupName=����������һ���ļ�������
GroupNameTooLong=�ļ�������·��̫����
InvalidGroupName=��Ч���ļ������֡�
BadGroupName=�ļ��������ܰ��������κ��ַ���%n%n%1
NoProgramGroupCheck2=��������ʼ�˵��ļ���(&D)

; *** ��׼����װ����ҳ
WizardReady=׼����װ
ReadyLabel1=��װ��������׼����ʼ��װ [name] �����ĵ����С�
ReadyLabel2a=�������װ�������˰�װ�����������Ҫ�ع˻��޸����ã���������һ������
ReadyLabel2b=�������װ�������˰�װ����
ReadyMemoUserInfo=�û���Ϣ��
ReadyMemoDir=Ŀ��λ�ã�
ReadyMemoType=��װ���ͣ�
ReadyMemoComponents=ѡ�������
ReadyMemoGroup=��ʼ�˵��ļ��У�
ReadyMemoTasks=��������

; *** ������׼����װ����ҳ
WizardPreparing=����׼����װ
PreparingDesc=��װ��������׼����װ [name] �����ĵ����С�
PreviousInstallNotCompleted=��ǰ����İ�װ/ж��δ��ɡ�����Ҫ�����������ĵ��Բ�����ɰ�װ��%n%n�������������Ժ������а�װ��� [name] �İ�װ��
CannotContinue=��װ�����ܼ�����������ȡ�����˳���
ApplicationsFound=����Ӧ�ó�������ʹ����Ҫ�ɰ�װ������µ��ļ�������������װ�����Զ��ر���ЩӦ�ó���
ApplicationsFound2=����Ӧ�ó�������ʹ����Ҫ�ɰ�װ������µ��ļ�������������װ�����Զ��ر���ЩӦ�ó��򡣰�װ��ɺ󣬰�װ���򽫳�����������Ӧ�ó���
CloseApplications=�Զ��رո�Ӧ�ó���(&A)
DontCloseApplications=��Ҫ�رո�Ӧ�ó���(&D)
ErrorCloseApplications=��װ�����޷��Զ��ر�����Ӧ�ó��򡣽������ڼ���֮ǰ�ر�����ʹ����Ҫ�ɰ�װ������µ��ļ���Ӧ�ó���

; *** �����ڰ�װ����ҳ
WizardInstalling=���ڰ�װ
InstallingLabel=��װ�������ڰ�װ [name] �����ĵ����У����Եȡ�

; *** ����װ��ɡ���ҳ
FinishedHeadingLabel=[name] ��װ���
FinishedLabelNoIcons=��װ�����������ĵ����а�װ�� [name]��
FinishedLabel=��װ�����������ĵ����а�װ�� [name]����Ӧ�ó������ͨ��ѡ��װ�Ŀ�ݷ�ʽ���С�
ClickFinish=�������ɡ��˳���װ����
FinishedRestartLabel=Ҫ��� [name] �İ�װ����װ������������������ĵ��ԡ�����Ҫ��������������
FinishedRestartMessage=Ҫ��� [name] �İ�װ����װ������������������ĵ��ԡ�%n%n����Ҫ��������������
ShowReadmeCheck=�ǣ�������������ļ�
YesRadio=�ǣ�����������������(&Y)
NoRadio=���Ժ�������������(&N)
; used for example as 'Run MyProg.exe'
RunEntryExec=���� %1
; used for example as 'View Readme.txt'
RunEntryShellExec=���� %1

; *** ����װ������Ҫ��һ�Ŵ��̡���ʾ
ChangeDiskTitle=��װ������Ҫ��һ�Ŵ���
SelectDiskLabel2=�������� %1 �������ȷ������%n%n�����������е��ļ������������ļ���֮����ļ������ҵ�����������ȷ��·���������������
PathLabel=·��(&P)��
FileNotInDir2=�ļ���%1�������ڡ�%2����λ���������ȷ�Ĵ��̻�ѡ�������ļ��С�
SelectDirectoryLabel=��ָ����һ�Ŵ��̵�λ�á�

; *** ��װ״̬��Ϣ
SetupAborted=��װ����δ��ɰ�װ��%n%n������������Ⲣ�������а�װ����
EntryAbortRetryIgnore=��������ԡ���ť���ԣ���������ԡ���������������ֹ��ȡ����װ��

; *** ��װ״̬��Ϣ
StatusClosingApplications=���ڹر�Ӧ�ó���...
StatusCreateDirs=���ڴ���Ŀ¼...
StatusExtractFiles=���ڽ�ѹ���ļ�...
StatusCreateIcons=���ڴ�����ݷ�ʽ...
StatusCreateIniEntries=���ڴ��� INI ��Ŀ...
StatusCreateRegistryEntries=���ڴ���ע�����Ŀ...
StatusRegisterFiles=����ע���ļ�...
StatusSavingUninstall=���ڱ���ж����Ϣ...
StatusRunProgram=������ɰ�װ...
StatusRestartingApplications=��������Ӧ�ó���...
StatusRollback=���ڳ�������...

; *** ��������
ErrorInternal2=�ڲ�����%1
ErrorFunctionFailedNoCode=%1 ʧ��
ErrorFunctionFailed=%1 ʧ�ܣ�������� %2
ErrorFunctionFailedWithMessage=%1 ʧ�ܣ�������� %2.%n%3
ErrorExecutingProgram=����ִ���ļ���%n%1

; *** ע������
ErrorRegOpenKey=��ע�����ʱ����%n%1\%2
ErrorRegCreateKey=����ע�����ʱ����%n%1\%2
ErrorRegWriteKey=д��ע�����ʱ����%n%1\%2

; *** INI ����
ErrorIniEntry=���ļ���%1���д���INI��Ŀʱ����

; *** �ļ����ƴ���
FileAbortRetryIgnore=��������ԡ����ԣ���������ԡ��������ļ������Ƽ�������������ֹ��ȡ����װ��
FileAbortRetryIgnore2=��������ԡ����ԣ���������ԡ����������Ƽ�������������ֹ��ȡ����װ��
SourceIsCorrupted=Դ�ļ�����
SourceDoesntExist=Դ�ļ���%1��������
ExistingFileReadOnly=�����ļ������Ϊֻ����%n%n��������ԡ�ɾ��ֻ�����Բ����ԣ���������ԡ��������ļ�����������ֹ��ȡ����װ��
ErrorReadingExistingDest=���Զ�ȡ�����ļ�ʱ����
FileExists=�ļ��Ѵ��ڡ�%n%n�Ƿ�Ҫ��װ���򸲸�����
ExistingFileNewer=�����ļ��Ȱ�װ�����԰�װ���ļ����¡����������������ļ���%n%n�Ƿ��������ļ���
ErrorChangingAttr=���Ըı��������е��ļ�������ʱ����
ErrorCreatingTemp=������Ŀ��Ŀ¼�����ļ�ʱ����
ErrorReadingSource=���Զ�ȡ����Դ�ļ�ʱ����
ErrorCopying=���Ը��������ļ�ʱ����
ErrorReplacingExistingFile=�����滻���е��ļ�ʱ����
ErrorRestartReplace=���������滻ʧ�ܣ�
ErrorRenamingTemp=����������������Ŀ��Ŀ¼�е�һ���ļ�ʱ����
ErrorRegisterServer=�޷�ע�� DLL/OCX��%1
ErrorRegSvr32Failed=RegSvr32 ʧ�ܣ��˳����� %1
ErrorRegisterTypeLib=�޷�ע�����Ϳ⣺%1

; *** ��װ�����
ErrorOpeningReadme=���Դ������ļ�ʱ����
ErrorRestartingComputer=��װ�����������������ԣ����ֶ�������

; *** ж����Ϣ
UninstallNotFound=�ļ���%1�������ڡ��޷�ж�ء�
UninstallOpenError=�ļ���%1�����ܴ򿪡��޷�ж�ء�
UninstallUnsupportedVer=�˰汾��ж�س����޷�ʶ��ж����־�ļ���%1���ĸ�ʽ���޷�ж��
UninstallUnknownEntry=��ж����־������һ��δ֪����Ŀ (%1)
ConfirmUninstall=��ȷ����Ҫ��ȫɾ�� %1 ���������������
UninstallOnlyOnWin64=�����װ����ֻ����64λWindows�н���ж�ء�
OnlyAdminCanUninstall=�����װ�ĳ�����Ҫ�й���ԱȨ�޵��û�����ж�ء�
UninstallStatusLabel=���ڴ����ĵ�����ɾ�� %1�����Եȡ�
UninstalledAll=%1 ��˳���ش����ĵ�����ɾ����
UninstalledMost=%1 ж����ɡ�%n%n��һЩ�����޷���ɾ�����������ֶ�ɾ�����ǡ�
UninstalledAndNeedsRestart=Ҫ��� %1 ��ж�أ����ĵ��Ա�������������%n%n����������������������
UninstallDataCorrupted=�ļ���%1�����𻵣��޷�ж��

; *** ж��״̬��Ϣ
ConfirmDeleteSharedFileTitle=ɾ�������ļ���
ConfirmDeleteSharedFile2=ϵͳ�а��������й����ļ��Ѿ����ٱ���������ʹ�á�����Ҫж�س���ɾ����Щ�����ļ���%n%n�����Щ�ļ���ɾ���������г�������ʹ����Щ�ļ�����Щ������ܲ�����ȷִ�С����������ȷ����ѡ�񡰷񡱡�����Щ�ļ�������ϵͳ�������������⡣
SharedFileNameLabel=�ļ�����
SharedFileLocationLabel=λ�ã�
WizardUninstalling=ж��״̬
StatusUninstalling=����ж�� %1...

; *** Shutdown block reasons
ShutdownBlockReasonInstallingApp=���ڰ�װ %1��
ShutdownBlockReasonUninstallingApp=����ж�� %1��

; The custom messages below aren't used by Setup itself, but if you make
; use of them in your scripts, you'll want to translate them.

[CustomMessages]

NameAndVersion=%1 �汾 %2
AdditionalIcons=���ӿ�ݷ�ʽ��
CreateDesktopIcon=���������ݷ�ʽ(&D)
CreateQuickLaunchIcon=����������������ݷ�ʽ(&Q)
ProgramOnTheWeb=%1 ��վ
UninstallProgram=ж�� %1
LaunchProgram=���� %1
AssocFileExtension=�� %2 �ļ���չ���� %1 ��������(&A)
AssocingFileExtension=���ڽ� %2 �ļ���չ���� %1 ��������...
AutoStartProgramGroupDescription=�����飺
AutoStartProgram=�Զ����� %1
AddonHostProgramNotFound=%1�޷��ҵ�����ѡ����ļ��С�%n%n����Ҫ������

; *** Fort Firewall messages
WindowsExplorerIntegration=���ɵ� Windows ��Դ������
WindowsService=���Windows����
Portable=��Я
UninstallPassword=������ж������
UninstallPasswordOK=ȷ��
UninstallPasswordCancel=ȡ��
WrongPassword=�������
NotCompatibleWithHVCI=���������HVCI�����ĸ��룩�����ݡ�
IsDeleteData=��ϣ��ɾ�������ļ��������ļ���
