/**********************************************************************

  Audacity: A Digital Audio Editor

  Menus.cpp

  Dominic Mazzoni
  Brian Gunlogson
  et. al.

*******************************************************************//**

\file Menus.cpp
\brief All AudacityProject functions that provide the menus.  
Implements AudacityProjectCommandFunctor.

  This file implements the method that creates the menu bar, plus
  all of the methods that get called when you select an item
  from a menu.

  All of the menu bar handling is part of the class AudacityProject,
  but the event handlers for all of the menu items have been moved
  to Menus.h and Menus.cpp for clarity.

*//****************************************************************//**

\class AudacityProjectCommandFunctor
\brief AudacityProjectCommandFunctor, derived from CommandFunctor, 
simplifies construction of menu items.

*//*******************************************************************/

#include "Audacity.h"

#include <math.h>

#include <wx/defs.h>
#include <wx/docview.h>
#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/textfile.h>
#include <wx/textdlg.h>
#include <wx/progdlg.h>
#include <wx/scrolbar.h>
#include <wx/ffile.h>

#include "Project.h"
#include "effects/Effect.h"

#include "AudacityApp.h"
#include "AudioIO.h"
#include "Dependencies.h"
#include "LabelTrack.h"
#include "import/ImportMIDI.h"
#include "import/ImportRaw.h"
#include "export/Export.h"
#include "export/ExportMP3.h"
#include "export/ExportMultiple.h"
#include "prefs/PrefsDialog.h"
#include "widgets/TimeTextCtrl.h"
#include "ShuttleGui.h"
#include "HistoryWindow.h"
#include "Internat.h"
#include "FileFormats.h"
#include "FreqWindow.h"
#include "Prefs.h"
#include "Printing.h"
#include "UploadDialog.h"
#include "NoteTrack.h"
#include "Tags.h"
#include "Mix.h"
#include "AboutDialog.h"
#include "Help.h"
#include "Benchmark.h"

#include "Resample.h"
#include "BatchProcessDialog.h"
#include "BatchCommands.h"
#include "prefs/BatchPrefs.h"

#include "toolbars/ToolManager.h"
#include "toolbars/ControlToolBar.h"
#include "toolbars/ToolsToolBar.h"

#include "Experimental.h"
#include "PlatformCompatibility.h"
#include "FileNames.h"
#include "TimeDialog.h"
#include "SmartRecordDialog.h"
#include "LabelDialog.h"

enum {
   kAlignZero=0,
   kAlignCursor,
   kAlignSelStart,
   kAlignSelEnd,
   kAlignEndCursor,
   kAlignEndSelStart,
   kAlignEndSelEnd,
   kAlign
};

typedef void (AudacityProject::*audCommandFunction)();
typedef void (AudacityProject::*audCommandListFunction)(int);

class AudacityProjectCommandFunctor:public CommandFunctor
{
public:
   AudacityProjectCommandFunctor(AudacityProject *project,
                                 audCommandFunction commandFunction)
   {
      mProject = project;
      mCommandFunction = commandFunction;
      mCommandListFunction = NULL;
   }

   AudacityProjectCommandFunctor(AudacityProject *project,
                                 audCommandListFunction commandFunction)
   {
      mProject = project;
      mCommandFunction = NULL;
      mCommandListFunction = commandFunction;
   }

   virtual void operator()(int index = 0)
   {
      if (mCommandListFunction)
         (mProject->*(mCommandListFunction)) (index);
      else
         (mProject->*(mCommandFunction)) ();
   }

private:
   AudacityProject *mProject;
   audCommandFunction mCommandFunction;
   audCommandListFunction mCommandListFunction;
};

// These flags represent the majority of the states that affect
// whether or not items in menus are enabled or disabled.
enum {
   AudioIONotBusyFlag     = 0x00000001,
   TimeSelectedFlag       = 0x00000002,
   TracksSelectedFlag     = 0x00000004,
   TracksExistFlag        = 0x00000008,
   LabelTracksExistFlag   = 0x00000010,
   WaveTracksSelectedFlag = 0x00000020,
   ClipboardFlag          = 0x00000040,
   TextClipFlag           = 0x00000040, // Same as Clipboard flag for now.
   UnsavedChangesFlag     = 0x00000080,
   HasLastEffectFlag      = 0x00000100,
   UndoAvailableFlag      = 0x00000200,
   RedoAvailableFlag      = 0x00000400,
   ZoomInAvailableFlag    = 0x00000800,
   ZoomOutAvailableFlag   = 0x00001000,
   StereoRequiredFlag     = 0x00002000,  //lda
   TopDockHasFocus        = 0x00004000,  //lll
   TrackPanelHasFocus     = 0x00008000,  //lll
   BotDockHasFocus        = 0x00010000,  //lll
   LabelsSelectedFlag     = 0x00020000,
   AudioIOBusyFlag        = 0x00040000,  //lll
   PlayRegionLockedFlag   = 0x00080000,  //msmeyer
   PlayRegionNotLockedFlag= 0x00100000   //msmeyer
};

#define FN(X) new AudacityProjectCommandFunctor(this, &AudacityProject:: X )

/// CreateMenusAndCommands builds the menus, and also rebuilds them after
/// changes in configured preferences - for example changes in key-bindings
/// affect the short-cut key legend that appears beside each command,
/// and changes in 'CleanSpeech Mode' customise the menus to a restricted
/// subset
void AudacityProject::CreateMenusAndCommands()
{
   CommandManager *c = &mCommandManager;
   EffectArray *effects;
   wxArrayString names;
   unsigned int i;

   wxMenuBar *menubar = c->AddMenuBar(wxT("appmenu"));

   //
   // File menu
   //

   c->BeginMenu(_("&File"));
   c->SetDefaultFlags(AudioIONotBusyFlag, AudioIONotBusyFlag);
   c->AddItem(wxT("New"),            _("&New\tCtrl+N"),                   FN(OnNew));
   c->SetCommandFlags(wxT("New"), 0, 0);
   c->AddItem(wxT("Open"),           _("&Open...\tCtrl+O"),               FN(OnOpen));
   c->SetCommandFlags(wxT("Open"), 0, 0);

	// On the Mac, the Preferences item doesn't actually go here...wxMac will pull it out
   // and put it in the Audacity menu for us based on its ID.
   // Moved Preferences to Edit Menu 02/09/05 Richard Ash.

   #ifndef __WXMSW__
   CreateRecentFilesMenu(c);
   #endif

   c->AddItem(wxT("Close"),          _("&Close\tCtrl+W"),                 FN(OnClose));
   if( !mCleanSpeechMode )
   {
      c->AddItem(wxT("Save"),           _("&Save Project\tCtrl+S"),          FN(OnSave));
      c->SetCommandFlags(wxT("Save"),
                         AudioIONotBusyFlag | UnsavedChangesFlag,
                         AudioIONotBusyFlag | UnsavedChangesFlag);
      c->AddItem(wxT("SaveAs"),         _("Save Project &As..."),            FN(OnSaveAs));
   }

   c->AddItem(wxT("CheckDeps"),      _("Chec&k Dependencies..."),          FN(OnCheckDependencies));

   c->AddSeparator();

   c->AddItem(wxT("EditID3"),        _("Open Me&tadata Editor"),              FN(OnEditID3));
   //c->SetCommandFlags(wxT("EditID3"), AudioIONotBusyFlag, AudioIONotBusyFlag);

   if( !mCleanSpeechMode )
	{
      c->AddSeparator();
      
      c->BeginSubMenu(_("&Import..."));
		c->SetDefaultFlags(AudioIONotBusyFlag, AudioIONotBusyFlag);
		c->AddItem(wxT("ImportAudio"),    _("&Audio...\tCtrl+Shift+I"),	FN(OnImport));
		c->AddItem(wxT("ImportLabels"),   _("&Labels..."),					FN(OnImportLabels));
		c->AddItem(wxT("ImportMIDI"),     _("&MIDI..."),							FN(OnImportMIDI));
		c->AddItem(wxT("ImportRaw"),      _("&Raw Data..."),				FN(OnImportRaw));
      c->EndSubMenu();
      
      c->AddSeparator();
      
      c->BeginSubMenu(_("&Export As..."));
      c->AddItem(wxT("Export"),         _("&WAV..."),                   FN(OnExportMix));
      c->AddItem(wxT("ExportMP3"),      _("&MP3..."),               FN(OnExportMP3Mix));
#ifdef USE_LIBVORBIS
      c->AddItem(wxT("ExportOgg"),      _("Ogg &Vorbis..."),        FN(OnExportOggMix));
      // Enable Export commands only when there are tracks
      c->SetCommandFlags(AudioIONotBusyFlag | TracksExistFlag,
                         AudioIONotBusyFlag | TracksExistFlag,
                         wxT("ExportOgg"), NULL);
#endif
#ifdef USE_LIBFLAC
      c->AddItem(wxT("ExportFLAC"),      _("&FLAC..."),        FN(OnExportFLACMix));
      c->SetCommandFlags(AudioIONotBusyFlag | TracksExistFlag,
                         AudioIONotBusyFlag | TracksExistFlag,
                         wxT("ExportFLAC"), NULL);
#endif
#ifdef USE_LIBTWOLAME
      c->AddItem(wxT("ExportMP2"), _("MP&2..."), FN(OnExportMP2Mix));
      c->SetCommandFlags(AudioIONotBusyFlag | TracksExistFlag,
                         AudioIONotBusyFlag | TracksExistFlag,
                         wxT("ExportMP2"), NULL);
#endif
      c->EndSubMenu();
      
      c->BeginSubMenu(_("Expo&rt Selection As..."));
      c->AddItem(wxT("ExportSel"),      _("&WAV..."),         FN(OnExportSelection));
      c->AddItem(wxT("ExportMP3Sel"),   _("&MP3..."),     FN(OnExportMP3Selection));
#ifdef USE_LIBVORBIS
      c->AddItem(wxT("ExportOggSel"),   _("Ogg &Vorbis..."), FN(OnExportOggSelection));
      // Enable Export Selection commands only when there's a selection
      c->SetCommandFlags(AudioIONotBusyFlag | TimeSelectedFlag | TracksSelectedFlag,
                         AudioIONotBusyFlag | TimeSelectedFlag | TracksSelectedFlag,
                         wxT("ExportOggSel"), NULL);
#endif
#ifdef USE_LIBFLAC
      c->AddItem(wxT("ExportFLACSel"),   _("&FLAC..."), FN(OnExportFLACSelection));
      c->SetCommandFlags(AudioIONotBusyFlag | TracksExistFlag | TracksSelectedFlag,
                         AudioIONotBusyFlag | TracksExistFlag | TracksSelectedFlag,
                         wxT("ExportFLACSel"), NULL);
#endif
#ifdef USE_LIBTWOLAME
      c->AddItem(wxT("ExportMP2Sel"),    _("&MP&2..."), FN(OnExportMP2Selection));
      c->SetCommandFlags(AudioIONotBusyFlag | TracksExistFlag | TracksSelectedFlag,
                         AudioIONotBusyFlag | TracksExistFlag | TracksSelectedFlag,
                         wxT("ExportMP2Sel"), NULL);
#endif
      c->EndSubMenu();
      
      // Enable Export commands only when there are tracks
      c->SetCommandFlags(AudioIONotBusyFlag | TracksExistFlag,
                         AudioIONotBusyFlag | TracksExistFlag,
                         wxT("Export"), wxT("ExportMP3"), NULL);
      // Enable Export Selection commands only when there's a selection
      c->SetCommandFlags(AudioIONotBusyFlag | TimeSelectedFlag | TracksSelectedFlag,
                         AudioIONotBusyFlag | TimeSelectedFlag | TracksSelectedFlag,
                         wxT("ExportSel"), wxT("ExportMP3Sel"), NULL);
      
      c->AddSeparator();
      c->AddItem(wxT("ExportLabels"),   _("Export &Labels..."),              FN(OnExportLabels));
      c->AddItem(wxT("ExportMultiple"),   _("Export &Multiple..."),              FN(OnExportMultiple));
      
      c->SetCommandFlags(wxT("ExportLabels"),
                         AudioIONotBusyFlag | LabelTracksExistFlag,
                         AudioIONotBusyFlag | LabelTracksExistFlag);
      c->SetCommandFlags(wxT("ExportMultiple"),
                         AudioIONotBusyFlag | TracksExistFlag,
                         AudioIONotBusyFlag | TracksExistFlag);                      
   }

   c->AddSeparator();

	if( mCleanSpeechMode )
	{
      
      c->BeginSubMenu(_("&Export As..."));
      c->AddItem(wxT("Export"),         _("&WAV..."),                   FN(OnExportMix));
      c->AddItem(wxT("ExportMP3"),      _("&MP3..."),               FN(OnExportMP3Mix));
      c->EndSubMenu();
      
      c->BeginSubMenu(_("Expo&rt Selection As..."));
      c->AddItem(wxT("ExportSel"),      _("&WAV..."),         FN(OnExportSelection));
      c->AddItem(wxT("ExportMP3Sel"),   _("&MP3..."),     FN(OnExportMP3Selection));
      c->EndSubMenu();
      
      // Enable Export commands only when there are tracks
      c->SetCommandFlags(AudioIONotBusyFlag | TracksExistFlag,
                         AudioIONotBusyFlag | TracksExistFlag,
                         wxT("Export"), wxT("ExportMP3"), NULL);
      // Enable Export Selection commands only when there's a selection
      c->SetCommandFlags(AudioIONotBusyFlag | TimeSelectedFlag | TracksSelectedFlag,
                         AudioIONotBusyFlag | TimeSelectedFlag | TracksSelectedFlag,
                         wxT("ExportSel"), wxT("ExportMP3Sel"), NULL);
      
      c->AddSeparator();

      c->AddItem(wxT("ApplyChain"), _("CleanSpeech C&hain..."),   FN(OnApplyChain));
      c->AddItem(wxT("EditChains"), _("Edit C&hains..."), FN(OnEditChains));
      c->AddItem(wxT("ExportCcSettings"), _("Export CleanSpeech &Presets..."),   FN(OnExportCleanSpeechPresets));
      c->AddItem(wxT("ImportCcSettings"), _("I&mport CleanSpeech Presets..."),   FN(OnImportCleanSpeechPresets));
      c->SetCommandFlags(wxT("BatchProcess"), AudioIONotBusyFlag, AudioIONotBusyFlag);
#ifdef __WXDEBUG__
	   gPrefs->Write(wxT("/Validate/DebugBuild"), "Y");
#else
	   gPrefs->Write(wxT("/Validate/DebugBuild"), "N");
#endif
	}
   else
   {
      c->AddItem(wxT("AppplyChain"), _("Appl&y Chain..."), FN(OnApplyChain));
      c->SetCommandFlags(wxT("BatchProcess"), AudioIONotBusyFlag, AudioIONotBusyFlag);
      c->AddItem(wxT("EditChains"), _("Edit C&hains..."), FN(OnEditChains));
   }

   c->AddSeparator();
   c->AddItem(wxT("Upload File"),      _("&Upload File..."),            FN(OnUpload));

   if( !mCleanSpeechMode )
	{
      c->AddSeparator();
      c->AddItem(wxT("PageSetup"),   _("Pa&ge Setup..."),              FN(OnPageSetup));
      c->AddItem(wxT("Print"),       _("&Print..."),                   FN(OnPrint));
      c->SetCommandFlags(wxT("PageSetup"),
                         AudioIONotBusyFlag | TracksExistFlag,
                         AudioIONotBusyFlag | TracksExistFlag);
      c->SetCommandFlags(wxT("Print"),
                         AudioIONotBusyFlag | TracksExistFlag,
                         AudioIONotBusyFlag | TracksExistFlag);   
	}
   
   c->AddSeparator();

   #ifdef __WXMSW__
   CreateRecentFilesMenu(c);
   #endif

   // On the Mac, the Exit item doesn't actually go here...wxMac will pull it out
   // and put it in the Audacity menu for us based on its ID.
  #ifdef __WXMAC__
   c->AddItem(wxT("Exit"),           _("E&xit\tCtrl+Q"),                          FN(OnExit));
   c->SetCommandFlags(wxT("Exit"), 0, 0);
  #else
   c->AddItem(wxT("Exit"),           _("E&xit\tCtrl+Q"),                          FN(OnExit));
   c->SetCommandFlags(wxT("Exit"), 0, 0);
  #endif
   c->EndMenu();

   //
   // Edit Menu
   //

   c->BeginMenu(_("&Edit"));
   c->SetDefaultFlags(AudioIONotBusyFlag | TimeSelectedFlag | TracksSelectedFlag,
                      AudioIONotBusyFlag | TimeSelectedFlag | TracksSelectedFlag);

   c->AddItem(wxT("Undo"),           _("&Undo\tCtrl+Z"),                  FN(OnUndo));
   c->SetCommandFlags(wxT("Undo"),
                      AudioIONotBusyFlag | UndoAvailableFlag,
                      AudioIONotBusyFlag | UndoAvailableFlag);

   // The default shortcut key for Redo is different
   // on different platforms.

   wxString redoLabel = _("&Redo");
   #ifdef __WXMSW__
   redoLabel += wxT("\tCtrl+Y");
   #else
   redoLabel += wxT("\tCtrl+Shift+Z");
   #endif

   c->AddItem(wxT("Redo"),           redoLabel,                           FN(OnRedo));
   c->SetCommandFlags(wxT("Redo"),
                      AudioIONotBusyFlag | RedoAvailableFlag,
                      AudioIONotBusyFlag | RedoAvailableFlag);

   c->AddSeparator();
   c->AddItem(wxT("Cut"),            _("Cu&t\tCtrl+X"),                   FN(OnCut));
   c->AddItem(wxT("SplitCut"),       _("Spl&it Cut\tCtrl+Alt+X"),          FN(OnSplitCut));
   c->AddItem(wxT("Copy"),           _("&Copy\tCtrl+C"),                  FN(OnCopy)); 
   c->AddItem(wxT("Paste"),          _("&Paste\tCtrl+V"),                 FN(OnPaste));
   c->SetCommandFlags(wxT("Paste"),
                      AudioIONotBusyFlag | ClipboardFlag,
                      AudioIONotBusyFlag | ClipboardFlag);
   c->AddItem(wxT("Trim"),           _("Tri&m\tCtrl+T"),                  FN(OnTrim));
   c->AddSeparator();
   c->AddItem(wxT("Delete"),         _("&Delete\tCtrl+K"),                FN(OnDelete));
   c->AddItem(wxT("SplitDelete"),    _("Split D&elete\tCtrl+Alt+K"),       FN(OnSplitDelete));
   c->AddItem(wxT("Silence"),        _("&Silence\tCtrl+L"),               FN(OnSilence));
   c->AddSeparator();
   c->AddItem(wxT("Split"),          _("Sp&lit\tCtrl+I"),                         FN(OnSplit));
   c->SetCommandFlags(wxT("Split"),
      AudioIONotBusyFlag | WaveTracksSelectedFlag,
      AudioIONotBusyFlag | WaveTracksSelectedFlag);

   c->AddItem(wxT("Join"),           _("&Join\tCtrl+J"),                  FN(OnJoin));
   c->AddItem(wxT("Disjoin"),        _("Disj&oin\tCtrl+Alt+J"),                       FN(OnDisjoin));
   
   c->AddItem(wxT("Duplicate"),      _("Duplic&ate\tCtrl+D"),             FN(OnDuplicate));

   // An anomoloy... StereoToMono and EditID3 are added here for CleanSpeech, 
   // which doesn't have a Project menu, but they are under Project for normal Audacity.
   if( mCleanSpeechMode )
	{
      c->AddItem(wxT("Stereo To Mono"),      _("Stereo To Mono"),            FN(OnStereoToMono));
      c->SetCommandFlags(wxT("Stereo To Mono"),
         AudioIONotBusyFlag | StereoRequiredFlag | WaveTracksSelectedFlag,
         AudioIONotBusyFlag | StereoRequiredFlag | WaveTracksSelectedFlag);
	}
   c->AddSeparator();

   c->BeginSubMenu( _( "Labeled Re&gions..." ) );
   c->AddItem( wxT( "CutLabels" ), _( "&Cut\tAlt+X" ), 
         FN( OnCutLabels ) );
   c->SetCommandFlags( wxT( "CutLabels" ), LabelsSelectedFlag, 
         LabelsSelectedFlag );
   c->AddItem( wxT( "SplitCutLabels" ), _( "&Split Cut\tShift+Alt+X" ), 
         FN( OnSplitCutLabels ) );
   c->SetCommandFlags( wxT( "SplitCutLabels" ), LabelsSelectedFlag, 
         LabelsSelectedFlag );
   c->AddItem( wxT( "CopyLabels" ), _( "Co&py\tShift+Alt+C" ), 
         FN( OnCopyLabels ) );
   c->SetCommandFlags( wxT( "CopyLabels" ), LabelsSelectedFlag, 
         LabelsSelectedFlag );
   c->AddSeparator();
   c->AddItem( wxT( "DeleteLabels" ), _( "&Delete\tAlt+K" ), 
         FN( OnDeleteLabels ) );
   c->SetCommandFlags( wxT( "DeleteLabels" ), LabelsSelectedFlag, 
         LabelsSelectedFlag );
   c->AddItem( wxT( "SplitDeleteLabels" ), _( "Sp&lit Delete\tShift+Alt+K" ), 
         FN( OnSplitDeleteLabels ) );
   c->SetCommandFlags( wxT( "SplitDeleteLabels" ), LabelsSelectedFlag, 
         LabelsSelectedFlag );
   c->AddItem( wxT( "SilenceLabels" ), _( "Sile&nce\tAlt+L" ), 
         FN( OnSilenceLabels ) );
   c->SetCommandFlags( wxT( "SilenceLabels" ), LabelsSelectedFlag, 
         LabelsSelectedFlag );
   c->AddSeparator();
   c->AddItem( wxT( "SplitLabels" ), _( "Spli&t\tAlt+I" ), 
         FN( OnSplitLabels ) );
   c->SetCommandFlags( wxT( "SplitLabels" ), LabelsSelectedFlag, 
         LabelsSelectedFlag );
   c->AddItem( wxT( "JoinLabels" ), _( "&Join\tAlt+J" ), 
         FN( OnJoinLabels ) );
   c->SetCommandFlags( wxT( "JoinLabels" ), LabelsSelectedFlag, 
         LabelsSelectedFlag );
   c->AddItem( wxT( "DisjoinLabels" ), _( "Disj&oin\tShift+Alt+J" ), 
         FN( OnDisjoinLabels ) );
   c->SetCommandFlags( wxT( "DisjoinLabels" ), LabelsSelectedFlag, 
         LabelsSelectedFlag );
   c->EndSubMenu();
   
   c->BeginSubMenu(_("Select..."));
   c->AddItem(wxT("SelectAll"),      _("&All\tCtrl+A"),                   FN(OnSelectAll));
   c->SetCommandFlags(wxT("SelectAll"),
                      TracksExistFlag, TracksExistFlag);

   c->AddItem(wxT("SetLeftSelection"),_("Left at Playback Position\t["), FN(OnSetLeftSelection));
   c->AddItem(wxT("SetRightSelection"),_("Right at Playback Position\t]"), FN(OnSetRightSelection));
   c->SetCommandFlags(TracksExistFlag, TracksExistFlag,
                      wxT("SetLeftSelection"), wxT("SetRightSelection"), NULL);

   c->AddItem(wxT("SelStartCursor"), _("Start to Cursor"),                FN(OnSelectStartCursor));
   c->AddItem(wxT("SelCursorEnd"),   _("Cursor to End"),                  FN(OnSelectCursorEnd));
   c->SetCommandFlags(TracksSelectedFlag, TracksSelectedFlag,
                      wxT("SelStartCursor"), wxT("SelCursorEnd"), NULL);

   c->EndSubMenu();

   c->AddItem(wxT("ZeroCross"),      _("Find &Zero Crossings\tZ"),         FN(OnZeroCrossing));

   c->BeginSubMenu(_("Mo&ve Cursor..."));

   c->AddItem(wxT("CursSelStart"),   _("to Selection Start"),             FN(OnCursorSelStart));
   c->AddItem(wxT("CursSelEnd"),     _("to Selection End"),               FN(OnCursorSelEnd));
   c->SetCommandFlags(TimeSelectedFlag, TimeSelectedFlag,
                      wxT("CursSelStart"), wxT("CursSelEnd"), NULL);
                      
   c->AddItem(wxT("CursTrackStart"), _("to Track Start"),                 FN(OnCursorTrackStart));
   c->AddItem(wxT("CursTrackEnd"),   _("to Track End"),                   FN(OnCursorTrackEnd));
   c->SetCommandFlags(TracksSelectedFlag, TracksSelectedFlag,
                      wxT("CursTrackStart"), wxT("CursTrackEnd"), NULL);

   c->EndSubMenu();

   c->AddSeparator();
   c->AddItem(wxT("SelSave"),        _("Selection Save"),                 FN(OnSelectionSave));
   c->AddItem(wxT("SelRestore"),     _("Selection Restore"),              FN(OnSelectionRestore));
   c->SetCommandFlags(TracksExistFlag, TracksExistFlag,
                      wxT("SelSave"), wxT("SelRestore"), NULL);
   c->AddSeparator();

   c->BeginSubMenu(_("S&nap-To..."));

   /* i18n-hint: Set snap-to mode on or off */
   c->AddItem(wxT("SnapOn"),         _("On"),                        FN(OnSnapOn));
   /* i18n-hint: Set snap-to mode on or off */
   c->AddItem(wxT("SnapOff"),        _("Off"),                       FN(OnSnapOff));

   c->SetCommandFlags(0, 0, wxT("SnapOn"), wxT("SnapOff"), NULL);

   c->EndSubMenu();
   
   c->BeginSubMenu(_("Play &Region..."));
   c->AddItem(wxT("LockPlayRegion"), _("Lock"), FN(OnLockPlayRegion));
   c->AddItem(wxT("UnlockPlayRegion"), _("Unlock"), FN(OnUnlockPlayRegion));
   c->SetCommandFlags(wxT("LockPlayRegion"), PlayRegionNotLockedFlag, PlayRegionNotLockedFlag);
   c->SetCommandFlags(wxT("UnlockPlayRegion"), PlayRegionLockedFlag, PlayRegionLockedFlag);
   c->EndSubMenu();

   // Alternate strings
   wxString dummy1 = _("Turn Snap-To On");
   wxString dummy2 = _("Turn Snap-To Off");
   wxString dummy3 = _("Turn Grid Snap On");
   wxString dummy4 = _("Turn Grid Snap Off");

   // Moved Preferences from File Menu 02/09/05 Richard Ash.
  #ifdef __WXMAC__
   /* i18n-hint: Mac OS X Preferences shortcut should be Ctrl+, */
   c->AddItem(wxT("Preferences"),    _("&Preferences...\tCtrl+,"),        FN(OnPreferences));
  #else
   /* i18n-hint: On Windows and Linux, the Preferences shortcut is usually Ctrl+P */
   c->AddSeparator();
   c->AddItem(wxT("Preferences"),    _("&Preferences...\tCtrl+P"),        FN(OnPreferences));
  #endif
   c->SetCommandFlags(wxT("Preferences"), AudioIONotBusyFlag, AudioIONotBusyFlag);

   c->EndMenu();

   //
   // View Menu
   //

   c->BeginMenu(_("&View"));
   c->SetDefaultFlags(0, 0);
   c->AddItem(wxT("ZoomIn"),         _("Zoom &In\tCtrl+1"),               FN(OnZoomIn));
   c->SetCommandFlags(wxT("ZoomIn"), ZoomInAvailableFlag, ZoomInAvailableFlag);

   c->AddItem(wxT("ZoomNormal"),     _("Zoom &Normal\tCtrl+2"),           FN(OnZoomNormal));
   c->AddItem(wxT("ZoomOut"),        _("Zoom &Out\tCtrl+3"),              FN(OnZoomOut));
   c->SetCommandFlags(wxT("ZoomOut"), ZoomOutAvailableFlag, ZoomOutAvailableFlag);

   c->AddItem(wxT("FitInWindow"),    _("&Fit in Window\tCtrl+F"),         FN(OnZoomFit));
   c->AddItem(wxT("FitV"),           _("Fit &Vertically\tCtrl+Shift+F"),  FN(OnZoomFitV));
   c->AddItem(wxT("ZoomSel"),        _("&Zoom to Selection\tCtrl+E"),     FN(OnZoomSel));

   c->AddSeparator();
   c->AddItem(wxT("CollapseAllTracks"), _("&Collapse All Tracks\tCtrl+Shift+C"), FN(OnCollapseAllTracks));
   c->AddItem(wxT("ExpandAllTracks"), _("E&xpand All Tracks\tCtrl+Shift+X"), FN(OnExpandAllTracks));

   c->AddSeparator();
//   c->BeginSubMenu(_("Set Selection Format"));
//   c->AddItemList(wxT("SelectionFormat"), GetSelectionFormats(), FN(OnSelectionFormat));
//   c->EndSubMenu();
//
//   c->AddSeparator();
   c->AddItem(wxT("UndoHistory"),    _("&History..."),               FN(OnHistory));
#ifdef EXPERIMENTAL_TRACK_PANEL
   c->AddItem(wxT("NewTrackPanel"),  _("&Experimental Display..."),  FN(OnExperimentalTrackPanel));
#endif
   c->AddSeparator();
   c->BeginSubMenu(_("&Toolbars..."));
   c->AddItem(wxT("ShowControlTB"),       _("Show Control Toolbar"),       FN(OnShowControlToolBar));
   c->AddItem(wxT("ShowDeviceTB"),        _("Show Device Toolbar"),        FN(OnShowDeviceToolBar));
   c->AddItem(wxT("ShowEditTB"),          _("Show Edit Toolbar"),          FN(OnShowEditToolBar));
   c->AddItem(wxT("ShowMeterTB"),         _("Show Meter Toolbar"),         FN(OnShowMeterToolBar));
   c->AddItem(wxT("ShowMixerTB"),         _("Show Mixer Toolbar"),         FN(OnShowMixerToolBar));
   c->AddItem(wxT("ShowSelectionTB"),     _("Show Selection Toolbar"),     FN(OnShowSelectionToolBar));
   c->AddItem(wxT("ShowToolsTB"),         _("Show Tools Toolbar"),         FN(OnShowToolsToolBar));
   c->AddItem(wxT("ShowTranscriptionTB"), _("Show Transcription Toolbar"), FN(OnShowTranscriptionToolBar));
   c->EndSubMenu();

   c->EndMenu();

   //
   // Tracks Menu (formerly Project Menu)
   //
	if( !mCleanSpeechMode )
	{
      c->BeginMenu(_("&Tracks"));
      c->SetDefaultFlags(AudioIONotBusyFlag, AudioIONotBusyFlag);
      /*c->BeginSubMenu(_("&Current Track..."));
		c->EndSubMenu();

      c->AddSeparator();*/
		
		c->BeginSubMenu(_("Add &New..."));
			c->AddItem(wxT("NewAudioTrack"),  _("&Audio Track\tCtrl+Shift+N"),               FN(OnNewWaveTrack));
			c->AddItem(wxT("NewStereoTrack"), _("&Stereo Track"),              FN(OnNewStereoTrack));
			c->AddItem(wxT("NewLabelTrack"),  _("&Label Track"),               FN(OnNewLabelTrack));
			c->AddItem(wxT("NewTimeTrack"),   _("&Time Track"),                FN(OnNewTimeTrack));
		c->EndSubMenu();

      c->AddItem(wxT("SmartRecord"), _("&Timer Record"), FN(OnSmartRecord));

      c->AddSeparator();
      // StereoToMono moves elsewhere in the menu when in CleanSpeech mode.
      // It belongs here normally, because it is a kind of mix-down.
      c->AddItem(wxT("Stereo To Mono"),      _("&Stereo To Mono"),            FN(OnStereoToMono));
      c->SetCommandFlags(wxT("Stereo To Mono"),
         AudioIONotBusyFlag | StereoRequiredFlag | WaveTracksSelectedFlag,
         AudioIONotBusyFlag | StereoRequiredFlag | WaveTracksSelectedFlag);
      c->AddItem(wxT("MixAndRender"),       _("&Mix and Render"),             FN(OnMixAndRender));
      c->SetCommandFlags(wxT("MixAndRender"),
                         AudioIONotBusyFlag | WaveTracksSelectedFlag,
                         AudioIONotBusyFlag | WaveTracksSelectedFlag);
      c->AddItem(wxT("Resample"), _("&Resample..."), FN(OnResample));
      c->SetCommandFlags(wxT("Resample"),
                         AudioIONotBusyFlag | WaveTracksSelectedFlag,
                         AudioIONotBusyFlag | WaveTracksSelectedFlag);
      c->AddSeparator();
     /* c->AddItem(wxT("NewAudioTrack"),  _("New &Audio Track\tShift+Ctrl+N"),               FN(OnNewWaveTrack));
      c->AddItem(wxT("NewStereoTrack"), _("New &Stereo Track"),              FN(OnNewStereoTrack));
      c->AddItem(wxT("NewLabelTrack"),  _("New La&bel Track"),               FN(OnNewLabelTrack));
      c->AddItem(wxT("NewTimeTrack"),   _("New &Time Track"),                FN(OnNewTimeTrack));
      c->AddSeparator();*/
      c->AddItem(wxT("RemoveTracks"),   _("Remo&ve Tracks"),                 FN(OnRemoveTracks));
      c->SetCommandFlags(wxT("RemoveTracks"),
                         AudioIONotBusyFlag | TracksSelectedFlag,
                         AudioIONotBusyFlag | TracksSelectedFlag);
      c->AddSeparator();
   
      wxArrayString alignLabels;
      alignLabels.Add(_("Align with &Zero"));
      alignLabels.Add(_("Align with &Cursor"));
      alignLabels.Add(_("Align with Selection &Start"));
      alignLabels.Add(_("Align with Selection &End"));
      alignLabels.Add(_("Align End with Cursor"));
      alignLabels.Add(_("Align End with Selection Start"));
      alignLabels.Add(_("Align End with Selection End"));
      alignLabels.Add(_("Align Tracks Together"));
   
      c->BeginSubMenu(_("&Align Tracks..."));
      c->AddItemList(wxT("Align"), alignLabels, FN(OnAlign));
      c->SetCommandFlags(wxT("Align"),
                         AudioIONotBusyFlag | TracksSelectedFlag,
                         AudioIONotBusyFlag | TracksSelectedFlag);
      c->EndSubMenu();
   
      alignLabels.RemoveAt(7); // Can't align together and move cursor
   
      c->BeginSubMenu(_("Ali&gn And Move Cursor..."));
      c->AddItemList(wxT("AlignMove"), alignLabels, FN(OnAlignMoveSel));
      c->SetCommandFlags(wxT("AlignMove"),
                         AudioIONotBusyFlag | TracksSelectedFlag,
                         AudioIONotBusyFlag | TracksSelectedFlag);
      c->EndSubMenu();
   
      c->AddSeparator();   
      c->AddItem(wxT("AddLabel"),       _("Add &Label At Selection\tCtrl+B"), FN(OnAddLabel));
      c->AddItem(wxT("AddLabelPlaying"),       _("Add Label At &Playback Position\tCtrl+M"), FN(OnAddLabelPlaying));
      c->SetCommandFlags(wxT("AddLabel"), 0, 0);
      c->SetCommandFlags(wxT("AddLabelPlaying"), 0, AudioIONotBusyFlag);
      c->AddItem(wxT("EditLabels"),       _("&Edit Labels"), FN(OnEditLabels));

      c->AddSeparator();   
      c->BeginSubMenu(_("S&ort tracks by..."));
      c->AddItem(wxT("SortByTime"), _("&Start time"), FN(OnSortTime));
      c->SetCommandFlags(wxT("SortByTime"), TracksExistFlag, TracksExistFlag);
      c->AddItem(wxT("SortByName"), _("&Name"), FN(OnSortName));
      c->SetCommandFlags(wxT("SortByName"), TracksExistFlag, TracksExistFlag);
      c->EndSubMenu();

      c->EndMenu();
   
      //
      // Generate, Effect & Analyze menus
      //
   
      c->BeginMenu(_("&Generate"));
      c->SetDefaultFlags(AudioIONotBusyFlag,
                         AudioIONotBusyFlag);
   
      effects = Effect::GetEffects(INSERT_EFFECT | BUILTIN_EFFECT);
      if(effects->GetCount()){
         for(i=0; i<effects->GetCount(); i++)
            names.Add((*effects)[i]->GetEffectName());
         c->AddItemList(wxT("Generate"), names, FN(OnGenerateEffect));
      }
      delete effects;
   
      effects = Effect::GetEffects(INSERT_EFFECT | PLUGIN_EFFECT);
      if (effects->GetCount()) {
         c->AddSeparator();
         names.Clear();
         for(i=0; i<effects->GetCount(); i++)
            names.Add((*effects)[i]->GetEffectName());
         c->AddItemList(wxT("GeneratePlugin"), names, FN(OnGeneratePlugin), true);
      }
      delete effects;
      c->EndMenu();
	}
   c->BeginMenu(_("Effe&ct"));
   c->SetDefaultFlags(AudioIONotBusyFlag | TimeSelectedFlag | WaveTracksSelectedFlag,
                      AudioIONotBusyFlag | TimeSelectedFlag | WaveTracksSelectedFlag);

   c->AddItem(wxT("RepeatLastEffect"),     _("Repeat Last Effect\tCtrl+R"),    FN(OnRepeatLastEffect));
   c->SetCommandFlags(wxT("RepeatLastEffect"),
                      AudioIONotBusyFlag | TimeSelectedFlag | WaveTracksSelectedFlag | HasLastEffectFlag,
                      AudioIONotBusyFlag | TimeSelectedFlag | WaveTracksSelectedFlag | HasLastEffectFlag);
   c->AddSeparator();

   int additionalEffects=ADVANCED_EFFECT;
   // set additionalEffects to zero to exclude the advanced effects.
   if( mCleanSpeechMode )
       additionalEffects = 0;
   effects = Effect::GetEffects(PROCESS_EFFECT | BUILTIN_EFFECT | additionalEffects);
   if(effects->GetCount()){
      names.Clear();
      for(i=0; i<effects->GetCount(); i++)
         names.Add((*effects)[i]->GetEffectName());
      c->AddItemList(wxT("Effect"), names, FN(OnProcessEffect));
   }
   delete effects;


	if( !mCleanSpeechMode )
	{
      effects = Effect::GetEffects(PROCESS_EFFECT | PLUGIN_EFFECT);
      if (effects->GetCount()) {
         c->AddSeparator();
         names.Clear();
         for(i=0; i<effects->GetCount(); i++)
            names.Add((*effects)[i]->GetEffectName());
         c->AddItemList(wxT("EffectPlugin"), names, FN(OnProcessPlugin), true);
      }
      delete effects;
      c->EndMenu();
      c->BeginMenu(_("&Analyze"));
 	   /* plot spectrum moved from view */
      c->AddItem(wxT("PlotSpectrum"), _("&Plot Spectrum..."), FN(OnPlotSpectrum));
      c->SetCommandFlags(wxT("PlotSpectrum"),
                           AudioIONotBusyFlag | WaveTracksSelectedFlag | TimeSelectedFlag,
                           AudioIONotBusyFlag | WaveTracksSelectedFlag | TimeSelectedFlag);
      
      effects = Effect::GetEffects(ANALYZE_EFFECT | BUILTIN_EFFECT);
      if(effects->GetCount()){
         names.Clear();
         for(i=0; i<effects->GetCount(); i++)
            names.Add((*effects)[i]->GetEffectName());
         c->AddItemList(wxT("Analyze"), names, FN(OnAnalyzeEffect));
      }
      delete effects;
      
      effects = Effect::GetEffects(ANALYZE_EFFECT | PLUGIN_EFFECT);
      if (effects->GetCount()) {
         c->AddSeparator();
         names.Clear();
         for(i=0; i<effects->GetCount(); i++)
            names.Add((*effects)[i]->GetEffectName());
         c->AddItemList(wxT("AnalyzePlugin"), names, FN(OnAnalyzePlugin), true);
      }
      delete effects;
      c->EndMenu();
	}

   // Resolve from list of all effects, 
   effects = Effect::GetEffects(ALL_EFFECTS);
   ResolveEffectIndices(effects);
   delete effects;


   #ifdef __WXMAC__
   wxGetApp().s_macHelpMenuTitleName = _("&Help");
   #endif

   c->BeginMenu(_("&Help"));
   c->SetDefaultFlags(0, 0);
   c->AddItem(wxT("Help"),           _("&Contents..."),             FN(OnHelp));
   c->AddSeparator();   
	if( mCleanSpeechMode )
   	c->AddItem(wxT("About"),          _("&About Audacity CleanSpeech..."), FN(OnAbout));
	else
   c->AddItem(wxT("About"),          _("&About Audacity..."),          FN(OnAbout));

#if 1 // Benchmark is enabled in unstable builds
	if( !mCleanSpeechMode )
	{
   c->AddSeparator();   
   c->AddItem(wxT("Benchmark"),      _("&Run Benchmark..."),           FN(OnBenchmark));
	}
#endif 

   c->EndMenu();

   ModifyExportMenus();

   SetMenuBar(menubar);

   c->SetDefaultFlags(0, 0);
   c->AddCommand(wxT("PrevFrame"),   _("Cycle backward through Dock, Track View, and Selection Bar\tCtrl+Shift+F6"), FN(PrevFrame));
   c->AddCommand(wxT("NextFrame"),   _("Cycle forward through Dock, Track View, and Selection Bar\tCtrl+F6"), FN(NextFrame));

//   c->SetDefaultFlags(TrackPanelHasFocus, TrackPanelHasFocus);
   c->AddCommand(wxT("SelectTool"),  _("Selection Tool\tF1"),          FN(OnSelectTool));
   c->AddCommand(wxT("EnvelopeTool"),_("Envelope Tool\tF2"),           FN(OnEnvelopeTool));
   c->AddCommand(wxT("DrawTool"),    _("Draw Tool\tF3"),               FN(OnDrawTool));
   c->AddCommand(wxT("ZoomTool"),    _("Zoom Tool\tF4"),               FN(OnZoomTool));
   c->AddCommand(wxT("TimeShiftTool"),_("Time Shift Tool\tF5"),        FN(OnTimeShiftTool));
   c->AddCommand(wxT("MultiTool"),   _("Multi Tool\tF6"),              FN(OnMultiTool));

   c->AddCommand(wxT("NextTool"),   _("Next Tool\tD"),                 FN(OnNextTool));
   c->AddCommand(wxT("PrevTool"),   _("Previous Tool\tA"),             FN(OnPrevTool));

   c->AddCommand(wxT("Play/Stop"),   _("Play/Stop\tSpacebar"),         FN(OnPlayStop));
   c->AddCommand(wxT("Stop"),        _("Stop\tS"),                     FN(OnStop));
   c->AddCommand(wxT("Pause"),       _("Pause\tP"),                    FN(OnPause));
   c->AddCommand(wxT("Record"),      _("Record\tR"),                   FN(OnRecord));
   
   c->AddCommand(wxT("StopSelect"),  _("Stop and Select\tShift+A"),    FN(OnStopSelect));

   c->AddCommand(wxT("PlayOneSec"),     _("Play One Second\t1"),       FN(OnPlayOneSecond));
   c->AddCommand(wxT("PlayToSelection"),_("Play To Selection\tB"),       FN(OnPlayToSelection));
   c->AddCommand(wxT("PlayLooped"),     _("Play Looped\tL"),           FN(OnPlayLooped));
   c->AddCommand(wxT("PlayLoopAlt"),    _("Play Looped\tShift+Spacebar"), FN(OnPlayLooped));
   c->AddCommand(wxT("PlayCutPreview"), _("Play Cut Preview\tC"),      FN(OnPlayCutPreview));

   c->AddCommand(wxT("SkipStart"),   _("Skip to Start\tHome"),         FN(OnSkipStart));
   c->AddCommand(wxT("SkipEnd"),     _("Skip to End\tEnd"),            FN(OnSkipEnd));

   c->AddCommand(wxT("SelStart"),    _("Selection to Start\tShift+Home"), FN(OnSelToStart));
   c->AddCommand(wxT("SelEnd"),      _("Selection to End\tShift+End"),    FN(OnSelToEnd));

   c->AddCommand(wxT("DeleteKey"),      _("DeleteKey\tBackspace"),           FN(OnDelete));
   c->SetCommandFlags(wxT("DeleteKey"),
                      AudioIONotBusyFlag | TracksSelectedFlag | TimeSelectedFlag,
                      AudioIONotBusyFlag | TracksSelectedFlag | TimeSelectedFlag);

   c->AddCommand(wxT("DeleteKey2"),      _("DeleteKey2\tDelete"),           FN(OnDelete));
   c->SetCommandFlags(wxT("DeleteKey2"),
                      AudioIONotBusyFlag | TracksSelectedFlag | TimeSelectedFlag,
                      AudioIONotBusyFlag | TracksSelectedFlag | TimeSelectedFlag);

   c->SetDefaultFlags(AudioIOBusyFlag, AudioIOBusyFlag);
   c->AddCommand(wxT("SeekLeftShort"), _("Seek left short period during playback\tLeft"),        FN(OnSeekLeftShort));
   c->AddCommand(wxT("SeekRightShort"),_("Seek right short period during playback\tRight"),      FN(OnSeekRightShort));
   c->AddCommand(wxT("SeekLeftLong"),  _("Seek left long period during playback\tShift+Left"),   FN(OnSeekLeftLong));
   c->AddCommand(wxT("SeekRightLong"), _("Seek right long period during playback\tShift+Right"), FN(OnSeekRightLong));
   
   c->SetDefaultFlags(TracksExistFlag | TrackPanelHasFocus,
                      TracksExistFlag | TrackPanelHasFocus);
   c->AddCommand(wxT("PrevTrack"),     _("Move to Previous Track\tUp"),                      FN(OnCursorUp));
   c->AddCommand(wxT("ShiftUp"),       _("Move to Previous and Change Selection\tShift+Up"), FN(OnShiftUp));
   c->AddCommand(wxT("NextTrack"),     _("Move to Next Track\tDown"),                        FN(OnCursorDown));
   c->AddCommand(wxT("ShiftDown"),     _("Move to Next and Change Selection\tShift+Down"),   FN(OnShiftDown));
   c->AddCommand(wxT("Toggle"),        _("Toggle Focused Track\tReturn"),                    FN(OnToggle));
   c->AddCommand(wxT("Toggle1"),       _("Toggle Focused Track\tNUMPAD_ENTER"),              FN(OnToggle));
   c->AddCommand(wxT("Toggle2"),       _("Toggle Focused Track\tCtrl+Spacebar"),             FN(OnToggle));

   c->AddCommand(wxT("CursorLeft"),    _("Cursor Left\tLeft"),                               FN(OnCursorLeft));
   c->AddCommand(wxT("CursorRight"),   _("Cursor Right\tRight"),                             FN(OnCursorRight));
   c->AddCommand(wxT("SelExtLeft"),    _("Selection Extend Left\tShift+Left"),               FN(OnSelExtendLeft));
   c->AddCommand(wxT("SelExtRight"),   _("Selection Extend Right\tShift+Right"),             FN(OnSelExtendRight));

   c->AddCommand(wxT("SelCntrLeft"),   _("Selection Contract Left\tCtrl+Shift+Right"),       FN(OnSelContractLeft));
   c->AddCommand(wxT("SelCntrRight"),  _("Selection Contract Right\tCtrl+Shift+Left"),       FN(OnSelContractRight));

   c->AddCommand(wxT("TrackPan"),      _("Change pan on focused track\tShift+P"),            FN(OnTrackPan));
   c->AddCommand(wxT("TrackPanLeft"),  _("Pan left on focused track\tAlt+Shift+Left"),       FN(OnTrackPanLeft));
   c->AddCommand(wxT("TrackPanRight"), _("Pan right on focused track\tAlt+Shift+Right"),     FN(OnTrackPanRight));
   c->AddCommand(wxT("TrackGain"),     _("Change gain on focused track\tShift+G"),           FN(OnTrackGain));
   c->AddCommand(wxT("TrackGainInc"),  _("Increase gain on focused track\tAlt+Shift+Up"),    FN(OnTrackGainInc));
   c->AddCommand(wxT("TrackGainDec"),  _("Decrease gain on focused track\tAlt+Shift+Down"),  FN(OnTrackGainDec));
   c->AddCommand(wxT("TrackMenu"),     _("Open menu on focused track\tShift+M"),             FN(OnTrackMenu));
   c->AddCommand(wxT("TrackMute"),     _("Mute/Unmute focused track\tShift+U"),              FN(OnTrackMute));
   c->AddCommand(wxT("TrackSolo"),     _("Solo/Unsolo focused track\tShift+S"),              FN(OnTrackSolo));
   c->AddCommand(wxT("TrackClose"),    _("Close focused track\tShift+C"),                    FN(OnTrackClose));

   mLastFlags = 0;

   mSel0save = 0;
   mSel1save = 0;
}

void AudacityProject::CreateRecentFilesMenu(CommandManager *c)
{
   // Recent Files and Recent Projects menus
   
   #ifdef __WXMAC__
      /* i18n-hint: This is the name of the menu item on Mac OS X only */
      wxMenu* pm = c->BeginSubMenu(_("Open Recent"));
   #else
      /* i18n-hint: This is the name of the menu item on Windows and Linux */
      wxMenu* pm = c->BeginSubMenu(_("Recent &Files..."));
   #endif

   c->EndSubMenu();
   // TODO - read the number of files to store in history from preferences
   mRecentFiles = new wxFileHistory();
   mRecentFiles->UseMenu(pm);
   gPrefs->SetPath(wxT("/RecentFiles"));
   mRecentFiles->Load(*gPrefs);
   gPrefs->SetPath(wxT(".."));

   c->AddSeparator();
}

// TIDY-ME: Replace indices by function pointers.
// This function is currently (July 2005) required because
// menus get to effect functions in a messy way.
// It would be much cleaner if each menu item already
// stored both the function pointer and the class pointer.
// so menus, and other dispatching, can dispatch to 'any' 
// function in 'any' class instance directly,
// rather than having to look up the class via an index
// when calling an effect.
void AudacityProject::ResolveEffectIndices(EffectArray *effects)
{
   for (unsigned int i = 0; i < effects->GetCount(); i++) {
      wxString effectName = (*effects)[i]->GetEffectName();
      if (effectName == wxT("Normalize...")) {
         mNormalizeIndex = i;
      }
      else if (effectName == wxT("Stereo To Mono")) {
         mStereoToMonoIndex = i;
      }
   }
}

void AudacityProject::ModifyExportMenus()
{
   int format = ReadExportFormatPref();
   wxString pcmFormat = sf_header_shortname(format & SF_FORMAT_TYPEMASK);

   mCommandManager.Modify(wxT("Export"),
                          wxString::Format(_("&%s..."),
                                           pcmFormat.c_str()));
   mCommandManager.Modify(wxT("ExportSel"),
                          wxString::Format(_("&%s..."),
                                           pcmFormat.c_str()));
}

void AudacityProject::ModifyUndoMenus()
{
   wxString desc;
   int cur = mUndoManager.GetCurrentState();

   if (mUndoManager.UndoAvailable()) {
      mUndoManager.GetShortDescription(cur, &desc);
      mCommandManager.Modify(wxT("Undo"),
                             wxString::Format(_("&Undo %s"),
                                              desc.c_str()));
      // LL:  Hackage Alert!!!
      //
      // On the Mac, all menu state changes are ignored if a modal
      // dialog is displayed.
      //
      // An example of this is when applying chains where the "Undo"
      // menu state should change when each command executes.  But,
      // since the state changes are ignored, the "Undo" menu item
      // will never get enabled.  And unfortunately, this will cause
      // the menu item to be permanently disabled since the recorded
      // state is enabled (even though it isn't) causing the routines
      // to ignore the new enable request.
      //
      // So, the workaround is to transition the item back to disabled
      // and then to enabled.  (Sorry, I couldn't find a better way of
      // doing it.)
      //
      // See src/mac/carbon/menuitem.cpp, wxMenuItem::Enable() for more
      // info.
      mCommandManager.Enable(wxT("Undo"), false);
      mCommandManager.Enable(wxT("Undo"), true);
   }
   else {
      mCommandManager.Modify(wxT("Undo"), 
                             wxString::Format(_("Can't Undo")));
      // LL: See above
      mCommandManager.Enable(wxT("Undo"), true);
      mCommandManager.Enable(wxT("Undo"), false);
   }

   if (mUndoManager.RedoAvailable()) {
      mUndoManager.GetShortDescription(cur+1, &desc);
      mCommandManager.Modify(wxT("Redo"),
                             wxString::Format(_("&Redo %s"),
                                              desc.c_str()));
      mCommandManager.Enable(wxT("Redo"), true);
   }
   else {
      mCommandManager.Modify(wxT("Redo"),
                             wxString::Format(_("Can't Redo")));
      mCommandManager.Enable(wxT("Redo"), false);
   }
}

void AudacityProject::RebuildMenuBar()
{

// Under Windows we delete the menus, since we will soon recreate them.
// rather oddly, the menus don't vanish as a result of doing this.
// Under Linux we can't delete them as this crashes gtk2....
// FIXME: So we have a memory leak of menu items under linux?  Oops.  
#ifdef __WXMSW__
   wxMenuBar *menuBar = GetMenuBar();

   // msmeyer: The following two lines make gtk2 crash on Linux
   DetachMenuBar();
   delete menuBar;
#endif

   /*
   // msmeyer: This also makes gtk2 crash on Linux
   for (int i = menuBar->GetMenuCount()-1; i >= 0; i--)
      delete menuBar->Remove(i);

   // msmeyer: However, this doesn't seem to matter, because CommandManager
   // knows how to properly rebuild menus, even when the menu bar is already
   // populated. So we just don't mess with the menus at this stage.
   */
  
   mCommandManager.PurgeData();
   delete mRecentFiles;
   mRecentFiles = NULL;

   CreateMenusAndCommands();
}

int AudacityProject::GetFocusedFrame()
{
   wxWindow *w = FindFocus();

   while (w) {
      if (w == mToolManager->GetTopDock()) {
         return TopDockHasFocus;
      }

      if (w == mTrackPanel) {
         return TrackPanelHasFocus;
      }

      if (w == mToolManager->GetBotDock()) {
         return BotDockHasFocus;
      }

      w = w->GetParent();
   }

   return 0;
}

wxUint32 AudacityProject::GetUpdateFlags()
{
   // This method determines all of the flags that determine whether
   // certain menu items and commands should be enabled or disabled,
   // and returns them in a bitfield.  Note that if none of the flags
   // have changed, it's not necessary to even check for updates.
   wxUint32 flags = 0;

   if (GetAudioIOToken() == 0 ||
       !gAudioIO->IsAudioTokenActive(GetAudioIOToken()))
      flags |= AudioIONotBusyFlag;
   else
      flags |= AudioIOBusyFlag;

   if (mViewInfo.sel1 > mViewInfo.sel0)
      flags |= TimeSelectedFlag;

   TrackListIterator iter(mTracks);
   Track *t = iter.First();
   while (t) {
      flags |= TracksExistFlag;
      if (t->GetKind() == Track::Label)
      {
         flags |= LabelTracksExistFlag;
         if( t->GetSelected() )
         {
            flags |= TracksSelectedFlag;

            LabelTrack *lt = ( LabelTrack* )t; 
            for( int i = 0; i < lt->GetNumLabels(); i++ ) 
            {
               const LabelStruct *ls = lt->GetLabel( i );
               if( ls->t >= mViewInfo.sel0 && ls->t1 <= mViewInfo.sel1 )
               {
                  flags |= LabelsSelectedFlag;
                  break;
               }
            }

            if (((LabelTrack *)t)->IsTextClipSupported())
               flags |= TextClipFlag;
         }
      }
      else if (t->GetKind() == Track::Wave) {
         if (t->GetSelected()) {
            flags |= TracksSelectedFlag;
            if (t->GetLinked()) {
               flags |= StereoRequiredFlag;
            }
            else {
               flags |= WaveTracksSelectedFlag;
            }
         }
      }
      t = iter.Next();
   }

   if(msClipLen > 0.0)
      flags |= ClipboardFlag;

   if (mUndoManager.UnsavedChanges())
      flags |= UnsavedChangesFlag;

   if (Effect::GetLastEffect() != NULL)
      flags |= HasLastEffectFlag;

   if (mUndoManager.UndoAvailable())
      flags |= UndoAvailableFlag;

   if (mUndoManager.RedoAvailable())
      flags |= RedoAvailableFlag;

   if (GetZoom() < gMaxZoom && (flags & TracksExistFlag))
      flags |= ZoomInAvailableFlag;

   if (GetZoom() > gMinZoom && (flags & TracksExistFlag))
      flags |= ZoomOutAvailableFlag;

   flags |= GetFocusedFrame();
   
   if (IsPlayRegionLocked())
      flags |= PlayRegionLockedFlag;
   else
      flags |= PlayRegionNotLockedFlag;

   return flags;
}

void AudacityProject::SelectAllIfNone()
{
   wxUint32 flags = GetUpdateFlags();
   if((flags & TracksSelectedFlag) ==0)
      OnSelectAll();
}


void AudacityProject::ModifyToolbarMenus()
{
   mCommandManager.Modify(wxT("ShowControlTB"),
                          mToolManager->IsVisible(ControlBarID ) ?
                          _("Hide Control Toolbar") :
                          _("Show Control Toolbar"));
   mCommandManager.Modify(wxT("ShowDeviceTB"),
                          mToolManager->IsVisible(DeviceBarID) ?
                          _("Hide Device Toolbar") :
                          _("Show Device Toolbar"));
   mCommandManager.Modify(wxT("ShowEditTB"),
                          mToolManager->IsVisible(EditBarID) ?
                          _("Hide Edit Toolbar") :
                          _("Show Edit Toolbar"));
   mCommandManager.Modify(wxT("ShowMeterTB"),
                          mToolManager->IsVisible(MeterBarID) ?
                          _("Hide Meter Toolbar") :
                          _("Show Meter Toolbar"));
   mCommandManager.Modify(wxT("ShowMixerTB"),
                          mToolManager->IsVisible(MixerBarID) ?
                          _("Hide Mixer Toolbar") :
                          _("Show Mixer Toolbar"));
   mCommandManager.Modify(wxT("ShowSelectionTB"),
                          mToolManager->IsVisible(SelectionBarID) ?
                          _("Hide Selection Toolbar") :
                          _("Show Selection Toolbar"));
   mCommandManager.Modify(wxT("ShowToolsTB"),
                          mToolManager->IsVisible(ToolsBarID) ?
                          _("Hide Tools Toolbar") :
                          _("Show Tools Toolbar"));
   mCommandManager.Modify(wxT("ShowTranscriptionTB"),
                          mToolManager->IsVisible(TranscriptionBarID) ?
                          _("Hide Transcription Toolbar") :
                          _("Show Transcription Toolbar"));
 }

void AudacityProject::UpdateMenus()
{
	//ANSWER-ME: Why UpdateMenus only does active project?
	//JKC: Is this test fixing a bug when multiple projects are open?
	//so that menu states work even when different in different projects?
   if (this != GetActiveProject())
      return;

   if (!IsActive())
      return;

   // Return from this function if nothing's changed since
   // the last time we were here.
   wxUint32 flags = GetUpdateFlags();
   if (flags == mLastFlags)
      return;

   mLastFlags = flags;
   mCommandManager.EnableUsingFlags(flags, 0xFFFFFFFF);

   ModifyToolbarMenus();

   // Now, go through each toolbar, and call EnableDisableButtons()
   for( int i = 0; i < ToolBarCount; i++ )
   {
      mToolManager->GetToolBar( i )->EnableDisableButtons();
   }
}

//
// Tool selection commands
//

void AudacityProject::SetTool(int tool)
{
   ToolsToolBar *toolbar = GetToolsToolBar();
   if (toolbar) {
      toolbar->SetCurrentTool(tool, true);
      mTrackPanel->Refresh(false);
   }
}

void AudacityProject::OnSelectTool()
{
   SetTool(selectTool);
}

void AudacityProject::OnZoomTool()
{
   SetTool(zoomTool);
}

void AudacityProject::OnEnvelopeTool()
{
   SetTool(envelopeTool);
}

void AudacityProject::OnTimeShiftTool()
{
   SetTool(slideTool);
}

void AudacityProject::OnDrawTool()
{
   SetTool(drawTool);
}

void AudacityProject::OnMultiTool()
{
   SetTool(multiTool);
}


void AudacityProject::OnNextTool()
{
   ToolsToolBar *toolbar = GetToolsToolBar();
   if (toolbar) {
      // Use GetDownTool() here since GetCurrentTool() can return a value that
      // doesn't represent the real tool if the Multi-tool is being used.
      toolbar->SetCurrentTool((toolbar->GetDownTool()+1)%numTools, true);
      mTrackPanel->Refresh(false);
   }
}

void AudacityProject::OnPrevTool()
{
   ToolsToolBar *toolbar = GetToolsToolBar();
   if (toolbar) {
      // Use GetDownTool() here since GetCurrentTool() can return a value that
      // doesn't represent the real tool if the Multi-tool is being used.
      toolbar->SetCurrentTool((toolbar->GetDownTool()+(numTools-1))%numTools, true);
      mTrackPanel->Refresh(false);
   }
}


//
// Audio I/O Commands
//

// TODO: Should all these functions which involve
// the toolbar actually move into ControlToolBar?

/// MakeReadyToPlay stops whatever is currently playing 
/// and pops the play button up.  Then, if nothing is now
/// playing, it pushes the play button down and enables
/// the stop button.
bool AudacityProject::MakeReadyToPlay()
{
   ControlToolBar *toolbar = GetControlToolBar();
   wxCommandEvent evt;

   // If this project is playing, stop playing
   if (gAudioIO->IsStreamActive(GetAudioIOToken())) {
      toolbar->SetPlay(false);        //Pops
      toolbar->SetStop(true);         //Pushes stop down
      toolbar->OnStop(evt);

      ::wxMilliSleep(100);
   }

   // If it didn't stop playing quickly, or if some other
   // project is playing, return
   if (gAudioIO->IsBusy())
      return false;

   toolbar->SetPlay(true);
   toolbar->SetStop(false);

   return true;
}

void AudacityProject::OnPlayOneSecond()
{
   if( !MakeReadyToPlay() )
      return;

   double pos = mTrackPanel->GetMostRecentXPos();
   mLastPlayMode = oneSecondPlay;
   GetControlToolBar()->PlayPlayRegion(pos - 0.5, pos + 0.5);
}


/// The idea for this function (and first implementation)
/// was from Juhana Sadeharju.  The function plays the 
/// sound between the current mouse position and the 
/// nearest selection boundary.  This gives four possible 
/// play regions depending on where the current mouse 
/// position is relative to the left and right boundaries 
/// of the selection region.
void AudacityProject::OnPlayToSelection()
{
   if( !MakeReadyToPlay() )
      return;

   double pos = mTrackPanel->GetMostRecentXPos();

   double t0,t1;
   // check region between pointer and the nearest selection edge
   if (fabs(pos - mViewInfo.sel0) < fabs(pos - mViewInfo.sel1)) {
      t0 = t1 = mViewInfo.sel0;
   } else {
      t0 = t1 = mViewInfo.sel1;
   }
   if( pos < t1) 
      t0=pos;
   else
      t1=pos;

   // JKC: oneSecondPlay mode disables auto scrolling
   // On balance I think we should always do this in this function
   // since you are typically interested in the sound EXACTLY 
   // where the cursor is.
   // TODO: have 'playing attributes' such as 'with_autoscroll'
   // rather than modes, since that's how we're now using the modes.
   mLastPlayMode = oneSecondPlay;

   // An alternative, commented out below, is to disable autoscroll
   // only when playing a short region, less than or equal to a second.
//   mLastPlayMode = ((t1-t0) > 1.0) ? normalPlay : oneSecondPlay;

   GetControlToolBar()->PlayPlayRegion(t0, t1);
}

void AudacityProject::OnPlayLooped()
{
   if( !MakeReadyToPlay() )
      return;

   // Now play in a loop
   // Will automatically set mLastPlayMode
   GetControlToolBar()->PlayCurrentRegion(true);
}

void AudacityProject::OnPlayCutPreview()
{
   if ( !MakeReadyToPlay() )
      return;
      
   // Play with cut preview
   GetControlToolBar()->PlayCurrentRegion(false, true);
}
   
void AudacityProject::OnPlayStop()
{
   ControlToolBar *toolbar = GetControlToolBar();

   //If busy, stop playing, make sure everything is unpaused.
   if (gAudioIO->IsStreamActive(GetAudioIOToken())) {
      toolbar->SetPlay(false);        //Pops
      toolbar->SetStop(true);         //Pushes stop down
      toolbar->StopPlaying();
   }
   else if (!gAudioIO->IsBusy()) {
      //Otherwise, start playing (assuming audio I/O isn't busy)
      toolbar->SetPlay(true);
      toolbar->SetStop(false);

      // Will automatically set mLastPlayMode
      toolbar->PlayCurrentRegion(false);
   }
}

void AudacityProject::OnStop()
{
   wxCommandEvent evt;

   if (gAudioIO->IsStreamActive())
      GetControlToolBar()->OnStop(evt);
}

void AudacityProject::OnPause()
{
   wxCommandEvent evt;

   GetControlToolBar()->OnPause(evt);
}

void AudacityProject::OnRecord()
{
   wxCommandEvent evt;

   GetControlToolBar()->OnRecord(evt);
}

void AudacityProject::OnStopSelect()
{
   wxCommandEvent evt;

   if (gAudioIO->IsStreamActive()) {
      mViewInfo.sel0 = gAudioIO->GetStreamTime();
      if( mViewInfo.sel1 < mViewInfo.sel0 ) {
         mViewInfo.sel1 = mViewInfo.sel0;
      }
      GetControlToolBar()->OnStop(evt);
   }

   ModifyState();
}

double AudacityProject::GetTime(Track *t)
{
   double stime = 0.0;

   if (t->GetKind() == Track::Wave) {
      WaveTrack *w = (WaveTrack *)t;
      stime = w->GetEndTime();

      WaveClip *c;
      int ndx;
      for (ndx = 0; ndx < w->GetNumClips(); ndx++) {
         c = w->GetClipByIndex(ndx);
         if (c->GetNumSamples() == 0)
            continue;
         if (c->GetStartTime() < stime) {
            stime = c->GetStartTime();
         }
      }
   }
   else if (t->GetKind() == Track::Label) {
      LabelTrack *l = (LabelTrack *)t;
      stime = l->GetStartTime();
   }

   return stime;
}

void AudacityProject::OnSortTime()
{
   int ndx;

   wxArrayPtrVoid warr, marr;
   TrackListIterator iter(mTracks);
   Track *track = iter.First();
   while (track) {
      if (track->GetKind() == Track::Wave) {
         for (ndx = 0; ndx < (int)warr.GetCount(); ndx++) {
            if (GetTime(track) < GetTime((Track *) warr[ndx])) {
               break;
            }
         }
         warr.Insert(track, ndx);
      }
      else {
         for (ndx = 0; ndx < (int)marr.GetCount(); ndx++) {
            if (GetTime(track) < GetTime((Track *) marr[ndx])) {
               break;
            }
         }
         marr.Insert(track, ndx);
      }
      track = iter.RemoveCurrent();
   }

   for (ndx = 0; ndx < (int)marr.GetCount(); ndx++) {
      mTracks->Add((Track *)marr[ndx]);
   }

   for (ndx = 0; ndx < (int)warr.GetCount(); ndx++) {
      mTracks->Add((Track *)warr[ndx]);
   }

   PushState(_("Tracks sorted by time"), _("Sort By Time"));

   mTrackPanel->Refresh(false);
}

void AudacityProject::OnSortName()
{
   int ndx;

   wxArrayPtrVoid arr;
   TrackListIterator iter(mTracks);
   Track *track = iter.First();
   while (track) {
      for (ndx = 0; ndx < (int)arr.GetCount(); ndx++) {
         if (track->GetName() < ((Track *) arr[ndx])->GetName()) {
            break;
         }
      }
      arr.Insert(track, ndx);
      track = iter.RemoveCurrent();
   }

   for (ndx = 0; ndx < (int)arr.GetCount(); ndx++) {
      mTracks->Add((Track *)arr[ndx]);
   }

   PushState(_("Tracks sorted by name"), _("Sort By Name"));

   mTrackPanel->Refresh(false);
}

void AudacityProject::OnSkipStart()
{
   wxCommandEvent evt;

   GetControlToolBar()->OnRewind(evt);
}

void AudacityProject::OnSkipEnd()
{
   wxCommandEvent evt;

   GetControlToolBar()->OnFF(evt);
}

void AudacityProject::OnSeekLeftShort()
{
   mTrackPanel->OnCursorLeft( false, false );
}

void AudacityProject::OnSeekRightShort()
{
   mTrackPanel->OnCursorRight( false, false );
}

void AudacityProject::OnSeekLeftLong()
{
   mTrackPanel->OnCursorLeft( true, false );
}

void AudacityProject::OnSeekRightLong()
{
   mTrackPanel->OnCursorRight( true, false );
}

void AudacityProject::OnSelToStart()
{
   mViewInfo.sel0 = 0;
   mTrackPanel->Refresh(false);

   ModifyState();
}

void AudacityProject::OnSelToEnd()
{
   SkipEnd(true);
}

void AudacityProject::OnCursorUp()
{
   mTrackPanel->OnPrevTrack( false );
}

void AudacityProject::OnShiftUp()
{
   mTrackPanel->OnPrevTrack( true );
}

void AudacityProject::OnCursorDown()
{
   mTrackPanel->OnNextTrack( false );
}

void AudacityProject::OnShiftDown()
{
   mTrackPanel->OnNextTrack( true );
}

void AudacityProject::OnToggle()
{
   mTrackPanel->OnToggle( );
}

void AudacityProject::OnCursorLeft()
{
   mTrackPanel->OnCursorLeft( false, false );
}

void AudacityProject::OnCursorRight()
{
   mTrackPanel->OnCursorRight( false, false );
}

void AudacityProject::OnSelExtendLeft()
{
   mTrackPanel->OnCursorLeft( true, false );
}

void AudacityProject::OnSelExtendRight()
{
   mTrackPanel->OnCursorRight( true, false );
}

void AudacityProject::OnSelContractLeft()
{
   mTrackPanel->OnCursorRight( true, true );
}

void AudacityProject::OnSelContractRight()
{
   mTrackPanel->OnCursorLeft( true, true );
}

//this pops up a dialog which allows the left selection to be set.
//If playing/recording is happening, it sets the left selection at
//the current play position.
void AudacityProject::OnSetLeftSelection()
{
   if (GetAudioIOToken()>0 &&
       gAudioIO->IsStreamActive(GetAudioIOToken()))
      {
         double indicator = gAudioIO->GetStreamTime();
         mViewInfo.sel0 = indicator;
      }
   else
      {
         wxString fmt = gPrefs->Read(wxT("/SelectionFormat"), wxT(""));

         TimeDialog D(this, wxID_ANY, _("Set Left Selection Bound"));
         D.SetFormatString(fmt);
         if(wxID_OK==D.ShowModal() )
            {
               //Get the value from the dialog
               mViewInfo.sel0 = D.GetTimeValue();
               
               //Make sure it is 'legal'
               if(mViewInfo.sel0 < 0.0)
                  mViewInfo.sel0 = 0.0;
            }
      }
   
   if(mViewInfo.sel1 < mViewInfo.sel0)
      mViewInfo.sel1 = mViewInfo.sel0;

   ModifyState();

   mTrackPanel->Refresh(false);
}


void AudacityProject::OnSetRightSelection()
{
   if (GetAudioIOToken()>0 &&
       gAudioIO->IsStreamActive(GetAudioIOToken())) 
      {
         double indicator = gAudioIO->GetStreamTime();
         mViewInfo.sel1 = indicator;
      }
   else
      {
         wxString fmt = gPrefs->Read(wxT("/SelectionFormat"), wxT(""));

         TimeDialog D(this, wxID_ANY, _("Set Right Selection Bound"));
         D.SetFormatString(fmt);
         if(wxID_OK==D.ShowModal() )
            {
               //Get the value from the dialog
               mViewInfo.sel1 = D.GetTimeValue();
               
               //Make sure it is 'legal'
               if(mViewInfo.sel1 < 0)
                  mViewInfo.sel1 = 0;
            }
      }

   if(mViewInfo.sel0 >  mViewInfo.sel1)
      mViewInfo.sel0 = mViewInfo.sel1;

   ModifyState();
   
   mTrackPanel->Refresh(false);
}

void AudacityProject::NextFrame()
{
   switch( GetFocusedFrame() )
   {
      case TopDockHasFocus:
         mTrackPanel->SetFocus();
      break;

      case TrackPanelHasFocus:
         mToolManager->GetBotDock()->SetFocus();
      break;

      case BotDockHasFocus:
         mToolManager->GetTopDock()->SetFocus();
      break;
   }
}

void AudacityProject::PrevFrame()
{
   switch( GetFocusedFrame() )
   {
      case TopDockHasFocus:
         mToolManager->GetBotDock()->SetFocus();
      break;

      case TrackPanelHasFocus:
         mToolManager->GetTopDock()->SetFocus();
      break;

      case BotDockHasFocus:
         mTrackPanel->SetFocus();
      break;
   }
}

void AudacityProject::OnTrackPan()
{
   mTrackPanel->OnTrackPan();
}

void AudacityProject::OnTrackPanLeft()
{
   mTrackPanel->OnTrackPanLeft();
}

void AudacityProject::OnTrackPanRight()
{
   mTrackPanel->OnTrackPanRight();
}

void AudacityProject::OnTrackGain()
{
   mTrackPanel->OnTrackGain();
}

void AudacityProject::OnTrackGainInc()
{
   mTrackPanel->OnTrackGainInc();
}

void AudacityProject::OnTrackGainDec()
{
   mTrackPanel->OnTrackGainDec();
}

void AudacityProject::OnTrackMenu()
{
   mTrackPanel->OnTrackMenu();
}

void AudacityProject::OnTrackMute()
{
   mTrackPanel->OnTrackMute(false);
}

void AudacityProject::OnTrackSolo()
{
   mTrackPanel->OnTrackSolo(false);
}

void AudacityProject::OnTrackClose()
{
   mTrackPanel->OnTrackClose();
}


double AudacityProject::NearestZeroCrossing(double t0)
{
   int windowSize = (int)(GetRate() / 100);
   float *dist = new float[windowSize];
   int i, j;

   for(i=0; i<windowSize; i++)
      dist[i] = 0.0;

   TrackListIterator iter(mTracks);
   Track *track = iter.First();
   while (track) {
      if (!track->GetSelected() || track->GetKind() != (Track::Wave)) {
         track = iter.Next();
         continue;
      }
      WaveTrack *one = (WaveTrack *)track;
      int oneWindowSize = (int)(one->GetRate() / 100);
      float *oneDist = new float[oneWindowSize];
      longSampleCount s = one->TimeToLongSamples(t0);
      one->Get((samplePtr)oneDist, floatSample,
               s - oneWindowSize/2, oneWindowSize);

      // Start by penalizing downward motion.  We prefer upward
      // zero crossings.
      if (oneDist[1] - oneDist[0] < 0)
         oneDist[0] = oneDist[0]*6 + 0.3;
      for(i=1; i<oneWindowSize; i++)
         if (oneDist[i] - oneDist[i-1] < 0)
            oneDist[i] = oneDist[i]*6 + 0.3;

      for(i=0; i<oneWindowSize; i++)
         oneDist[i] = fabs(oneDist[i]);

      for(i=0; i<windowSize; i++) {
         if (windowSize != oneWindowSize)
            j = i * (oneWindowSize-1) / (windowSize-1);
         else
            j = i;

         dist[i] += oneDist[j];
      }
         
      track = iter.Next();
   }

   int argmin = windowSize/2; // Start at default pos in center
   float min = dist[argmin];
   for(i=0; i<windowSize; i++) {
      if (dist[i] < min) {
         argmin = i;
         min = dist[i];
      }
   }

   return t0 + (argmin - windowSize/2)/GetRate();
}

void AudacityProject::OnZeroCrossing()
{
   if (mViewInfo.sel0 == mViewInfo.sel1)
      mViewInfo.sel0 = mViewInfo.sel1 =
         NearestZeroCrossing(mViewInfo.sel0);
   else {
      mViewInfo.sel0 = NearestZeroCrossing(mViewInfo.sel0);
      mViewInfo.sel1 = NearestZeroCrossing(mViewInfo.sel1);

      if (mViewInfo.sel1 < mViewInfo.sel0)
         mViewInfo.sel1 = mViewInfo.sel0;
   }

   ModifyState();

   mTrackPanel->Refresh(false);
}

//
// Effect Menus
//

void AudacityProject::OnEffect(int type, int index)
{
   EffectArray *effects;
   Effect *f = NULL;

   effects = Effect::GetEffects(type);
   f = (*effects)[index];
   delete effects;

   if (!f)
      return;
   //TIDY-ME: Effect Type parameters serve double duty.
   // The type parameter is over used.
   // It is being used:
   //  (a) to filter the list of effects
   //  (b) to specify whether to prompt for parameters.
   OnEffect( type, f );
}

/// OnEffect() takes an Effect and executes it.
///
/// At the moment flags are used only to indicate 
/// whether to prompt for parameters or whether to
/// use the most recently stored values.
///
/// At some point we should change to specifying a 
/// parameter source - one of:
///   + Prompt
///   + Use previous values
///   + Parse from a string that is passed in
bool AudacityProject::OnEffect(int type, Effect * f)
{
   TrackListIterator iter(mTracks);
   Track *t = iter.First();
   WaveTrack *newTrack = NULL;

   //double prevEndTime = mTracks->GetEndTime();
   int count = 0;
   
   while (t) {
      if (t->GetSelected() && t->GetKind() == (Track::Wave))
         count++;
      t = iter.Next();
   }

   if (count == 0) {
      // No tracks were selected...
      if (f->GetEffectFlags() & INSERT_EFFECT) {
         // Create a new track for the generated audio...
         newTrack = mTrackFactory->NewWaveTrack();
         mTracks->Add(newTrack);
         newTrack->SetSelected(true);
      }
      else {
         wxMessageBox(_("You must select a track first."));
         return false;
      }
   }
   
   if (f->DoEffect(this, type, mRate, mTracks, mTrackFactory,
                   &mViewInfo.sel0, &mViewInfo.sel1)) {
      wxString longDesc = f->GetEffectDescription();
      wxString shortDesc = f->GetEffectName();

      if (shortDesc.Length() > 3 && shortDesc.Right(3)==wxT("..."))
         shortDesc = shortDesc.Left(shortDesc.Length()-3);

      PushState(longDesc, shortDesc);
      
      //STM:
      //The following automatically re-zooms after sound was generated.
      // IMO, it was disorienting, removing to try out without re-fitting
      //if (mTracks->GetEndTime() > prevEndTime)
      //      OnZoomFit();

      RedrawProject();
      mTrackPanel->EnsureVisible(mTrackPanel->GetFirstSelectedTrack());

      // Only remember a successful effect, don't rmemeber insert,
      // or analyze effects.
      if ((f->GetEffectFlags() & (INSERT_EFFECT | ANALYZE_EFFECT))==0) {
         Effect::SetLastEffect( type, f );
         mCommandManager.Modify(wxT("RepeatLastEffect"),
            wxString::Format(_("Repeat %s\tCtrl+R"),
            shortDesc.c_str()));
      }
      
      mTrackPanel->Refresh(false);
   } else {
      if (newTrack) {
         mTracks->Remove(newTrack);
         mTrackPanel->Refresh(false);
      }
      return false;
   }
   return true;
}

void AudacityProject::OnGenerateEffect(int index)
{
   OnEffect(BUILTIN_EFFECT | INSERT_EFFECT, index);
}

void AudacityProject::OnGeneratePlugin(int index)
{
   OnEffect(PLUGIN_EFFECT | INSERT_EFFECT, index);
}

void AudacityProject::OnRepeatLastEffect(int index)
{
   Effect *f = Effect::GetLastEffect();
   if( f  != NULL )
   {
      // Setting the CONFIGURED_EFFECT bit prevents
      // prompting for parameters.
      OnEffect( 
         Effect::GetLastEffectType() | CONFIGURED_EFFECT,
         f);
   }
}

void AudacityProject::OnProcessEffect(int index)
{
   int additionalEffects=ADVANCED_EFFECT;
   // set additionalEffects to zero to exclude the advanced effects.
   if( mCleanSpeechMode )
       additionalEffects = 0;
   OnEffect(BUILTIN_EFFECT | PROCESS_EFFECT | additionalEffects, index);
}

void AudacityProject::OnStereoToMono(int index)
{
   OnEffect(ALL_EFFECTS, mStereoToMonoIndex);
}

void AudacityProject::OnProcessPlugin(int index)
{
   OnEffect(PLUGIN_EFFECT | PROCESS_EFFECT, index);
}

void AudacityProject::OnAnalyzeEffect(int index)
{
   OnEffect(BUILTIN_EFFECT | ANALYZE_EFFECT, index);
}

void AudacityProject::OnAnalyzePlugin(int index)
{
   OnEffect(PLUGIN_EFFECT | ANALYZE_EFFECT, index);
}

//
// File Menu
//

void AudacityProject::OnNew()
{
   CreateNewAudacityProject(gParentWindow);
}

void AudacityProject::OnOpen()
{
   ShowOpenDialog(this);
}

void AudacityProject::OnClose()
{
   Close();
}

void AudacityProject::OnSave()
{
   Save();
}

void AudacityProject::OnSaveAs()
{
   SaveAs();
}

void AudacityProject::OnCheckDependencies()
{
   ShowDependencyDialogIfNeeded(this, false);
}

void AudacityProject::OnExit()
{
   QuitAudacity();
}

void AudacityProject::OnUpload()
{
   //if (mTags->ShowEditDialog(this, _("Edit ID3 Tags (for MP3 exporting)")))
   //   PushState(_("Edit ID3 tags"), _("Edit ID3 Tags"));

   UploadDialog dlog(this);
   dlog.ShowModal();
}

void AudacityProject::OnExportLabels()
{
   Track *t;
   int numLabelTracks = 0;

   TrackListIterator iter(mTracks);

   t = iter.First();
   while (t) {
      if (t->GetKind() == Track::Label)
         numLabelTracks++;
      t = iter.Next();
   }

   if (numLabelTracks == 0) {
      wxMessageBox(_("There are no label tracks to export."));
      return;
   }

   wxString fName = _("labels.txt");

   fName = wxFileSelector(_("Export Labels As:"),
                          NULL,
                          fName,
                          wxT("txt"),
                          wxT("*.txt"), wxSAVE | wxOVERWRITE_PROMPT, this);

   if (fName == wxT(""))
      return;

   // Move existing files out of the way.  Otherwise wxTextFile will
   // append to (rather than replace) the current file.

   if (wxFileExists(fName)) {
#ifdef __WXGTK__
      wxString safetyFileName = fName + wxT("~");
#else
      wxString safetyFileName = fName + wxT(".bak");
#endif

      if (wxFileExists(safetyFileName))
         wxRemoveFile(safetyFileName);

      wxRename(fName, safetyFileName);
   }

   wxTextFile f(fName);
#ifdef __WXMAC__
   wxFile *temp = new wxFile();
   temp->Create(fName);
   delete temp;
#else
   f.Create();
#endif
   f.Open();
   if (!f.IsOpened()) {
      wxMessageBox(_("Couldn't write to file: ") + fName);
      return;
   }

   t = iter.First();
   while (t) {
      if (t->GetKind() == Track::Label)
         ((LabelTrack *) t)->Export(f);

      t = iter.Next();
   }

#ifdef __WXMAC__
   f.Write(wxTextFileType_Mac);
#else
   f.Write();
#endif
   f.Close();
}

void AudacityProject::OnExportMix()
{
   ::ExportPCM(this, false, 0.0, mTracks->GetEndTime());
}

void AudacityProject::OnExportSelection()
{
   ::ExportPCM(this, true, mViewInfo.sel0, mViewInfo.sel1);
}

void AudacityProject::OnExportMP3Mix()
{
   ::ExportCompressed(this, wxT("MP3"), false, 0.0, mTracks->GetEndTime());
}

void AudacityProject::OnExportMP3Selection()
{
   ::ExportCompressed(this, wxT("MP3"), true, mViewInfo.sel0, mViewInfo.sel1);
}

void AudacityProject::OnExportMP2Mix()
{
   ::ExportCompressed(this, wxT("MP2"), false, 0.0, mTracks->GetEndTime());
}

void AudacityProject::OnExportMP2Selection()
{
   ::ExportCompressed(this, wxT("MP2"), true, mViewInfo.sel0, mViewInfo.sel1);
}

void AudacityProject::OnExportOggMix()
{
   ::ExportCompressed(this, wxT("OGG"), false, 0.0, mTracks->GetEndTime());
}

void AudacityProject::OnExportFLACMix()
{
   ::ExportCompressed(this, wxT("FLAC"), false, 0.0, mTracks->GetEndTime());
}

void AudacityProject::OnExportOggSelection()
{
   ::ExportCompressed(this, wxT("OGG"), true, mViewInfo.sel0, mViewInfo.sel1);
}

void AudacityProject::OnExportFLACSelection()
{
   ::ExportCompressed(this, wxT("FLAC"), true, mViewInfo.sel0, mViewInfo.sel1);
}

void AudacityProject::OnExportMultiple()
{
   ::ExportMultiple(this);
}

void AudacityProject::OnPreferences()
{
   PrefsDialog dialog(this /* parent */ );
   dialog.ShowModal();
}

void AudacityProject::OnPageSetup()
{
   HandlePageSetup(this);
}

void AudacityProject::OnPrint()
{
   HandlePrint(this, GetName(), mTracks);
}

//
// Edit Menu
//

void AudacityProject::OnUndo()
{
   if (!mUndoManager.UndoAvailable()) {
      wxMessageBox(_("Nothing to undo"));
      return;
   }

   TrackList *l = mUndoManager.Undo(&mViewInfo.sel0, &mViewInfo.sel1);
   PopState(l);

   mTrackPanel->EnsureVisible(mTrackPanel->GetFirstSelectedTrack());

   RedrawProject();

   if (mHistoryWindow)
      mHistoryWindow->UpdateDisplay();

   ModifyUndoMenus();   
}

void AudacityProject::OnRedo()
{
   if (!mUndoManager.RedoAvailable()) {
      wxMessageBox(_("Nothing to redo"));
      return;
   }

   TrackList *l = mUndoManager.Redo(&mViewInfo.sel0, &mViewInfo.sel1);
   PopState(l);

   RedrawProject();

   if (mHistoryWindow)
      mHistoryWindow->UpdateDisplay();

   ModifyUndoMenus();
}

void AudacityProject::OnHistory()
{
   if (!mHistoryWindow)
      mHistoryWindow = new HistoryWindow(this, &mUndoManager);

   mHistoryWindow->Show(true);
   mHistoryWindow->UpdateDisplay();
}

void AudacityProject::OnExperimentalTrackPanel()
{
#ifdef EXPERIMENTAL_TRACK_PANEL
   // Parasite Track Panel.
   // Takes over the display...
   TrackPanel2 * pDlg = new TrackPanel2(NULL) ;
   wxTopLevelWindow * pOldWindow =  ((wxTopLevelWindow*)wxTheApp->GetTopWindow());
   pOldWindow->Show(false);
// Dlg.Maximize();
//   Dlg.ShowModal();

   wxTheApp->SetTopWindow(pDlg);
   pDlg->Show( true );
#define RETURN_TO_AUDACITY
#ifdef RETURN_TO_AUDACITY
   wxTheApp->SetTopWindow(pOldWindow);
   pOldWindow->Show(true);
//   ((wxTopLevelWindow*)wxTheApp->GetTopWindow())->Show(true);
#else
   QuitAudacity( false );
#endif
#endif
}


void AudacityProject::OnCut()
{
   TrackListIterator iter(mTracks);
   Track *n = iter.First();
   Track *dest;

   // This doesn't handle cutting labels, it handles
   // cutting the _text_ inside of labels, i.e. if you're
   // in the middle of editing the label text and select "Cut".

   while (n) {
      if (n->GetSelected()) {
         if (n->GetKind() == Track::Label) {
            if (((LabelTrack *)n)->CutSelectedText()) {
               mTrackPanel->Refresh(false);
               return;
            }
         }
      }
      n = iter.Next();
   }
   
   ClearClipboard();
   n = iter.First();
   while (n) {
      if (n->GetSelected()) {
         dest = NULL;
         if (n->GetKind() == Track::Wave && 
             gPrefs->Read(wxT("/GUI/EnableCutLines"), (long)0))
         {
            ((WaveTrack*)n)->CutAndAddCutLine(mViewInfo.sel0, mViewInfo.sel1, &dest);
         } else
         {
            n->Cut(mViewInfo.sel0, mViewInfo.sel1, &dest);
         }
         if (dest) {
            dest->SetChannel(n->GetChannel());
            dest->SetTeamed(n->GetTeamed()); // do first
            dest->SetLinked(n->GetLinked());
            dest->SetName(n->GetName());
            msClipboard->Add(dest);
         }
      }
      n = iter.Next();
   }

   msClipLen = (mViewInfo.sel1 - mViewInfo.sel0);
   msClipProject = this;

   mViewInfo.sel1 = mViewInfo.sel0;

   PushState(_("Cut to the clipboard"), _("Cut"));

   RedrawProject();
}


void AudacityProject::OnSplitCut()
{
   TrackListIterator iter(mTracks);
   Track *n = iter.First();
   Track *dest;

   ClearClipboard();
   n = iter.First();
   while (n) {
      if (n->GetSelected()) {
         dest = NULL;
         if (n->GetKind() == Track::Wave)
         {
            ((WaveTrack*)n)->SplitCut(mViewInfo.sel0, mViewInfo.sel1, &dest);
         } else
         {
            n->Cut(mViewInfo.sel0, mViewInfo.sel1, &dest);
         }
         if (dest) {
            dest->SetChannel(n->GetChannel());
            dest->SetTeamed(n->GetTeamed()); // do first
            dest->SetLinked(n->GetLinked());
            dest->SetName(n->GetName());
            msClipboard->Add(dest);
         }
      }
      n = iter.Next();
   }

   msClipLen = (mViewInfo.sel1 - mViewInfo.sel0);
   msClipProject = this;

   PushState(_("Split-cut to the clipboard"), _("Split Cut"));

   RedrawProject();
}


void AudacityProject::OnCopy()
{
  
   TrackListIterator iter(mTracks);

   Track *n = iter.First();
   Track *dest;

   while (n) {
      if (n->GetSelected()) {
         if (n->GetKind() == Track::Label) {
            if (((LabelTrack *)n)->CopySelectedText()) {
               //mTrackPanel->Refresh(false);
               return;
            }
         }
      }
      n = iter.Next();
   }

   ClearClipboard();
   n = iter.First();
   while (n) {
      if (n->GetSelected()) {
         dest = NULL;
         n->Copy(mViewInfo.sel0, mViewInfo.sel1, &dest);
         if (dest) {
            dest->SetChannel(n->GetChannel());
            dest->SetTeamed(n->GetTeamed()); // do first
            dest->SetLinked(n->GetLinked());
            dest->SetName(n->GetName());
            msClipboard->Add(dest);
         }
      }
      n = iter.Next();
   }

   msClipLen = (mViewInfo.sel1 - mViewInfo.sel0);
   msClipProject = this;
   
   //Make sure the menus/toolbar states get updated
   mTrackPanel->Refresh(false);
}

void AudacityProject::OnPaste()
{
   // If nothing's selected, we just insert new tracks...so first
   // check to see if anything's selected
   
   TrackListIterator iter2(mTracks);
   Track *countTrack = iter2.First();

   while (countTrack) {
      if (countTrack->GetSelected()) {
         if (countTrack->GetKind() == Track::Label) {
            if (((LabelTrack *)countTrack)->PasteSelectedText(mViewInfo.sel0, mViewInfo.sel1)) {
               PushState(_("Pasted text from the clipboard"), _("Paste"));
               RedrawProject();
               return;
            }
         }
      }
      countTrack = iter2.Next();
   }

   int numSelected =0;

   countTrack = iter2.First();
   while (countTrack) {
      if (countTrack->GetSelected())
         numSelected++;
      countTrack = iter2.Next();
   }
   
   if (numSelected == 0) {
      TrackListIterator clipIter(msClipboard);
      Track *c = clipIter.First();
      Track *n;

      while (c) {
         if (msClipProject != this && c->GetKind() == Track::Wave)
            ((WaveTrack *) c)->Lock();
         
         switch(c->GetKind()) {
         case Track::Wave: {
            WaveTrack *w = (WaveTrack *)c;
            n = mTrackFactory->NewWaveTrack(w->GetSampleFormat(), w->GetRate());
            } break;

         case Track::Note:
            n = mTrackFactory->NewNoteTrack();
            break;

         case Track::Label:
            n = mTrackFactory->NewLabelTrack();
            break;
            
         case Track::Time:
            n = mTrackFactory->NewTimeTrack();
            break;

         default:
            c = clipIter.Next();
            continue;
         }

         n->SetTeamed(c->GetTeamed()); // do first
         n->SetLinked(c->GetLinked());
         n->SetChannel(c->GetChannel());
         n->SetName(c->GetName());

         n->Paste(0.0, c);
         mTracks->Add(n);
         n->SetSelected(true);         
         
         if (msClipProject != this && c->GetKind() == Track::Wave)
            ((WaveTrack *) c)->Unlock();
         
         c = clipIter.Next();
      }

      mViewInfo.sel0 = 0.0;
      mViewInfo.sel1 = msClipLen;
      
      PushState(_("Pasted from the clipboard"), _("Paste"));
      
      RedrawProject();

      return;
   }

   // Otherwise, paste into the selected tracks.

   // This old logic is no longer necessary now that we have WaveClips:
   //
   double t0 = mViewInfo.sel0;
   double t1 = mViewInfo.sel1;

   TrackListIterator iter(mTracks);
   TrackListIterator clipIter(msClipboard);

   Track *n = iter.First();
   Track *c = clipIter.First();
   
   bool pastedSomething = false;

   while (n && c) {
      if (n->GetSelected()) {
         // When trying to copy from stereo to mono track, show error and exit
         // TODO: Automatically offer user to mix down to mono (unfortunately
         //       this is not easy to implement
         if (c->GetLinked() && !n->GetLinked())
         {
            wxMessageBox(
               _("Copying stereo audio into a mono track is not allowed."),
               _("Error"), wxICON_ERROR, this);
            break;
         }
         
         if (msClipProject != this && c->GetKind() == Track::Wave)
            ((WaveTrack *) c)->Lock();

         if (n->GetKind() == Track::Wave) {
            //printf("Checking to see if we need to pre-clear the track\n");
            if (!((WaveTrack *) n)->IsEmpty(t0, t1)) {
               ((WaveTrack *) n)->Clear(t0, t1);
            }
         }
         else {
            ((WaveTrack *) n)->Clear(t0, t1);
         }

         n->Paste(t0, c);
         
         pastedSomething = true;
         
         // When copying from mono to stereo track, paste the wave form
         // to both channels
         if (n->GetLinked() && !c->GetLinked())
         {
            n = iter.Next();
            
            if (n->GetKind() == Track::Wave) {
               //printf("Checking to see if we need to pre-clear the track\n");
               if (!((WaveTrack *) n)->IsEmpty(t0, t1)) {
                  ((WaveTrack *) n)->Clear(t0, t1);
               }
            }
            else {
               ((WaveTrack *) n)->Clear(t0, t1);
            }
            
            n->Paste(t0, c);
         }
         
         if (msClipProject != this && c->GetKind() == Track::Wave)
            ((WaveTrack *) c)->Unlock();

         c = clipIter.Next();
      }

      n = iter.Next();
   }

   // TODO: What if we clicked past the end of the track?

   if (pastedSomething)
   {
      mViewInfo.sel0 = t0;
      mViewInfo.sel1 = t0 + msClipLen;

      PushState(_("Pasted from the clipboard"), _("Paste"));

      RedrawProject();
   }
}

void AudacityProject::OnPasteOver()
{
   if(msClipLen>0.0)
   {
      mViewInfo.sel1=mViewInfo.sel0+msClipLen;
   }
   OnPaste();

   return;
}

void AudacityProject::OnTrim()
{
   if (mViewInfo.sel0 >= mViewInfo.sel1)
      return;

   TrackListIterator iter(mTracks);
   Track *n = iter.First();

   while (n) {
      if ((n->GetKind() == Track::Wave) && n->GetSelected()) {
         //Delete the section before the left selector
        ((WaveTrack*)n)->Trim(mViewInfo.sel0, mViewInfo.sel1);
      }
      n = iter.Next();
   }

   PushState(_("Trim file to selection"), _("Trim"));

   RedrawProject();
}

void AudacityProject::OnDelete()
{
   Clear();
}

void AudacityProject::OnSplitDelete()
{
   TrackListIterator iter(mTracks);

   Track *n = iter.First();

   while (n) {
      if (n->GetSelected()) {
         if (n->GetKind() == Track::Wave)
         {
            ((WaveTrack*)n)->SplitDelete(mViewInfo.sel0, mViewInfo.sel1);
         }
         else {
            n->Silence(mViewInfo.sel0, mViewInfo.sel1);
         }
      }
      n = iter.Next();
   }

   PushState(wxString::Format(_("Split-deleted %.2f seconds at t=%.2f"),
                              mViewInfo.sel1 - mViewInfo.sel0,
                              mViewInfo.sel0),
             _("Split Delete"));

   RedrawProject();
}

void AudacityProject::OnDisjoin()
{
   TrackListIterator iter(mTracks);

   Track *n = iter.First();

   while (n) {
      if (n->GetSelected()) {
         if (n->GetKind() == Track::Wave)
         {
            ((WaveTrack*)n)->Disjoin(mViewInfo.sel0, mViewInfo.sel1);
         }
      }
      n = iter.Next();
   }

   PushState(wxString::Format(_("Disjoined %.2f seconds at t=%.2f"),
                              mViewInfo.sel1 - mViewInfo.sel0,
                              mViewInfo.sel0),
             _("Disjoin"));

   RedrawProject();
}

void AudacityProject::OnJoin()
{
   TrackListIterator iter(mTracks);

   Track *n = iter.First();

   while (n) {
      if (n->GetSelected()) {
         if (n->GetKind() == Track::Wave)
         {
            ((WaveTrack*)n)->Join(mViewInfo.sel0, mViewInfo.sel1);
         }
      }
      n = iter.Next();
   }

   PushState(wxString::Format(_("Joined %.2f seconds at t=%.2f"),
                              mViewInfo.sel1 - mViewInfo.sel0,
                              mViewInfo.sel0),
             _("Join"));

   RedrawProject();
}

void AudacityProject::OnSilence()
{
   TrackListIterator iter(mTracks);

   Track *n = iter.First();

   while (n) {
      if (n->GetSelected())
         n->Silence(mViewInfo.sel0, mViewInfo.sel1);

      n = iter.Next();
   }

   PushState(wxString::
             Format(_("Silenced selected tracks for %.2f seconds at %.2f"),
                    mViewInfo.sel1 - mViewInfo.sel0, mViewInfo.sel0),
             _("Silence"));

   mTrackPanel->Refresh(false);
}

void AudacityProject::OnDuplicate()
{
   TrackListIterator iter(mTracks);

   Track *n = iter.First();
   Track *dest;

   TrackList newTracks;

   while (n) {
      if (n->GetSelected()) {
         dest = NULL;
         n->Copy(mViewInfo.sel0, mViewInfo.sel1, &dest);
         if (dest) {
            dest->Init(*n);
            dest->SetOffset(wxMax(mViewInfo.sel0, n->GetOffset()));
            newTracks.Add(dest);
         }
      }
      n = iter.Next();
   }

   TrackListIterator nIter(&newTracks);
   n = nIter.First();
   while (n) {
      mTracks->Add(n);
      n = nIter.Next();
   }

   PushState(_("Duplicated"), _("Duplicate"));

   RedrawProject();
}

void AudacityProject::OnCutLabels()
{
  if( mViewInfo.sel0 >= mViewInfo.sel1 )
     return;
 
  if( gPrefs->Read( wxT( "/GUI/EnableCutLines" ), ( long )0 ) )
     EditClipboardByLabel( &WaveTrack::CutAndAddCutLine );
  else
     EditClipboardByLabel( &WaveTrack::Cut );
  
  msClipProject = this;

  mViewInfo.sel1 = mViewInfo.sel0 = 0.0;
  
  PushState( _( "Cut labeled regions to the clipboard" ), _( "Cut Labels" ) );

  RedrawProject();
}

void AudacityProject::OnSplitCutLabels()
{
  if( mViewInfo.sel0 >= mViewInfo.sel1 )
     return;

  EditClipboardByLabel( &WaveTrack::SplitCut );
  
  msClipProject = this;

  PushState( _( "SplitCut labeled regions to the clipboard" ), 
        _( "Split Cut Labels" ) );

  RedrawProject();
}

void AudacityProject::OnCopyLabels()
{
  if( mViewInfo.sel0 >= mViewInfo.sel1 )
     return;

  EditClipboardByLabel( &WaveTrack::Copy );
  
  msClipProject = this;
  
  PushState( _( "Copied labeled regions to the clipboard" ), _( "Copy Labels" ) );

  mTrackPanel->Refresh( false );
}

void AudacityProject::OnDeleteLabels()
{
  if( mViewInfo.sel0 >= mViewInfo.sel1 )
     return;
  
  EditByLabel( &WaveTrack::Clear );
  
  PushState( _( "Deleted labeled regions" ), _( "Delete Labels" ) );

  RedrawProject();
}

void AudacityProject::OnSplitDeleteLabels()
{
  if( mViewInfo.sel0 >= mViewInfo.sel1 )
     return;
  
  EditByLabel( &WaveTrack::SplitDelete );
  
  PushState( _( "Split Deleted labeled regions" ), _( "Split Delete Labels" ) );

  RedrawProject();
}

void AudacityProject::OnSilenceLabels()
{
  if( mViewInfo.sel0 >= mViewInfo.sel1 )
     return;
  
  EditByLabel( &WaveTrack::Silence );
  
  PushState( _( "Silenced labeled regions" ), _( "Silence Labels" ) );

  mTrackPanel->Refresh( false );
}

void AudacityProject::OnSplitLabels()
{
  if( mViewInfo.sel0 >= mViewInfo.sel1 )
     return;
  
  EditByLabel( &WaveTrack::Split );
  
  PushState( _( "Split labeled regions" ), _( "Split Labels" ) );

  RedrawProject();
}

void AudacityProject::OnJoinLabels()
{
  if( mViewInfo.sel0 >= mViewInfo.sel1 )
     return;
  
  EditByLabel( &WaveTrack::Join );
  
  PushState( _( "Joined labeled regions" ), _( "Join Labels" ) );

  RedrawProject();
}

void AudacityProject::OnDisjoinLabels()
{
  if( mViewInfo.sel0 >= mViewInfo.sel1 )
     return;
  
  EditByLabel( &WaveTrack::Disjoin );
  
  PushState( _( "Disjoined labeled regions" ), _( "Disjoin Labels" ) );

  RedrawProject();
}

void AudacityProject::OnSplit()
{
   TrackListIterator iter(mTracks);

   double sel0 = mViewInfo.sel0;
   double sel1 = mViewInfo.sel1;

   for (Track* n=iter.First(); n; n = iter.Next())
   {
      if (n->GetKind() == Track::Wave)
      {
         WaveTrack* wt = (WaveTrack*)n;
         if (wt->GetSelected())
            wt->Split( sel0, sel1 );
      }
   }

   PushState(_("Split"), _("Split"));
   mTrackPanel->Refresh(false);
#if 0
//ANSWER-ME: Do we need to keep this commented out OnSplit() code?
// This whole section no longer used...
   /*
    * Previous (pre-multiclip) implementation of "Split" command
    * This does work only when a range is selected!
    *
   TrackListIterator iter(mTracks);

   Track *n = iter.First();
   Track *dest;

   TrackList newTracks;

   while (n) {
      if (n->GetSelected()) {
         double sel0 = mViewInfo.sel0;
         double sel1 = mViewInfo.sel1;

         dest = NULL;
         n->Copy(sel0, sel1, &dest);
         if (dest) {
            dest->Init(*n);
            dest->SetOffset(wxMax(sel0, n->GetOffset()));

            if (sel1 >= n->GetEndTime())
               n->Clear(sel0, sel1);
            else if (sel0 <= n->GetOffset()) {
               n->Clear(sel0, sel1);
               n->SetOffset(sel1);
            } else
               n->Silence(sel0, sel1);

            newTracks.Add(dest);
         }
      }
      n = iter.Next();
   }

   TrackListIterator nIter(&newTracks);
   n = nIter.First();
   while (n) {
      mTracks->Add(n);
      n = nIter.Next();
   }

   PushState(_("Split"), _("Split"));

   FixScrollbars();
   mTrackPanel->Refresh(false);
   */
#endif
}

void AudacityProject::OnSplitLabelsToTracks()
{
   TrackListIterator iter(mTracks);

   Track *n = iter.First();
   Track *srcRight = 0;
   Track *srcLeft = 0;
   bool stereo = false;
   LabelTrack *label = 0;

   while(n) {
      if(n->GetSelected()) {
         if(n->GetKind() == Track::Wave) {
            if(n->GetLinked() == true) {
               stereo = true;
               srcLeft = n;
               srcRight  = iter.Next();
            }
            else {
               srcRight = n;
               stereo = false;
            }
         }
         else if(n->GetKind() == Track::Label)
            label = (LabelTrack*)n;  // cast necessary to call LabelTrack specific methods
      }
      n = iter.Next();
   }

   // one new track for every label, from that label to the next
   
   TrackList newTracks;

   for(int i = 0; i < label->GetNumLabels(); i++) {
      wxString name = label->GetLabel(i)->title;
      double begin = label->GetLabel(i)->t;
      double end;

      // if on the last label, extend to the end of the wavetrack
      if(i == label->GetNumLabels() - 1) {
         if(stereo)
            end = wxMax(srcLeft->GetEndTime(), srcRight->GetEndTime());
         else
            end = srcLeft->GetEndTime();
      }
      else
         end = label->GetLabel(i+1)->t;

      Track *destLeft = 0;
      Track *destRight = 0;

      srcLeft->Copy(begin, end, &destLeft);
      if (destLeft) {
         destLeft->Init(*srcLeft);
         destLeft->SetOffset(wxMax(begin, srcLeft->GetOffset()));
         destLeft->SetName(name);
         
         mTracks->Add(destLeft);
      }

      if(stereo) {
         srcRight->Copy(begin, end, &destRight);
         if (destRight) {
            destRight->Init(*srcRight);
            destRight->SetOffset(wxMax(begin, srcRight->GetOffset()));
            destRight->SetName(name);
            
            mTracks->Add(destRight);
         }
         else if(destLeft)
            // account for possibility of a non-aligned linked track, which could
            // cause the left channel to be eligible for creating a new track,
            // but not the right.
            destLeft->SetLinked(false);
      }
   }

   PushState(_("Split at labels"), _("Split at labels"));

   RedrawProject();
}

void AudacityProject::OnSelectAll()
{
   TrackListIterator iter(mTracks);

   Track *t = iter.First();
   while (t) {
      t->SetSelected(true);
      t = iter.Next();
   }
   mViewInfo.sel0 = mTracks->GetMinOffset();
   mViewInfo.sel1 = mTracks->GetEndTime();

   ModifyState();
   
   mTrackPanel->Refresh(false);
}

void AudacityProject::OnSelectCursorEnd()
{
   double maxEndOffset = -1000000.0;

   TrackListIterator iter(mTracks);
   Track *t = iter.First();

   while (t) {
      if (t->GetSelected()) {
         if (t->GetEndTime() > maxEndOffset)
            maxEndOffset = t->GetEndTime();
      }

      t = iter.Next();
   }

   mViewInfo.sel1 = maxEndOffset;

   ModifyState();
   
   mTrackPanel->Refresh(false);
}

void AudacityProject::OnSelectStartCursor()
{
   double minOffset = 1000000.0;

   TrackListIterator iter(mTracks);
   Track *t = iter.First();

   while (t) {
      if (t->GetSelected()) {
         if (t->GetOffset() < minOffset)
            minOffset = t->GetOffset();
      }

      t = iter.Next();
   }

   mViewInfo.sel0 = minOffset;

   ModifyState();
   
   mTrackPanel->Refresh(false);
}

//
// View Menu
//

void AudacityProject::OnZoomIn()
{
   // DMM: Here's my attempt to get logical zooming behavior
   // when there's a selection that's currently at least
   // partially on-screen

   bool selectionIsOnscreen =
      (mViewInfo.sel0 < mViewInfo.h + mViewInfo.screen) &&
      (mViewInfo.sel1 > mViewInfo.h);

   bool selectionFillsScreen =
      (mViewInfo.sel0 < mViewInfo.h) &&
      (mViewInfo.sel1 > mViewInfo.h + mViewInfo.screen);
   
   if (selectionIsOnscreen && !selectionFillsScreen) {
      // Start with the center of the selection
      double selCenter = (mViewInfo.sel0 + mViewInfo.sel1) / 2;

      // If the selection center is off-screen, pick the
      // center of the part that is on-screen.
      if (selCenter < mViewInfo.h)
         selCenter = mViewInfo.h + (mViewInfo.sel1 - mViewInfo.h) / 2;
      if (selCenter > mViewInfo.h + mViewInfo.screen)
         selCenter = mViewInfo.h + mViewInfo.screen -
            (mViewInfo.h + mViewInfo.screen - mViewInfo.sel0) / 2;
         
      // Zoom in
      Zoom(mViewInfo.zoom *= 2.0);

      // Recenter on selCenter
      TP_ScrollWindow(selCenter - mViewInfo.screen / 2);
      return;
   }


   double origLeft = mViewInfo.h;
   double origWidth = mViewInfo.screen;
   Zoom(mViewInfo.zoom *= 2.0);
   
   double newh = origLeft + (origWidth - mViewInfo.screen) / 2;
   
   // MM: Commented this out because it was confusing users
   /*
   // make sure that the *right-hand* end of the selection is
   // no further *left* than 1/3 of the way across the screen
   if (mViewInfo.sel1 < newh + mViewInfo.screen / 3)
      newh = mViewInfo.sel1 - mViewInfo.screen / 3;

   // make sure that the *left-hand* end of the selection is
   // no further *right* than 2/3 of the way across the screen
   if (mViewInfo.sel0 > newh + mViewInfo.screen * 2 / 3)
      newh = mViewInfo.sel0 - mViewInfo.screen * 2 / 3;
   */

   TP_ScrollWindow(newh);
}

void AudacityProject::OnZoomOut()
{  
   //Zoom() may change these, so record original values:
   double origLeft = mViewInfo.h;
   double origWidth = mViewInfo.screen;

   Zoom(mViewInfo.zoom /= 2.0);

   double newh = origLeft + (origWidth - mViewInfo.screen) / 2;
   // newh = (newh > 0) ? newh : 0;
   TP_ScrollWindow(newh);

}

static double OldZooms[2]={ 44100.0/512.0, 4410.0/512.0 };
void AudacityProject::OnZoomToggle()
{
   double origLeft = mViewInfo.h;
   double origWidth = mViewInfo.screen;

   float f;
   // look at percentage difference.  We add a small fudge factor
   // to avoid testing for zero divisor.
   f = mViewInfo.zoom / (OldZooms[0] + 0.0001f);
   // If old zoom is more than 10 percent different, use it.
   if( (0.90f > f) || (f >1.10) ){
      OldZooms[1]=OldZooms[0];
      OldZooms[0]=mViewInfo.zoom;
   }
   Zoom( OldZooms[1] );
   double newh = origLeft + (origWidth - mViewInfo.screen) / 2;
   TP_ScrollWindow(newh);
}


void AudacityProject::OnZoomNormal()
{
   Zoom(44100.0 / 512.0);
   mTrackPanel->Refresh(false);
}

void AudacityProject::OnZoomFit()
{
   double len = mTracks->GetEndTime();

   if (len <= 0.0)
      return;

   int w, h;
   mTrackPanel->GetTracksUsableArea(&w, &h);
   w -= 10;

   Zoom(w / len);
   TP_ScrollWindow(0.0);
}

void AudacityProject::OnZoomFitV_Calc()
{
   int width, height, count;

   mTrackPanel->GetTracksUsableArea(&width, &height);

   height -= 28;

   count = 0;
   TrackListIterator iter(mTracks);
   Track *t = iter.First();
   while (t) {
      if (t->GetKind() == Track::Wave)
         count++;
      else
         height -= t->GetHeight();

      t = iter.Next();
   }

   if (count == 0)
      return;

   height = height / count;

   if (height < 40)
      height = 40;

   TrackListIterator iter2(mTracks);
   t = iter2.First();
   while (t) {
      if (t->GetKind() == Track::Wave)
         t->SetHeight(height);
      t = iter2.Next();
   }
}

void AudacityProject::OnZoomFitV()
{
   OnZoomFitV_Calc();

   mVsbar->SetThumbPosition(0);
   RedrawProject();
   ModifyState();
}

void AudacityProject::OnZoomSel()
{
   if (mViewInfo.sel1 <= mViewInfo.sel0)
      return;

   Zoom(mViewInfo.zoom * mViewInfo.screen / (mViewInfo.sel1 - mViewInfo.sel0));
   TP_ScrollWindow(mViewInfo.sel0);
}

void AudacityProject::OnSnapOn()
{
   mSnapTo = 1;
   TP_DisplaySelection();
}

void AudacityProject::OnSnapOff()
{
   mSnapTo = 0;
   TP_DisplaySelection();
}

void AudacityProject::OnPlotSpectrum()
{
   int selcount = 0;
   int i;
   double rate = 0;
   sampleCount len = 0;
   float *buffer = NULL;
   bool warning = false;
   TrackListIterator iter(mTracks);
   Track *t = iter.First();
   while (t) {
      if (t->GetSelected() && t->GetKind() == Track::Wave) {
         WaveTrack *track = (WaveTrack *)t;
         if (selcount==0) {
            rate = track->GetRate();
            longSampleCount start, end;
            start = track->TimeToLongSamples(mViewInfo.sel0);
            end = track->TimeToLongSamples(mViewInfo.sel1);
            len = (sampleCount)(end - start);
            if (len > 1048576) {
               warning = true;
               len = 1048576;
            }
            buffer = new float[len];
            track->Get((samplePtr)buffer, floatSample, start, len);
         }
         else {
            if (track->GetRate() != rate) {
               wxMessageBox(_("To plot the spectrum, all selected tracks must be the same sample rate."));
               delete[] buffer;
               return;
            }
            longSampleCount start;
            start = track->TimeToLongSamples(mViewInfo.sel0);
            float *buffer2 = new float[len];
            track->Get((samplePtr)buffer2, floatSample, start, len);
            for(i=0; i<len; i++)
               buffer[i] += buffer2[i];
            delete[] buffer2;
         }
         selcount++;
      }
      t = iter.Next();
   }
   
   if (selcount == 0)
      return;
   
   if (selcount > 1)
      for(i=0; i<len; i++)
         buffer[i] /= selcount;
   
   if (warning) {
      wxString msg;
      msg.Printf(_("Too much audio was selected.  Only the first %.1f seconds of audio will be analyzed."),
                          (len / rate));
      wxMessageBox(msg);
   }

   InitFreqWindow(gParentWindow);
   gFreqWindow->Plot(len, buffer, rate);
   gFreqWindow->Show(true);
   gFreqWindow->Raise();

   delete[] buffer;
}


void AudacityProject::OnShowControlToolBar()
{
   mToolManager->ShowHide( ControlBarID );
   ModifyToolbarMenus();
}

void AudacityProject::OnShowDeviceToolBar()
{
   mToolManager->ShowHide( DeviceBarID );
   ModifyToolbarMenus();
}

void AudacityProject::OnShowEditToolBar()
{
   mToolManager->ShowHide( EditBarID );
   ModifyToolbarMenus();
}

void AudacityProject::OnShowMeterToolBar()
{
   mToolManager->ShowHide( MeterBarID );
   ModifyToolbarMenus();
}

void AudacityProject::OnShowMixerToolBar()
{
   mToolManager->ShowHide( MixerBarID );
   ModifyToolbarMenus();
}

void AudacityProject::OnShowSelectionToolBar()
{
   mToolManager->ShowHide( SelectionBarID );
   ModifyToolbarMenus();
}

void AudacityProject::OnShowToolsToolBar()
{
   mToolManager->ShowHide( ToolsBarID );
   ModifyToolbarMenus();
}

void AudacityProject::OnShowTranscriptionToolBar()
{
   mToolManager->ShowHide( TranscriptionBarID );
   ModifyToolbarMenus();
}


//
// Project Menu
//

void AudacityProject::OnImport()
{
   wxString path = gPrefs->Read(wxT("/DefaultOpenPath"),::wxGetCwd());

   // TODO: Build the list of file types dynamically
   
   wxFileDialog dlog(this, _("Select one or more audio files..."),
                     path, wxT(""),
                     GetImportFilesFilter(),
                     wxOPEN | wxMULTIPLE);

   int result = dlog.ShowModal();

   if (result != wxID_OK)
      return;

   wxArrayString selectedFiles;
   unsigned int ff;

   dlog.GetPaths(selectedFiles);

   selectedFiles.Sort();

   for(ff=0; ff<selectedFiles.GetCount(); ff++) {
      wxString fileName = selectedFiles[ff];

      path =::wxPathOnly(fileName);
      gPrefs->Write(wxT("/DefaultOpenPath"), path);
      
      Import(fileName);
   }
}

void AudacityProject::OnImportLabels()
{
   wxString path = gPrefs->Read(wxT("/DefaultOpenPath"),::wxGetCwd());

   wxString fileName =
       wxFileSelector(_("Select a text file containing labels..."),
                      path,     // Path
                      wxT(""),       // Name
                      wxT(".txt"),   // Extension
                      _("Text files (*.txt)|*.txt|All files (*.*)|*.*"),
                      0,        // Flags
                      this);    // Parent

   if (fileName != wxT("")) {
      path =::wxPathOnly(fileName);
      gPrefs->Write(wxT("/DefaultOpenPath"), path);

      wxTextFile f;

      f.Open(fileName);
      if (!f.IsOpened()) {
         wxMessageBox(_("Could not open file: ") + fileName);
         return;
      }

      LabelTrack *newTrack = new LabelTrack(mDirManager);

      newTrack->Import(f);

      SelectNone();
      mTracks->Add(newTrack);
      newTrack->SetSelected(true);

      PushState(wxString::
                Format(_("Imported labels from '%s'"), fileName.c_str()),
                _("Import Labels"));

      RedrawProject();
   }
}

void AudacityProject::OnImportMIDI()
{
   wxString path = gPrefs->Read(wxT("/DefaultOpenPath"),::wxGetCwd());

   wxString fileName = wxFileSelector(_("Select a MIDI file..."),
                                      path,     // Path
                                      wxT(""),       // Name
                                      wxT(""),       // Extension
                                      _("All files (*.*)|*.*|MIDI files (*.mid)|*.mid|Allegro files (*.gro)|*.gro"),
                                      0,        // Flags
                                      this);    // Parent

   if (fileName != wxT("")) {
      path =::wxPathOnly(fileName);
      gPrefs->Write(wxT("/DefaultOpenPath"), path);

      NoteTrack *newTrack = new NoteTrack(mDirManager);

      if (::ImportMIDI(fileName, newTrack)) {

         SelectNone();
         mTracks->Add(newTrack);
         newTrack->SetSelected(true);

         PushState(wxString::Format(_("Imported MIDI from '%s'"),
                                    fileName.c_str()),
                   _("Import MIDI"));

         RedrawProject();
         mTrackPanel->EnsureVisible(newTrack);
      }
   }
}

void AudacityProject::OnImportRaw()
{
   wxString path = gPrefs->Read(wxT("/DefaultOpenPath"),::wxGetCwd());

   wxString fileName =
       wxFileSelector(_("Select any uncompressed audio file..."),
                      path,     // Path
                      wxT(""),       // Name
                      wxT(""),       // Extension
                      _("All files (*)|*"),
                      0,        // Flags
                      this);    // Parent

   if (fileName == wxT(""))
      return;

   path =::wxPathOnly(fileName);
   gPrefs->Write(wxT("/DefaultOpenPath"), path);
   
   Track **newTracks;
   int numTracks;
   
   mImportingRaw = true;

   ProgressShow(_("Import"));

   numTracks = ::ImportRaw(this, fileName, mTrackFactory, &newTracks,
                           AudacityProject::ImportProgressCallback,
                           this);

   ProgressHide();

   mImportingRaw = false;
   
   if (numTracks <= 0)
      return;

   AddImportedTracks(fileName, newTracks, numTracks);
}

void AudacityProject::OnEditID3()
{
   if (mTags->ShowEditDialog(this, _("Edit ID3 Tags (for MP3 exporting)")))
      PushState(_("Edit ID3 tags"), _("Edit ID3 Tags"));
}

void AudacityProject::OnMixAndRender()
{
   WaveTrack *newLeft = NULL;
   WaveTrack *newRight = NULL;

   if (::MixAndRender(mTracks, mTrackFactory, mRate, mDefaultFormat, 0.0, 0.0,
                      &newLeft, &newRight)) {

      // Remove originals, get stats on what tracks were mixed

      TrackListIterator iter(mTracks);
      Track *t = iter.First();
      int selectedCount = 0;
      wxString firstName;

      while (t) {
         if (t->GetSelected()) {
            if (selectedCount==0)
               firstName = t->GetName();

            // Add one to the count if it's an unlinked track, or if it's the first
            // in a stereo pair
            if (t->GetLinked() || !mTracks->GetLink(t))
                selectedCount++;

            delete t;
            t = iter.RemoveCurrent();
         }
         else
            t = iter.Next();
      }

      // Add new tracks

      mTracks->Add(newLeft);
      if (newRight)
         mTracks->Add(newRight);

      // If we're just rendering (not mixing), keep the track name the same
      if (selectedCount==1) {
         newLeft->SetName(firstName);
         if (newRight)
            newRight->SetName(firstName);
      }

      // Smart history/undo message
      if (selectedCount==1) {
         wxString msg;
         msg.Printf(_("Rendered all audio in track '%s'"), firstName.c_str());
         PushState(msg, _("Render"));
      }
      else {
         wxString msg;
         if (newRight)
            msg.Printf(_("Mixed and rendered %d tracks into one new stereo track"),
                       selectedCount);
         else
            msg.Printf(_("Mixed and rendered %d tracks into one new mono track"),
                       selectedCount);
         PushState(msg, _("Mix and Render"));
      }

      mTrackPanel->SetFocusedTrack(newLeft);
      RedrawProject();
   }
}

void AudacityProject::OnSelectionSave()
{
   mSel0save = mViewInfo.sel0;
   mSel1save = mViewInfo.sel1;
}

void AudacityProject::OnSelectionRestore()
{
   mViewInfo.sel0 = mSel0save;
   mViewInfo.sel1 = mSel1save;

   ModifyState();
   
   mTrackPanel->Refresh(false);
}

void AudacityProject::OnCursorTrackStart()
{
   double minOffset = 1000000.0;

   TrackListIterator iter(mTracks);
   Track *t = iter.First();

   while (t) {
      if (t->GetSelected()) {
         if (t->GetOffset() < minOffset)
            minOffset = t->GetOffset();
      }

      t = iter.Next();
   }

   if (minOffset < 0.0) minOffset = 0.0;
   mViewInfo.sel0 = minOffset;
   mViewInfo.sel1 = minOffset;
   ModifyState();
   mTrackPanel->Refresh(false);
}

void AudacityProject::OnCursorTrackEnd()
{
   double maxEndOffset = -1000000.0;
   double thisEndOffset = 0.0;

   TrackListIterator iter(mTracks);
   Track *t = iter.First();

   while (t) {
      if (t->GetSelected()) {
         thisEndOffset = t->GetEndTime();
         if (thisEndOffset > maxEndOffset)
            maxEndOffset = thisEndOffset;
      }

      t = iter.Next();
   }

   mViewInfo.sel0 = maxEndOffset;
   mViewInfo.sel1 = maxEndOffset;
   ModifyState();
   mTrackPanel->Refresh(false);
}

void AudacityProject::OnCursorSelStart()
{
   mViewInfo.sel1 = mViewInfo.sel0;
   ModifyState();
   mTrackPanel->Refresh(false);
}

void AudacityProject::OnCursorSelEnd()
{
   mViewInfo.sel0 = mViewInfo.sel1;
   ModifyState();
   mTrackPanel->Refresh(false);
}

void AudacityProject::HandleAlign(int index, bool moveSel)
{
   TrackListIterator iter(mTracks);
   wxString action;
   double offset;
   double minOffset = 1000000000.0;
   double maxEndOffset = 0.0;
   double avgOffset = 0.0;
   int numSelected = 0;
   Track *t = iter.First();
   double delta = 0.0;
   double newPos = -1.0;

   while (t) {
      if (t->GetSelected()) {
         numSelected++;

         offset = t->GetOffset();
         avgOffset += offset;
         if (offset < minOffset)
            minOffset = offset;
         if (t->GetEndTime() > maxEndOffset)
            maxEndOffset = t->GetEndTime();
      }
      t = iter.Next();
   }

   avgOffset /= numSelected;

   switch(index) {
   case kAlignZero:
      delta = -minOffset;
      action = _("Aligned with zero");
      break;
   case kAlignCursor:
      delta = mViewInfo.sel0 - minOffset;
      action = _("Aligned cursor");
      break;
   case kAlignSelStart:
      delta = mViewInfo.sel0 - minOffset;
      action = _("Aligned with selection start");
      break;
   case kAlignSelEnd:
      delta = mViewInfo.sel1 - minOffset;
      action = _("Aligned with selection end");
      break;
   case kAlignEndCursor:
      delta = mViewInfo.sel0 - maxEndOffset;
      action = _("Aligned end with cursor");
      break;
   case kAlignEndSelStart:
      delta = mViewInfo.sel0 - maxEndOffset;
      action = _("Aligned end with selection start");
      break;
   case kAlignEndSelEnd:
      delta = mViewInfo.sel1 - maxEndOffset;
      action = _("Aligned end with selection end");
      break;
   case kAlign:
      newPos = avgOffset;
      action = _("Aligned");
      break;
   }

   if (newPos >= 0.0) {
      TrackListIterator iter(mTracks);
      Track *t = iter.First();
      
      while (t) {
         if (t->GetSelected()) {
            t->SetOffset(newPos);
         }
         t = iter.Next();
      }
   }

   if (delta != 0.0) {
      TrackListIterator iter(mTracks);
      Track *t = iter.First();
      
      while (t) {
         if (t->GetSelected()) {
            t->SetOffset(t->GetOffset() + delta);
         }
         t = iter.Next();
      }
   }

   if (moveSel) {
      mViewInfo.sel0 += delta;
      mViewInfo.sel1 += delta;
   }

   PushState(action, _("Align"));

   mTrackPanel->Refresh(false);
}

void AudacityProject::OnAlign(int index)
{
   HandleAlign(index, false);
}

void AudacityProject::OnAlignMoveSel(int index)
{
   HandleAlign(index, true);
}

void AudacityProject::OnNewWaveTrack()
{
   WaveTrack *t = mTrackFactory->NewWaveTrack(mDefaultFormat, mRate);
   SelectNone();

   mTracks->Add(t);
   t->SetSelected(true);

   PushState(_("Created new audio track"), _("New Track"));

   RedrawProject();
   mTrackPanel->EnsureVisible(t);
}

void AudacityProject::OnNewStereoTrack()
{
   WaveTrack *t = mTrackFactory->NewWaveTrack(mDefaultFormat, mRate);
   t->SetChannel(Track::LeftChannel);
   SelectNone();
   
   mTracks->Add(t);
   t->SetSelected(true);
   t->SetLinked (true);
   
   t = mTrackFactory->NewWaveTrack(mDefaultFormat, mRate);
   t->SetChannel(Track::RightChannel);
   
   mTracks->Add(t);
   t->SetSelected(true);
   t->SetTeamed (true);
   
   PushState(_("Created new stereo audio track"), _("New Track"));
   
   RedrawProject();
   mTrackPanel->EnsureVisible(t);
}

void AudacityProject::OnNewLabelTrack()
{
   LabelTrack *t = new LabelTrack(mDirManager);

   SelectNone();

   mTracks->Add(t);
   t->SetSelected(true);

   PushState(_("Created new label track"), _("New Track"));

   RedrawProject();
   mTrackPanel->EnsureVisible(t);
}

void AudacityProject::OnNewTimeTrack()
{
   TrackListIterator iter(mTracks);
   Track *t = iter.First();
   bool alreadyHaveTimeTrack = false;
   
   while (t)
      {
         if (t->GetKind() == Track::Time)
            {
               alreadyHaveTimeTrack = true;
               break;
            }
         t = iter.Next();
      }
   
   if( alreadyHaveTimeTrack )
      {
         wxString msg;
         msg.Printf(_("The version of Audacity you are using does not support multiple time tracks."));
         wxMessageBox(msg);
      }
   else
      {
         TimeTrack *t = new TimeTrack(mDirManager);

         SelectNone();
         mTracks->AddToHead(t);
         t->SetSelected(true);
         
         PushState(_("Created new time track"), _("New Track"));

         /*
         TrackListIterator iter(mTracks);
         for( Track *tr = iter.First(); (tr); tr = iter.Next() )
            tr->SetTimeTrack( t );
         */
         
         RedrawProject();
         mTrackPanel->EnsureVisible(t);
      }
}

void AudacityProject::OnSmartRecord()
{
   SmartRecordDialog dialog(this /* parent */ );
   dialog.ShowModal();
}

int AudacityProject::DoAddLabel(double left, double right)
{
   TrackListIterator iter(mTracks);
   LabelTrack *lt = NULL;
   LabelTrack *st = NULL;

   Track *t = iter.First();
   while (t) {
      if (t->GetKind() == Track::Label) {
         lt = (LabelTrack *)t;
         if (lt->GetSelected()) {
            st = lt;
            break;
         }
      }
      t = iter.Next();
   }

   if (st)
      lt = st;

   if (!lt) {
      lt = new LabelTrack(mDirManager);
      mTracks->Add(lt);
      lt->SetSelected(true);
   }

// LLL: Commented as it seemed a little forceful to remove users
//      selection when adding the label.  This does not happen if
//      you select several tracks and the last one of those is a
//      label track...typing a label will not clear the selections.
//
//   SelectNone();
//   lt->SetSelected(true);

   int index = lt->AddLabel(left, right);

   PushState(_("Added label"), _("Label"));

   RedrawProject();
   mTrackPanel->EnsureVisible((Track *)lt);
   mTrackPanel->SetFocus();

   return index;
}

void AudacityProject::OnAddLabel()
{
   DoAddLabel(mViewInfo.sel0, mViewInfo.sel1);
}

void AudacityProject::OnAddLabelPlaying()
{
   if (GetAudioIOToken()>0 &&
       gAudioIO->IsStreamActive(GetAudioIOToken())) {
      double indicator = gAudioIO->GetStreamTime();
      DoAddLabel(indicator, indicator);
   }
}

void AudacityProject::OnEditLabels()
{
   LabelDialog d(this, mDirManager, mTracks, mViewInfo, mRate);
   
   if (d.ShowModal() == wxID_OK) {
      PushState(_("Edited labels"), _("Label"));
      RedrawProject();
   }
}

// #define PRESET_FORMAT 20050501
#define PRESET_FORMAT 20050428
// #define PRESET_COUNT  16
#define PRESET_COUNT  14
void AudacityProject::OnExportCleanSpeechPresets()
{
   wxString userdatadir = FileNames::DataDir();
   #ifdef __WXMSW__
   wxString presetsDefaultLoc = userdatadir + wxT("\\presets");
   #else
   wxString presetsDefaultLoc = userdatadir + wxT("/presets");
   #endif
   wxString path = gPrefs->Read(wxT("/Directories/PresetsDir"), presetsDefaultLoc);

   wxString nameOnly;
   wxString fName;
   wxString extension = wxT(".csp");
   bool fileOkay;

   do {
      fileOkay = true;

      fName = wxFileSelector(_("Save CleanSpeech Preset File As:"),
                        path,
                        wxT("*.csp"),       // default file extension
                        extension,
                        _("CleanSpeech Presets (*.csp)|*.csp"),
                        wxSAVE | wxOVERWRITE_PROMPT);

      if (fName.empty()) { // if cancel selected
         return;
      }
      if (fName.Length() >= 256) {
         wxMessageBox(_("Sorry, pathnames longer than 256 characters not supported."));
         fileOkay = false;
         continue;
      }
      ::wxSplitPath(fName, &path, &nameOnly, &extension);
      wxFFile presetsFile(fName, wxT("wb"));

      bool flag = presetsFile.IsOpened();

      if (flag == true) {
         int preset[PRESET_COUNT];
         preset[0]  = PRESET_FORMAT;
         preset[1]  = PRESET_COUNT;

         preset[2]  = gPrefs->Read(wxT("/CsPresets/ClickThresholdLevel"), 200L);
         preset[3]  = gPrefs->Read(wxT("/CsPresets/ClickWidth"), 20L);
         preset[4]  = gPrefs->Read(wxT("/CsPresets/LevellerDbChoiceIndex"), 10L);
         preset[5]  = gPrefs->Read(wxT("/CsPresets/LevellerNumPasses"), 2L);
         preset[6]  = gPrefs->Read(wxT("/CsPresets/Noise_Level"), 3L);
         preset[7]  = gPrefs->Read(wxT("/CsPresets/Norm_AmpDbGain"), 1L);
         preset[8]  = gPrefs->Read(wxT("/CsPresets/Norm_RemoveDcOffset"), 1L);
         preset[9]  = gPrefs->Read(wxT("/CsPresets/SpikeDbChoiceIndex"), 13L);
         preset[10] = gPrefs->Read(wxT("/CsPresets/SpikeMaxDurationMs"), SKIP_EFFECT_MILLISECOND);
         preset[11] = gPrefs->Read(wxT("/CsPresets/TruncDbChoiceIndex"), 8L);
         preset[12] = gPrefs->Read(wxT("/CsPresets/TruncLongestAllowedSilentMs"), 1000L);
//         preset[14] = gPrefs->Read(wxT("/GUI/Save128HqMasterAfter"), 0L);
//         preset[15] = gPrefs->Read(wxT("/GUI/Save128HqMasterBefore"), 0L);

         int expectedCount = wxGetApp().GetCleanSpeechNoiseGateExpectedCount();
         float* pNoiseGate = wxGetApp().GetCleanSpeechNoiseGate();
         double noiseGateSum = 0.0;
         int lenNoiseGate = expectedCount / sizeof(float);
         for (int i = 0; i < lenNoiseGate; ++i) {
            noiseGateSum += fabs(pNoiseGate[i]);
         }
         int noiseCheckSum = abs((int)noiseGateSum);
         preset[13] = noiseCheckSum;
         gPrefs->Write(wxT("/Validate/NoiseGateSum"), noiseCheckSum);

         int lenPreset = sizeof(preset);
         int count = presetsFile.Write(preset, lenPreset);
         count = presetsFile.Write(pNoiseGate, expectedCount);

         presetsFile.Close();
      }
      else {
         wxMessageBox(_("Problem encountered exporting presets."),
                     _("Unable to export"),
                     wxOK | wxICON_WARNING);
         fileOkay = false;
         continue;
      }
   } while(!fileOkay);
}

void AudacityProject::OnImportCleanSpeechPresets()
{
   wxString userdatadir = FileNames::DataDir();
   #ifdef __WXMSW__
   wxString presetsDefaultLoc = userdatadir + wxT("\\presets");
   #else
   wxString presetsDefaultLoc = userdatadir + wxT("/presets");
   #endif

   wxString path = gPrefs->Read(wxT("/Directories/PresetsDir"), presetsDefaultLoc);
   wxString extension = wxT(".csp");
   wxString fName;
   bool fileOkay;

   do {
      fileOkay = true;

      fName = wxFileSelector(_("Open CleanSpeech Preset File:"),
                             path,
                             _("*.csp"),       // default file name
                             extension,
                             _("CleanSpeech Presets (*.csp)|*.csp"),
                             wxOPEN);

      if (fName.empty()) { // if cancel selected
         return;
      }
      wxFFile presetsFile(fName, wxT("rb"));
      bool flag = presetsFile.IsOpened();
      if (flag == true) {
         int preset[PRESET_COUNT];
         int lenPreset = sizeof(preset);
         int count = presetsFile.Read(preset, lenPreset);
         if (preset[0] != PRESET_FORMAT) {
            wxMessageBox(wxString::Format(_("Preset may be invalid or corrupted.\nExpected format %d ... found %d"), PRESET_FORMAT, preset[0]),
                         _("Error opening preset"),
                         wxOK | wxCENTRE | wxICON_WARNING, this);
            return;
         }
         int expectedCount = wxGetApp().GetCleanSpeechNoiseGateExpectedCount();
         float* pNoiseGate = wxGetApp().GetCleanSpeechNoiseGate();
         count = presetsFile.Read(pNoiseGate, expectedCount);

         gPrefs->Write(wxT("/CsPresets/ClickThresholdLevel"), preset[2]);
         gPrefs->Write(wxT("/CsPresets/ClickWidth"), preset[3]);
         gPrefs->Write(wxT("/CsPresets/LevellerDbChoiceIndex"), preset[4]);
         gPrefs->Write(wxT("/CsPresets/LevellerNumPasses"), preset[5]);
         gPrefs->Write(wxT("/CsPresets/Noise_Level"), preset[6]);
         gPrefs->Write(wxT("/CsPresets/Norm_AmpDbGain"), preset[7]);
         gPrefs->Write(wxT("/CsPresets/Norm_RemoveDcOffset"), preset[8]);
         gPrefs->Write(wxT("/CsPresets/SpikeDbChoiceIndex"), preset[9]);
         gPrefs->Write(wxT("/CsPresets/SpikeMaxDurationMs"), preset[10]);
         gPrefs->Write(wxT("/CsPresets/TruncDbChoiceIndex"), preset[11]);
         gPrefs->Write(wxT("/CsPresets/TruncLongestAllowedSilentMs"), preset[12]);
//         gPrefs->Write(wxT("/GUI/Save128HqMasterAfter"), preset[14]);
//         gPrefs->Write(wxT("/GUI/Save128HqMasterBefore"), preset[15]);

         double noiseGateSum = 0.0;
         int lenNoiseGate = expectedCount / sizeof(float);
         for (int i = 0; i < lenNoiseGate; ++i) {
            noiseGateSum += fabs(pNoiseGate[i]);
         }
         preset[13] = abs((int)noiseGateSum);

         presetsFile.Close();
      }
      else {
         wxMessageBox(_("Problem encountered importing presets."),
                     _("Unable to import"),
                     wxOK | wxICON_WARNING);
         fileOkay = false;
         continue;
      }
   } while(!fileOkay);
}

void AudacityProject::OnApplyChain()
{
   BatchProcessDialog d(this);
   d.ShowModal();

   // LL:  See comments in ModifyUndoMenus() for info about this...
   //
   // Refresh the Undo menu.
   ModifyUndoMenus();
}

void AudacityProject::OnEditChains()
{
   EditChainsDialog d(this);
   d.ShowModal();
}

wxString AudacityProject::BuildCleanFileName(wxString fileName)
{
   wxFileName newFileName(fileName);
   wxString justName = newFileName.GetName();
   wxString pathName = newFileName.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);

   if (justName == wxT("")) {
      wxDateTime now = wxDateTime::Now();
      int year = now.GetYear();
      wxDateTime::Month month = now.GetMonth();
      wxString monthName = now.GetMonthName(month);
      int dom = now.GetDay();
      int hour = now.GetHour();
      int minute = now.GetMinute();
      int second = now.GetSecond();
      justName = wxString::Format(wxT("%d-%s-%02d-%02d-%02d-%02d"), 
           year, monthName.c_str(), dom, hour, minute, second);

//      SetName(cleanedFileName);
//      bool isStereo;
//      double endTime = project->mTracks->GetEndTime();
//      double startTime = 0.0;
      OnSelectAll();
      pathName = gPrefs->Read(wxT("/DefaultOpenPath"), ::wxGetCwd());
      ::wxMessageBox(wxString::Format(_("Export recording to %s\n/cleaned/%s.mp3"), 
         pathName.c_str(), justName.c_str()),
         _("Export recording"),
                  wxOK | wxCENTRE);
      pathName += wxT("/");
   }
   wxString cleanedName = pathName;
   cleanedName += wxT("cleaned");
   bool flag  = ::wxFileName::FileExists(cleanedName);
   if (flag == true) {
      ::wxMessageBox(_("Cannot create directory 'cleaned'. \nFile already exists that is not a directory"));
      return wxT("");
   }
   ::wxFileName::Mkdir(cleanedName, 0777, wxPATH_MKDIR_FULL); // make sure it exists

   cleanedName += wxT("/");
   cleanedName += justName;
   cleanedName += wxT(".mp3");
   mRecentFiles->AddFileToHistory(cleanedName);

   return cleanedName;
}

void AudacityProject::OnRemoveTracks()
{
   TrackListIterator iter(mTracks);
   Track *t = iter.First();
   Track *f = NULL;
   Track *l = NULL;

   while (t) {
      if (t->GetSelected()) {
         if (!f)
            f = l;         // Capture the track preceeding the first removed track
         delete t;
         t = iter.RemoveCurrent();
      }
      else {
         l = t;
         t = iter.Next();
      }
   }

   // All tracks but the last were removed...try to use the last track
   if (!f)
      f = l;

   // Try to use the first track after the removal or, if none,
   // the track preceeding the removal
   if (f) {
      t = mTracks->GetNext(f, true);
      if (t)
         f = t;
   }

   // If we actually have something left, then make sure it's seen
   if (f)
      mTrackPanel->EnsureVisible(f);

   PushState(_("Removed audio track(s)"), _("Remove Track"));

   mTrackPanel->Refresh(false);
}

//
// Help Menu
//

void AudacityProject::OnAbout()
{
   AboutDialog dlog(this);
   dlog.ShowModal();
}

void AudacityProject::OnHelp()
{
   ::ShowHelp(this);
}

void AudacityProject::OnHelpIndex()
{
   ::ShowHelpIndex(this);
}

void AudacityProject::OnHelpSearch()
{
   ::SearchHelp(this);
}

void AudacityProject::OnBenchmark()
{
   ::RunBenchmark(this);
}

//

void AudacityProject::OnSeparator()
{

}

void AudacityProject::OnCollapseAllTracks()
{
   TrackListIterator iter(mTracks);
   Track *t = iter.First();

   while (t)
   {
      t->SetMinimized(true);
      t = iter.Next();
   }

   ModifyState();
   RedrawProject();
}

void AudacityProject::OnExpandAllTracks()
{
   TrackListIterator iter(mTracks);
   Track *t = iter.First();

   while (t)
   {
      t->SetMinimized(false);
      t = iter.Next();
   }

   ModifyState();
   RedrawProject();
}

void AudacityProject::OnLockPlayRegion()
{
   mLockPlayRegion = true;
   mRuler->Refresh(false);
}

void AudacityProject::OnUnlockPlayRegion()
{
   mLockPlayRegion = false;
   mRuler->Refresh(false);
}

void AudacityProject::OnResample()
{
   TrackListIterator iter(mTracks);

   int newRate;
   
   while (true) {
      wxTextEntryDialog* dlg = new wxTextEntryDialog(this,
         _("Enter new samplerate:"), _("Resample"), wxT(""));
      if (dlg->ShowModal() != wxID_OK)
         return; // user cancelled dialog
      newRate = atoi(dlg->GetValue().mb_str());
      delete dlg;
      if (newRate < 1 || newRate > 1000000)
         wxMessageBox(_("The entered value is invalid"), _("Error"),
                      wxICON_STOP, this);
      else
         break;
   }

   int ndx = 0;
   for (Track *t = iter.First(); t; t = iter.Next())
   {
      wxString msg;
      
      msg.Printf(_("Resampling track %d"), ++ndx);

      GetActiveProject()->ProgressShow(_("Resample"),
                                       msg);

      if (t->GetSelected() && t->GetKind() == Track::Wave)
         if (!((WaveTrack*)t)->Resample(newRate, true))
            break;
   }
   GetActiveProject()->ProgressHide();
   
   PushState(_("Resampled audio track(s)"), _("Resample Track"));
   RedrawProject();
   
   // Need to reset
   FinishAutoScroll();
}

// Indentation settings for Vim and Emacs and unique identifier for Arch, a
// version control system. Please do not modify past this point.
//
// Local Variables:
// c-basic-offset: 3
// indent-tabs-mode: nil
// End:
//
// vim: et sts=3 sw=3
// arch-tag: e8ab21c6-d9b9-4d35-b4c2-ff90c1781b85

