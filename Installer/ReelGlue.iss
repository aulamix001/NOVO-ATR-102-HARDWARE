#ifndef AppVersion
#define AppVersion "0.1.0"
#endif
[Setup]
AppId={{5D195A4B-64F4-49A2-A163-61A986537640}
AppName=ATR-102
AppVersion={#AppVersion}
AppPublisher=ReelGlue Audio
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\out
OutputBaseFilename=ATR-102-{#AppVersion}-Windows-x64
Compression=lzma2
SolidCompression=yes
UninstallFilesDir={commonappdata}\ATR-102\Uninstall
[Files]
Source: "..\build\ReelGlue_artefacts\Release\VST3\ATR-102.vst3\*"; DestDir: "{app}\ATR-102.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
