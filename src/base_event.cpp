#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#  include <wx/wx.h>
#endif

#include "base.h"
#include "controller.h"
#include "misc.h"
#include <vector>

/////////////////
// EVENT TABLE //
/////////////////

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
  EVT_BUTTON(BUTTON_Calc, MainFrame::OnCalc)
  EVT_BUTTON(BUTTON_Browse, MainFrame::OnAtomBrowse)
  EVT_BUTTON(BUTTON_Radius, MainFrame::OnRadiusBrowse)
  EVT_BUTTON(BUTTON_LoadFiles, MainFrame::OnLoadFiles)
  EVT_GRID_CELL_CHANGING(MainFrame::GridChange)
END_EVENT_TABLE()

////////////////////////////////
// METHODS FOR EVENT HANDLING //
////////////////////////////////

// exit program
void MainFrame::OnExit(wxCommandEvent& event){
  this->Close(TRUE);
}

// display text in output, used for debugging
void MainFrame::OnPrint(wxCommandEvent& event){
  std::string text = "treat yourself well";
  printToOutput(text);
}

void MainFrame::showRendering(){
	//show rendering
	auto width = 512;
	auto height = 512;
	wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
	frame = new wxFrame(NULL, wxID_ANY, wxT("3D Renderer"), wxPoint(50,50), wxSize(width,height));
	try {
		drawPane = new wxVolumeRenderer( frame, wxBITMAP_TYPE_JPEG);
		sizer->Add(drawPane, 1, wxEXPAND);
	} catch(const std::runtime_error& e){
		frame->SetLabel(e.what());//todo add empty image when error
	}
	frame->SetSizer(sizer);
	frame->Show();
}

// begin calculation
void MainFrame::OnCalc(wxCommandEvent& event){
  enableGuiElements(false);

  Ctrl::getInstance()->runCalculation();
  showRendering();
  // without wxYield, the clicks on disabled buttons are queued
  enableGuiElements(true);
}

// load input files to display radii list
void MainFrame::OnLoadFiles(wxCommandEvent& event){
  enableGuiElements(false);

  Ctrl::getInstance()->loadRadiusFile();
  Ctrl::getInstance()->loadAtomFile();

  toggleButtons(); // sets accessibility of buttons
  wxYield();
  enableGuiElements(true);
}

// browse for atom file
void MainFrame::OnAtomBrowse(wxCommandEvent& event){
  std::string filetype = "XYZ and PDB files (*.xyz;*.pdb)|*.xyz;*pdb";
  OnBrowse(event, filetype, filepathText);
  // if user selects a .pdb file, then the .pdf file options are unlocked
  toggleOptionsPdb();
  enableGuiElements(true);
}

// browse for radius file
void MainFrame::OnRadiusBrowse(wxCommandEvent& event){
  std::string filetype = "TXT files (*.txt)|*.txt";
  OnBrowse(event, filetype, radiuspathText);
}

// browse (can only be called by another method function)
void MainFrame::OnBrowse(wxCommandEvent& event, std::string& filetype, wxTextCtrl* textbox){
  wxFileDialog openFileDialog
    (this,
     _("Select File"),
     "",
     "",
     filetype,
     wxFD_OPEN|wxFD_FILE_MUST_EXIST,
     wxDefaultPosition,
     wxDefaultSize,
     "file browser"
    );

  // if user closes dialogue
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

  textbox->SetLabel(openFileDialog.GetPath());
  // import files after file selection
  if (!getAtomFilepath().empty() && !getRadiusFilepath().empty()){
    OnLoadFiles(event);
  }
}

// Functions to dynamically change the color of the atom list grid cells
void MainFrame::GridChange(wxGridEvent& event){
  int col = event.GetCol();
  int row = event.GetRow();
  wxString value = event.GetString();
  if (col == 0){
    if (value == "1"){ //
      atomListGrid->SetCellBackgroundColour(row,1,col_white);
      atomListGrid->SetCellBackgroundColour(row,2,col_white);
    }
    else {
      atomListGrid->SetCellBackgroundColour(row,1,col_grey_cell);
      atomListGrid->SetCellBackgroundColour(row,2,col_grey_cell);
    }
  }
  else if (col == 3){
    if (wcstod(value,NULL) == 0){
      atomListGrid->SetCellBackgroundColour(row,3,col_red_cell);
    }
    else {
      atomListGrid->SetCellBackgroundColour(row,3,col_cyan_cell);
    }
  }
  atomListGrid->ForceRefresh();
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

void MainFrame::toggleOptionsPdb(){
  setDefaultState(pdbHetatmCheckbox, fileExtension(getAtomFilepath()) == "pdb" );
}

void MainFrame::toggleButtons(){
  setDefaultState(loadFilesButton, true);
  setDefaultState(calcButton, atomListGrid->GetNumberRows() != 0);
}

