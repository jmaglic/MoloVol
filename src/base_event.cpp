#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#  include <wx/wx.h>
#endif

#include "base.h"
#include "controller.h"
#include "misc.h"
#include <vector>
#include <chrono>
#include <thread>

/////////////////
// EVENT TABLE //
/////////////////

wxDEFINE_EVENT(wxEVT_COMMAND_WORKERTHREAD_COMPLETED, wxThreadEvent);

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
  EVT_CLOSE(MainFrame::OnClose)
  EVT_BUTTON(BUTTON_Calc, MainFrame::OnCalc)
  EVT_BUTTON(BUTTON_Abort, MainFrame::OnAbort)
  EVT_BUTTON(BUTTON_Browse, MainFrame::OnAtomBrowse)
  EVT_BUTTON(BUTTON_Elements, MainFrame::OnElementsBrowse)
  EVT_BUTTON(BUTTON_LoadFiles, MainFrame::OnLoadFiles)
  EVT_TEXT_ENTER(TEXT_Filename, MainFrame::OnLoadFiles)
  EVT_TEXT_ENTER(TEXT_Elementspath, MainFrame::OnLoadFiles)
  EVT_BUTTON(BUTTON_Report, MainFrame::OnExportReport)
  EVT_BUTTON(BUTTON_TotalMap, MainFrame::OnExportTotalMap)
  EVT_BUTTON(BUTTON_CavityMap, MainFrame::OnExportCavityMap)
  EVT_BUTTON(BUTTON_Dirpicker, MainFrame::OnBrowseOutput)
  EVT_TEXT(wxID_ANY, MainFrame::OnTextInput)
  EVT_CHECKBOX(CHECKBOX_TwoProbes, MainFrame::ProbeModeChange)
  EVT_CHECKBOX(CHECKBOX_Report, MainFrame::OnToggleAutoExport)
  EVT_CHECKBOX(CHECKBOX_SurfaceMap, MainFrame::OnToggleAutoExport)
  EVT_CHECKBOX(CHECKBOX_CavityMaps, MainFrame::OnToggleAutoExport)
  EVT_GRID_CELL_CHANGING(MainFrame::GridChange)
  EVT_COMMAND(wxID_ANY, wxEVT_COMMAND_WORKERTHREAD_COMPLETED, MainFrame::OnCalculationFinished)
END_EVENT_TABLE()

////////////////////////////////
// METHODS FOR EVENT HANDLING //
////////////////////////////////

int MainApp::OnExit(){
  return 0;
}

void MainFrame::OnClose(wxCloseEvent& event){
  if (_abort_q->IsOk()){
    _abort_q->Post(true);
  }
  if (GetThread() && GetThread()->IsRunning()){
    GetThread()->Wait();
  }
  event.Skip();
}

// begin calculation
void MainFrame::OnCalc(wxCommandEvent& event){
  // stop calculation if probe 2 radius is too small in two probes mode
  if(getProbeMode() && getProbe1Radius() > getProbe2Radius()){
    Ctrl::getInstance()->displayErrorMessage(104);
    wxYield(); // without wxYield, the clicks on disabled buttons are queued
    enableGuiElements(true);
    return;
  }

  enableGuiElements(false);
  abortButton->Enable(true);
  wxYield(); // without wxYield, the clicks on disabled buttons are queued

  // create worker thread
  if (CreateThread(wxTHREAD_JOINABLE) != wxTHREAD_NO_ERROR){
    wxLogError("Could not create worker thread!");
    return;
  }
  // start calculation in worker thread
  if (GetThread()->Run() != wxTHREAD_NO_ERROR){
    wxLogError("Could not run worker thread!");
  }
}

wxThread::ExitCode MainFrame::Entry(){
  // worker thread
  Ctrl::getInstance()->runCalculation();
  // after the calculation is finished, this event tells the main thread to give the stop signal
  // and reenable the GUI
  wxQueueEvent(this, new wxThreadEvent(wxEVT_COMMAND_WORKERTHREAD_COMPLETED));
  // give main thread a millisecond to give stop signal. this avoids starting the while loop again
  GetThread()->Sleep(1);
  return (wxThread::ExitCode)0;
}

void MainFrame::OnAbort(wxCommandEvent& event){
  if (_abort_q->IsOk()){
    _abort_q->Post(true);
  }
  if (GetThread() && GetThread()->IsRunning()){
    GetThread()->Wait();
  }
}

bool MainFrame::receivedAbortCommand(){
  bool abort = false;
  _abort_q->ReceiveTimeout(0,abort);
  return abort;
}

void MainFrame::OnCalculationFinished(wxCommandEvent& event){
  _abort_q->Clear();
  // main thread will wait for the thread to finish its work
  if (GetThread() && GetThread()->IsRunning()){
    GetThread()->Wait();
  }

  setDefaultState(reportButton,true);
  setDefaultState(totalMapButton,true);
  setDefaultState(cavityMapButton, outputGrid->GetNumberRows() != 0);
  // reenable GUI
  enableGuiElements(true);
}

// load input files to display radii list
void MainFrame::OnLoadFiles(wxCommandEvent& event){
  enableGuiElements(false);

  Ctrl::getInstance()->loadElementsFile();
  Ctrl::getInstance()->loadAtomFile();

  Ctrl::getInstance()->newCalculation();

  // reset output
  clearOutputText();
  clearOutputGrid();
  toggleButtons(); // sets accessibility of buttons

  wxYield();
  enableGuiElements(true);
}

// browse for atom file
void MainFrame::OnAtomBrowse(wxCommandEvent& event){
  std::string filetype = "XYZ, PDB, CIF files (*.xyz;*.pdb;*.cif)|*.xyz;*pdb;*.cif";
  OnBrowse(event, filetype, filepathText);
  // if user selects a .pdb or .cif file, then the related options are unlocked
  toggleOptionsFiletype();
  enableGuiElements(true);
}

// browse for elements file
void MainFrame::OnElementsBrowse(wxCommandEvent& event){
  std::string filetype = "TXT files (*.txt)|*.txt";
  OnBrowse(event, filetype, elementspathText);
}

// browse (can only be called by another method function)
void MainFrame::OnBrowse(wxCommandEvent& event, std::string& filetype, wxTextCtrl* textbox){
  wxFileDialog openFileDialog(this,_("Select file"),textbox->GetValue(),"",filetype,wxFD_OPEN|wxFD_FILE_MUST_EXIST);

  // if user closes dialog
  if (openFileDialog.ShowModal() == wxID_CANCEL)
    return;

  // proceed loading the file chosen by the user;
  wxFileInputStream input_stream(openFileDialog.GetPath());
  // if filepath is invalid
  if (!input_stream.IsOk())
  {
    wxLogError("Cannot open file '%s'.", openFileDialog.GetPath());
    return;
  }

  textbox->SetValue(openFileDialog.GetPath());
  // import files after file selection
  if (!getAtomFilepath().empty() && !getElementsFilepath().empty()){
    OnLoadFiles(event);
  }
}

// toggle the probe 2 radius box depending on the probe mode
void MainFrame::ProbeModeChange(wxCommandEvent& event){
  probe2InputText->Enable(event.IsChecked());
  setDefaultState(probe2InputText, event.IsChecked());
}

// Functions to dynamically change the color of the atom list grid cells
void MainFrame::GridChange(wxGridEvent& event){
  int col = event.GetCol();
  int row = event.GetRow();
  wxString value = event.GetString();
  if (col == 0){
    if (value == "1"){
      atomListGrid->SetCellBackgroundColour(row,1,wxNullColour);
      atomListGrid->SetCellBackgroundColour(row,2,wxNullColour);
    }
    else {
      atomListGrid->SetCellBackgroundColour(row,1,_col_grey_cell);
      atomListGrid->SetCellBackgroundColour(row,2,_col_grey_cell);
    }
  }
  else if (col == 3){
    if (wcstod(value,NULL) == 0){
      atomListGrid->SetCellBackgroundColour(row,3,_col_red_cell);
    }
    else {
      atomListGrid->SetCellBackgroundColour(row,3,_col_cyan_cell);
    }
  }
  atomListGrid->ForceRefresh();
}

static bool s_first_call = true;
void MainFrame::OnTextInput(wxCommandEvent& event){
  if (s_first_call){
    s_first_call = !s_first_call;
    return;
  }

  wxTextEntry* text_ctrls[] = {
    filepathText,
    elementspathText,
    probe1DropDown,
    probe2InputText,
    gridsizeInputText
  };

  for (auto& elem : text_ctrls){
    elem->ChangeValue(elem->GetValue());
  }
}

///////////////
// ON EXPORT //
///////////////

std::string MainFrame::OpenExportFileDialog(const std::string file_type, const std::string file_extension){
  // open file dialog
  wxFileDialog save_dialog(this, _("Export " + file_type + " as..."), "", "", file_extension, wxFD_SAVE);

  // if user closes dialog
  if (save_dialog.ShowModal() == wxID_CANCEL) {return "";}

  return save_dialog.GetPath().ToStdString();
}

void MainFrame::OnExportReport(wxCommandEvent& event){
  const std::string file = "report";
  // check whether report can be generated
  if (!Ctrl::getInstance()->isCalculationDone()){
    Ctrl::getInstance()->displayErrorMessage(301);
    return;
  }
  std::string path = OpenExportFileDialog(file, "*.txt");
  if (path.empty()) {return;}

  // create report
  Ctrl::getInstance()->exportReport(path);
}

void MainFrame::OnExportTotalMap(wxCommandEvent& event){
  const std::string file = "total surface map";
  if (!Ctrl::getInstance()->isCalculationDone()){
    Ctrl::getInstance()->displayErrorMessage(301);
    return;
  }

  std::string path = OpenExportFileDialog(file, "*.dx");
  if (path.empty()) {return;}

  Ctrl::getInstance()->exportSurfaceMap(path, false);
}

void MainFrame::OnExportCavityMap(wxCommandEvent& event){
  const std::string file = "cavity surface maps";
  if (!Ctrl::getInstance()->isCalculationDone() || outputGrid->GetNumberRows() == 0){
    Ctrl::getInstance()->displayErrorMessage(301);
    return;
  }

  std::string path = OpenExportFileDialog(file, "*.dx");
  if (path.empty()) {return;}

  Ctrl::getInstance()->exportSurfaceMap(path, true);
}

void MainFrame::OnToggleAutoExport(wxCommandEvent& event){
  if(dirpickerText->GetValue().IsNull()){
    const int checkbox_id = event.GetId();
    switch(checkbox_id){
      case CHECKBOX_Report:
        if (!getMakeReport()){return;}
        break;
      case CHECKBOX_SurfaceMap:
        if (!getMakeSurfaceMap()){return;}
        break;
      case CHECKBOX_CavityMaps:
        if (!getMakeCavityMaps()){return;}
        break;
    }
    OnBrowseOutput(event);
  }
}

void MainFrame::OnBrowseOutput(wxCommandEvent& event){
  wxDirDialog openDirDialog(this, _("Select output directory"), dirpickerText->GetValue(), wxDD_DIR_MUST_EXIST | wxDD_DEFAULT_STYLE);

  // if user closes dialog
  if (openDirDialog.ShowModal() == wxID_CANCEL){
    // if directory textbox is empty then uncheck all auto export tick boxes
    if (dirpickerText->GetValue().IsNull()){
      for (wxCheckBox* cb : getAutoExportCheckBoxes()){cb->SetValue(false);}
    }
    return;
  }

  dirpickerText->SetValue(openDirDialog.GetPath());
}

////////////////////////////////
// GUI enabling and disabling //
////////////////////////////////

void MainFrame::enableGuiElements(bool inp){
  // disables all interactable widgets listed in InitDefaultStates() in base_init.cpp
  // enables all widgets whose default state has been set to true
  for (std::map<wxWindow*, bool>::iterator it = default_states.begin(); it != default_states.end(); it++){
    it->first->Enable( inp ? it->second : false );
  }
}

void MainFrame::setDefaultState(wxWindow* widget, bool state){
  default_states[widget] = state;
}

void MainFrame::toggleOptionsFiletype(){
  std::string file_type = fileExtension(getAtomFilepath());
  setDefaultState(pdbHetatmCheckbox, file_type == "pdb" );
  setDefaultState(unitCellCheckbox, file_type == "pdb" || file_type == "cif" );
  if(file_type != "pdb" && file_type != "cif"){
    unitCellCheckbox->SetValue(false);
  }
}

void MainFrame::toggleButtons(){
  setDefaultState(reportButton,false);
  setDefaultState(totalMapButton,false);
  setDefaultState(cavityMapButton,false);
  setDefaultState(loadFilesButton, true);
  setDefaultState(calcButton, atomListGrid->GetNumberRows() != 0);
}
