; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "OpenMAMA"
#define MyAppVersion "6.1.0"
#define MyAppPublisher "OpenMAMA.org"
#define MyAppURL "http://openmama.org"
; Set this via command line to "" to use 32 bit
#define arch "x64"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{5DF31254-EC05-4936-BE5B-36F55B2C14FA}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
ChangesEnvironment=yes
DefaultDirName={pf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
LicenseFile=..\LICENSE.md
InfoBeforeFile=.\setup_info_before.txt
InfoAfterFile=.\setup_info_after.txt
OutputDir=.
OutputBaseFilename={#MyAppName}-{#MyAppVersion}-Setup
SetupIconFile=openmama.ico
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode={#arch}
AlwaysShowDirOnReadyPage=yes
DisableDirPage=no
PrivilegesRequired=admin

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Registry]
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType: expandsz; ValueName: "OPENMAMA_ROOT"; ValueData: "{app}\.."
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType: expandsz; ValueName: "Path"; ValueData: "{app};{olddata}"

[Files]
Source: "..\OpenMAMA\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\{cm:ProgramOnTheWeb,{#MyAppName}}"; Filename: "{#MyAppURL}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"

