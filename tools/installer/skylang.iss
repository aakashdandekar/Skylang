; Skylang Inno Setup Script
; Generates a Windows graphical Setup Wizard installer (skylang-setup.exe)

#define MyAppName "Skylang"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Aakash Dandekar"
#define MyAppURL "https://github.com/skylang-lang/skylang"
#define MyAppExeName "sky.exe"

[Setup]
AppId={{D37E860B-883F-4822-B831-277519E86E20}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={localappdata}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
LicenseFile=..\..\LICENSE
OutputDir=..\..\bin
OutputBaseFilename=skylang-setup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ChangesEnvironment=yes
PrivilegesRequired=lowest

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "..\..\bin\sky.exe"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\..\bin\skylang.exe"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\..\include\*"; DestDir: "{app}\include"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\..\src\*"; DestDir: "{app}\src"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\..\lib\*"; DestDir: "{app}\lib"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\DOC.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion

[Registry]
; Register User Environment PATH
Root: HKCU; Subkey: "Environment"; ValueType: expandsz; ValueName: "Path"; ValueData: "{app}\bin;{olddata}"; Check: NeedsAddPath(ExpandConstant('{app}\bin'))

; Register File Associations (.sky and .skylang)
Root: HKCU; Subkey: "Software\Classes\.sky"; ValueType: string; ValueName: ""; ValueData: "SkylangSourceFile"; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.skylang"; ValueType: string; ValueName: ""; ValueData: "SkylangSourceFile"; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\SkylangSourceFile"; ValueType: string; ValueName: ""; ValueData: "Skylang Source File"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\SkylangSourceFile\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\sky.exe"" ""%1"""
Root: HKCU; Subkey: "Software\Classes\SkylangSourceFile\shell\run\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\sky.exe"" run ""%1"""

[Code]
function NeedsAddPath(Param: string): boolean;
var
  OrigPath: string;
begin
  if not RegQueryStringValue(HKEY_CURRENT_USER, 'Environment', 'Path', OrigPath) then
  begin
    Result := True;
    exit;
  end;
  Result := Pos(';' + UpperCase(Param) + ';', ';' + UpperCase(OrigPath) + ';') = 0;
end;
