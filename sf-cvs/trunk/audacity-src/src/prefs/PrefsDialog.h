/**********************************************************************

  Audacity: A Digital Audio Editor

  PrefsDialog.h

  Joshua Haberman

**********************************************************************/

#ifndef __AUDACITY_PREFS_DIALOG__
#define __AUDACITY_PREFS_DIALOG__

#include <wx/dialog.h>
#include <wx/string.h>

class wxNotebook;
class wxWindow;
class wxButton;
class wxCommandEvent;
class wxFrame;

class PrefsDialog:public wxDialog {

 public:
   PrefsDialog(wxWindow * parent);
   ~PrefsDialog();

   void OnCategoryChange(wxCommandEvent & event);
   void OnOK(wxCommandEvent & event);
   void OnCancel(wxCommandEvent & event);

   void SelectPageByName(wxString pageName);
   void ShowTempDirPage();

 private:
   wxNotebook *mCategories;
   wxButton *mOK;
   wxButton *mCancel;

   int mSelected;

   #ifdef __WXMAC__
   wxFrame *mMacHiddenFrame;
   #endif

 public:
    DECLARE_EVENT_TABLE()
};

#endif

// Indentation settings for Vim and Emacs and unique identifier for Arch, a
// version control system. Please do not modify past this point.
//
// Local Variables:
// c-basic-offset: 3
// indent-tabs-mode: nil
// End:
//
// vim: et sts=3 sw=3
// arch-tag: 43249bcd-739b-4b30-95dd-3e70495da6eb

