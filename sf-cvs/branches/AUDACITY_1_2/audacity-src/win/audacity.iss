; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
AppName=Audacity
AppVerName=Audacity 1.2.0
AppPublisherURL=http://audacity.sourceforge.net
AppSupportURL=http://audacity.sourceforge.net
AppUpdatesURL=http://audacity.sourceforge.net
ChangesAssociations=yes
DefaultDirName={pf}\Audacity
DefaultGroupName=Audacity
; AlwaysCreateUninstallIcon=yes
LicenseFile=E:\sfw_dev\audacity\LICENSE.txt
InfoBeforeFile=E:\sfw_dev\audacity\README.txt
; uncomment the following line if you want your installation to run on NT 3.51 too.
; MinVersion=4,3.51

[Tasks]
Name: desktopicon; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"; MinVersion: 4,4
Name: associate_aup; Description: "&Associate Audacity project files"; GroupDescription: "Other tasks:"; Flags: checkedonce; MinVersion: 4,4


[Files]
Source: "E:\sfw_dev\audacity\win\Release\audacity.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\audacity-1.2-help.htb"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\bg\audacity.mo"; DestDir: "{app}\Languages\bg"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\ca\audacity.mo"; DestDir: "{app}\Languages\ca"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\cs\audacity.mo"; DestDir: "{app}\Languages\cs"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\cs\wxstd.mo"; DestDir: "{app}\Languages\cs"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\da\audacity.mo"; DestDir: "{app}\Languages\da"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\da\wxstd.mo"; DestDir: "{app}\Languages\da"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\de\audacity.mo"; DestDir: "{app}\Languages\de"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\de\wxstd.mo"; DestDir: "{app}\Languages\de"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\es\audacity.mo"; DestDir: "{app}\Languages\es"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\es\wxstd.mo"; DestDir: "{app}\Languages\es"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\fr\audacity.mo"; DestDir: "{app}\Languages\fr"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\fr\wxstd.mo"; DestDir: "{app}\Languages\fr"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\hu\audacity.mo"; DestDir: "{app}\Languages\hu"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\hu\wxstd.mo"; DestDir: "{app}\Languages\hu"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\it\audacity.mo"; DestDir: "{app}\Languages\it"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\it\wxstd.mo"; DestDir: "{app}\Languages\it"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\ja\audacity.mo"; DestDir: "{app}\Languages\ja"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\mk\audacity.mo"; DestDir: "{app}\Languages\mk"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\nl\audacity.mo"; DestDir: "{app}\Languages\nl"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\nl\wxstd.mo"; DestDir: "{app}\Languages\nl"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\pl\audacity.mo"; DestDir: "{app}\Languages\pl"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\pl\wxstd.mo"; DestDir: "{app}\Languages\pl"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\pt\audacity.mo"; DestDir: "{app}\Languages\pt"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\ru\audacity.mo"; DestDir: "{app}\Languages\ru"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\ru\wxstd.mo"; DestDir: "{app}\Languages\ru"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\sl\audacity.mo"; DestDir: "{app}\Languages\sl"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\sl\wxstd.mo"; DestDir: "{app}\Languages\sl"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Languages\sv\audacity.mo"; DestDir: "{app}\Languages\sv"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\README.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Nyquist\bug.lsp"; DestDir: "{app}\Nyquist"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Nyquist\dspprims.lsp"; DestDir: "{app}\Nyquist"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Nyquist\evalenv.lsp"; DestDir: "{app}\Nyquist"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Nyquist\follow.lsp"; DestDir: "{app}\Nyquist"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Nyquist\init.lsp"; DestDir: "{app}\Nyquist"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Nyquist\misc.lsp"; DestDir: "{app}\Nyquist"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Nyquist\nyinit.lsp"; DestDir: "{app}\Nyquist"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Nyquist\nyqmisc.lsp"; DestDir: "{app}\Nyquist"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Nyquist\nyquist.lsp"; DestDir: "{app}\Nyquist"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Nyquist\printrec.lsp"; DestDir: "{app}\Nyquist"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Nyquist\profile.lsp"; DestDir: "{app}\Nyquist"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Nyquist\seq.lsp"; DestDir: "{app}\Nyquist"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Nyquist\seqfnint.lsp"; DestDir: "{app}\Nyquist"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Nyquist\seqmidi.lsp"; DestDir: "{app}\Nyquist"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Nyquist\sndfnint.lsp"; DestDir: "{app}\Nyquist"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Nyquist\system.lsp"; DestDir: "{app}\Nyquist"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Nyquist\test.lsp"; DestDir: "{app}\Nyquist"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Nyquist\xlinit.lsp"; DestDir: "{app}\Nyquist"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Plug-Ins\analyze.ny"; DestDir: "{app}\Plug-Ins"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Plug-Ins\beat.ny"; DestDir: "{app}\Plug-Ins"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Plug-Ins\clicktrack.ny"; DestDir: "{app}\Plug-Ins"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Plug-Ins\delay.ny"; DestDir: "{app}\Plug-Ins"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Plug-Ins\fadein.ny"; DestDir: "{app}\Plug-Ins"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Plug-Ins\fadeout.ny"; DestDir: "{app}\Plug-Ins"; Flags: ignoreversion

; Freeverb2 requires VST Enabler.
Source: "E:\sfw_dev\audacity\win\Release\Plug-Ins\Freeverb2.dll"; DestDir: "{app}\Plug-Ins"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Plug-Ins\Freeverb-readme.txt"; DestDir: "{app}\Plug-Ins"; Flags: ignoreversion

; GVerb requires VST Enabler.
Source: "E:\sfw_dev\audacity\win\Release\Plug-Ins\GVerb.dll"; DestDir: "{app}\Plug-Ins"; Flags: ignoreversion

Source: "E:\sfw_dev\audacity\win\Release\Plug-Ins\highpass.ny"; DestDir: "{app}\Plug-Ins"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Plug-Ins\highpass.ny"; DestDir: "{app}\Plug-Ins"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Plug-Ins\lowpass.ny"; DestDir: "{app}\Plug-Ins"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Plug-Ins\pluck.ny"; DestDir: "{app}\Plug-Ins"; Flags: ignoreversion

; sc4 requires VST Enabler.
Source: "E:\sfw_dev\audacity\win\Release\Plug-Ins\sc4.dll"; DestDir: "{app}\Plug-Ins"; Flags: ignoreversion

Source: "E:\sfw_dev\audacity\win\Release\Plug-Ins\tremolo.ny"; DestDir: "{app}\Plug-Ins"; Flags: ignoreversion
Source: "E:\sfw_dev\audacity\win\Release\Plug-Ins\undcbias.ny"; DestDir: "{app}\Plug-Ins"; Flags: ignoreversion

Source: "E:\sfw_dev\audacity\win\Release\Plug-Ins\VST Enabler.dll"; DestDir: "{app}\Plug-Ins"; Flags: ignoreversion


[Icons]
Name: "{commonprograms}\Audacity"; Filename: "{app}\audacity.exe"
Name: "{userdesktop}\Audacity"; Filename: "{app}\audacity.exe"; MinVersion: 4,4; Tasks: desktopicon

[InstallDelete]
; Get rid of Audacity 1.0.0 stuff that's no longer used.
Type: files; Name: "{app}\audacity-help.htb"
; Not sure we want to do this because user may have stored their own.
;   Type: filesandordirs; Name: "{app}\vst"

; We've switched from a folder in the start menu to just the Audacity.exe at the top level.
; Get rid of 1.0.0 folder and its icons.
Type: files; Name: "{commonprograms}\Audacity\audacity.exe"
Type: files; Name: "{commonprograms}\Audacity\unins000.exe"
Type: dirifempty; Name: "{commonprograms}\Audacity"

[Registry]
Root: HKCR; Subkey: ".AUP"; ValueType: string; ValueData: "Audacity.Project"; Flags: createvalueifdoesntexist uninsdeletekey; Tasks: associate_aup
Root: HKCR; Subkey: "Audacity.Project"; ValueType: string; ValueData: "Audacity Project File"; Flags: createvalueifdoesntexist uninsdeletekey; Tasks: associate_aup
Root: HKCR; Subkey: "Audacity.Project\shell"; ValueType: string; ValueData: ""; Flags: createvalueifdoesntexist uninsdeletekey; Tasks: associate_aup
Root: HKCR; Subkey: "Audacity.Project\shell\open"; Flags: createvalueifdoesntexist uninsdeletekey; Tasks: associate_aup
Root: HKCR; Subkey: "Audacity.Project\shell\open\command"; ValueType: string; ValueData: "{app}\audacity.exe ""%1"""; Flags: createvalueifdoesntexist uninsdeletekey; Tasks: associate_aup

[Run]
Filename: "{app}\audacity.exe"; Description: "Launch Audacity"; Flags: nowait postinstall skipifsilent

