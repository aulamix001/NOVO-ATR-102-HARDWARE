#ifndef AppVersion
#define AppVersion "0.1.0"
#endif
[Setup]
AppId={{5D195A4B-64F4-49A2-A163-61A986537640}
AppName=ReelGlue
AppVersion={#AppVersion}
AppPublisher=ReelGlue Audio
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\out
OutputBaseFilename=ReelGlue-{#AppVersion}-Windows-x64
Compression=lzma2
SolidCompression=yes
UninstallFilesDir={commonappdata}\ReelGlue\Uninstall
[Files]
Source: "..\build\ReelGlue_artefacts\Release\VST3\ReelGlue.vst3\*"; DestDir: "{app}\ReelGlue.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
