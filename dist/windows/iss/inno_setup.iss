#include "inno_setup_config.isi"

; ExecAndCaptureOutput() was added in 6.4.
#if VER < EncodeVer(6, 4, 0)
  #error Inno Setup version 6.4 or newer is required
#endif

[Setup]
#if APP_IS_64_BIT
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
#endif

; Windows 7 with Service Pack 1.
MinVersion=6.1sp1

AppName={#APP_NAME}
AppVersion={#APP_VERSION}
AppPublisher={#APP_AUTHOR}
AppPublisherURL={#APP_URL}
AppCopyright=© {#APP_COPYRIGHT_YEAR} {#APP_AUTHOR}
AppSupportURL={#APP_URL}
LicenseFile={#APP_SOURCE_DIR}\LICENSE.txt

; Use the lowest privileges so that the installer shows the per-user
; installation as the recommended method.
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog

OutputDir=.

#if APP_IS_64_BIT
#define OUTPUT_SUFFIX 64
#else
#define OUTPUT_SUFFIX 32
#endif

; We use APP_NAME instead of APP_FILE_NAME to match CPack output.
OutputBaseFilename={#APP_NAME}-{#APP_VERSION}-win{#OUTPUT_SUFFIX}

DefaultDirName={autopf}\{#APP_NAME}
DisableProgramGroupPage=yes
UninstallDisplayIcon={app}\{#APP_FILE_NAME}.exe
SetupIconFile={#APP_SOURCE_DIR}\data\icons\{#APP_FILE_NAME}.ico

ShowLanguageDialog=auto

#define RES_DIR APP_SOURCE_DIR + "\dist\windows\iss"
WizardImageFile={#RES_DIR}\wizard.bmp
WizardSmallImageFile={#RES_DIR}\wizard_small.bmp

Compression=lzma2
SolidCompression=yes

[Files]
Source: "{#APP_FILE_NAME}\*"; \
  DestDir: "{app}"; \
  Flags: ignoreversion recursesubdirs

[Icons]
Name: "{autoprograms}\{#APP_NAME}"; \
  Filename: "{app}\{#APP_FILE_NAME}.exe"

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
#include "inno_setup_languages.isi"

[Code]

procedure OurLog(const Scope, Msg: String);
begin
  Log('[{#APP_FILE_NAME}] ' + Scope + ': ' + Msg);
end;

function OurExecAndGetOutput(
  const LogScope,
  Filename,
  Params: String;
  var Output: String): Boolean;
var
  ExecResultCode: Integer;
  ExecOutput: TExecOutput;
begin
  Result := False;
  Output := '';

  if not ExecAndCaptureOutput(
    Filename,
    Params,
    '',
    SW_HIDE,
    ewWaitUntilTerminated,
    ExecResultCode,
    ExecOutput) then
  begin
    OurLog(LogScope, format(
      'Can''t execute "%s" with params "%s": %s (code %d)'
      , [Filename, Params, SysErrorMessage(ExecResultCode),
      ExecResultCode]));
    exit;
  end;

  if ExecResultCode <> 0 then
  begin
    OurLog(LogScope, format(
      '"%s" with params "%s" exited with a code %d and stderr "%s"'
      , [Filename, Params, ExecResultCode,
      StringJoin(#13#10, ExecOutput.StdErr)]));
    exit;
  end;

  Output := StringJoin(#13#10, ExecOutput.StdOut);
  Result := True;
end;

function OurExec(const LogScope, Filename, Params: String): Boolean;
var
  Output: String;
begin
  Result := OurExecAndGetOutput(LogScope, Filename, Params, Output);
end;

function LocateRootRegKey(
  const RootKey32, RootKey64: Integer;
  const SubKeyName: String;
  var RootKey: Integer): Boolean;
begin
  Result := True;

  if RegKeyExists(RootKey32, SubKeyName) then
    RootKey := RootKey32
  else if IsWin64 and RegKeyExists(RootKey64, SubKeyName) then
    RootKey := RootKey64
  else
    Result := False;
end;

const
  UninstallRegPath =
    'Software\Microsoft\Windows\CurrentVersion\Uninstall\' +
    '{#APP_NAME}_is1';

function IsAutostartEnabled(): Boolean;
var
  LogScope: String;
  RootRegKey: Integer;
  InstallLocation: String;
  InstalledAppPath: String;
  MinAppVersion: Int64;
  InstalledAppVersionStr: String;
  InstalledAppVersion: Int64;
  AutostartParams: String;
  AutostartState: String;
begin
  LogScope := 'IsAutostartEnabled';

  Result := False;

  if IsAdminInstallMode() then
    exit;

  if not LocateRootRegKey(
      HKA32, HKA64, UninstallRegPath, RootRegKey) then
    exit;

  // This is the first version of our application to introduce the
  // command line interface and the "autostart" command in particular.
  // The previous versions simply ignored command line arguments.
  MinAppVersion := PackVersionComponents(1, 5, 0, 0);

  // We can also get the version from MajorVersion/MinorVersion, or
  // extract it from the executable using GetPackedVersion().
  if not RegQueryStringValue(
    RootRegKey,
    UninstallRegPath,
    'DisplayVersion',
    InstalledAppVersionStr) then
  begin
    OurLog(LogScope, format(
      'No DisplayVersion in %s', [UninstallRegPath]));
    exit;
  end;

  if not StrToVersion(
    InstalledAppVersionStr, InstalledAppVersion) then
  begin
    OurLog(LogScope, format(
      'Can''t convert DisplayVersion "%s" to a version number'
      , [InstalledAppVersionStr]));
    exit;
  end;

  if ComparePackedVersion(InstalledAppVersion, MinAppVersion) < 0 then
  begin
    OurLog(LogScope, format(
      'Installed app version %s is less than %s where autostart '
      + 'was added'
      , [VersionToStr(InstalledAppVersion),
      VersionToStr(MinAppVersion)]));
    exit;
  end;

  // We don't want to assume that UsePreviousAppDir is enabled, so we
  // extract the path from the registry instead of using {app}.
  if not RegQueryStringValue(
    RootRegKey,
    UninstallRegPath,
    'InstallLocation',
    InstallLocation) then
  begin
    OurLog(LogScope, format(
      'No InstallLocation in %s', [UninstallRegPath]));
    exit;
  end;

  InstalledAppPath :=
    AddBackslash(InstallLocation) + '{#APP_FILE_NAME}.exe';

  AutostartParams := 'autostart query';
  if not OurExecAndGetOutput(
      LogScope,
      InstalledAppPath,
      AutostartParams,
      AutostartState) then
    exit;

  if AutostartState = 'on' then
    Result := True
  else if AutostartState <> 'off' then
    OurLog(LogScope, format(
      'Unexpected "%s" output of "%s": %s'
      , [AutostartParams, InstalledAppPath, AutostartState]));
end;

// Unlike IsAutostartEnabled(), this function is always called for the
// executable that is being handled by this installer version, so we
// don't need to extract the executable path from the registry or do
// version checks.
procedure SetAutostartIsEnabled(IsEnabled: Boolean);
var
  LogScope: String;
  InstalledAppPath: String;
  AutostartState: String;
begin
  LogScope := 'SetAutostartIsEnabled';

  if IsAdminInstallMode() then
    exit;

  InstalledAppPath := ExpandConstant('{app}/{#APP_FILE_NAME}.exe');

  if IsEnabled then
    AutostartState := 'on'
  else
    AutostartState := 'off';

  if not OurExec(
      LogScope, InstalledAppPath, 'autostart ' + AutostartState) then
    exit;

  OurLog(LogScope, format(
    'Set autostart to "%s" via "%s"'
    , [AutostartState, InstalledAppPath]));
end;

const
  UninstallerMutexName = 'iss_{#APP_FILE_NAME}_uninstaller_mutex';

// Returns false if mutex was not released within the given time.
function WaitForMutex(
  const MutexName: String;
  const MaxWaitTimeMilliseconds: Integer): Boolean;
var
  CheckInterval: Integer;
  RemainingWaitTime: Integer;
begin
  CheckInterval := 200;
  RemainingWaitTime := MaxWaitTimeMilliseconds;

  while True do
  begin
    if not CheckForMutexes(MutexName) then
    begin
      Result := True;
      exit;
    end;

    if RemainingWaitTime <= 0 then
      break;

    if CheckInterval > RemainingWaitTime then
      CheckInterval := RemainingWaitTime;

    Sleep(CheckInterval);

    RemainingWaitTime := RemainingWaitTime - CheckInterval;
  end;

  Result := False;
end;

// Removing the previous version is not actually necessary with Inno
// Setup: it always appends to the uninstall log, so the uninstaller
// will never leave any files, even if the user has installed several
// versions of the app with different file hierarchies. However, this
// also means that until the user uninstalls the app, unnecessary
// files from previous versions will still waste disk space, so let's
// try to uninstall the existing version automatically.
//
// Alternatively, we could use the [InstallDelete] section, but it's
// rather cumbersome to keep a list of files to be removed for all
// previous versions, and there will be no full cleanup if the user
// rolls back to a previous version.
procedure UninstallExisting();
var
  LogScope: String;
  RootRegKey: Integer;
  UninstallString: String;
begin
  LogScope := 'UninstallExisting';

  if not LocateRootRegKey(
      HKA32, HKA64, UninstallRegPath, RootRegKey) then
    exit;

  if not RegQueryStringValue(
      RootRegKey, UninstallRegPath,
      'UninstallString', UninstallString) then
    exit;

  if not OurExec(
      LogScope,
      '>',
      UninstallString
        + ' /VERYSILENT /SUPPRESSMSGBOXES /NORESTART') then
    exit;

  if not WaitForMutex(UninstallerMutexName, 10000) then
    OurLog(LogScope, 'Uninstaller didn''t release the mutex');

  OurLog(LogScope, format(
    'Uninstalled existing via "%s"', [UninstallString]));
end;

var
  ShouldRestoreAutostart: Boolean;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssInstall then
  begin
    ShouldRestoreAutostart := IsAutostartEnabled();
    UninstallExisting();
  end
  else if CurStep = ssPostInstall then
  begin
    if ShouldRestoreAutostart then
      SetAutostartIsEnabled(True);
  end;
end;

function InitializeUninstall(): Boolean;
begin
  CreateMutex(UninstallerMutexName);
  Result := True;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usUninstall then
    SetAutostartIsEnabled(False);
end;
