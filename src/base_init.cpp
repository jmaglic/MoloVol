#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#  include <wx/wx.h>
#endif

#include "base.h"
#include "wxVolumeRenderer.h"


IMPLEMENT_APP(MainApp)

/////////////////////////////
// MAIN APP IS INITIALISED //
/////////////////////////////

bool MainApp::OnInit(){
  // initialise a new MainFrame object and have MainWin point to that object
  MainFrame* MainWin = new MainFrame(_("CaVol"), wxDefaultPosition, wxDefaultSize);
  MainWin->SetBackgroundColour(col_win);
  // call member function of the MainFrame object to set visibility
  MainWin->Show(true);
  SetTopWindow(MainWin);
  return true;
};


// set default states of GUI elements here
void MainFrame::InitDefaultStates(){
  // set default accessibility of interactable gui controls  
  wxWindow* widgets_enabled[] = {
    browseButton,
    radiusButton,
    filepathText,
    radiuspathText,
    atomListGrid,
    gridsizeInputText,
    depthInput};
  wxWindow* widgets_disabled[] = {
    pdbHetatmCheckbox,
    loadFilesButton,
    calcButton};
  
  // initialise map
  for (auto i : widgets_enabled){
    default_states[i] = true;
  }
  for (auto i : widgets_disabled){
    default_states[i] = false;
  }
}

////////////////////////////////////
// INITIALISATION OF GUI ELEMENTS //
////////////////////////////////////

/////////////////////
// TOP LEVEL FRAME //
/////////////////////
void MainFrame::InitTopLevel(){
  // contains browse panel and atom list panel
  leftMainPanel = new wxPanel
    (this,
     PANEL_LeftMain,
     wxDefaultPosition,
     wxDefaultSize,
     wxTAB_TRAVERSAL
    );
  leftMainPanel->SetBackgroundColour(col_panel);

  // contains parameter panel and send-and-receive panel
  rightMainPanel = new wxPanel
    (this,
     PANEL_RightMain,
     wxDefaultPosition,
     wxDefaultSize,
     wxTAB_TRAVERSAL
    );
  rightMainPanel->SetBackgroundColour(col_panel);

  InitLeftMainPanel();
  InitRightMainPanel();

  wxBoxSizer *topLevelSizerH = new wxBoxSizer(wxHORIZONTAL);
  topLevelSizerH->Add(leftMainPanel,1,wxRIGHT | wxEXPAND,5);
  topLevelSizerH->Add(rightMainPanel,1,wxLEFT | wxEXPAND,5);
  SetSizerAndFit(topLevelSizerH);
  
  InitDefaultStates();
}

/////////////////////////
// SECOND LEVEL PANELS //
/////////////////////////

void MainFrame::InitLeftMainPanel(){
  // contains file panels and load button
  browsePanel = new wxPanel
    (leftMainPanel,
     PANEL_Browse,
     wxDefaultPosition,
     wxDefaultSize,
     wxTAB_TRAVERSAL
    );
  browsePanel->SetBackgroundColour(col_panel);
  browsePanel->SetMaxSize(wxSize(-1,160));

  // contains a grid widget that displays a table of atoms
  atomListPanel = new wxPanel
    (leftMainPanel,
     PANEL_AtomList,
     wxDefaultPosition,
     wxDefaultSize,
     wxTAB_TRAVERSAL);
  atomListPanel->SetBackgroundColour(col_panel);
  atomListPanel->SetMaxSize(wxSize(-1,200));

  InitBrowsePanel();
  InitAtomListPanel();

  wxBoxSizer *leftSizerV = new wxBoxSizer(wxVERTICAL);
  leftSizerV->Add(browsePanel,1,wxEXPAND,20);
  leftSizerV->Add(atomListPanel,1,wxEXPAND,20);
  leftMainPanel->SetSizerAndFit(leftSizerV);
}

void MainFrame::InitRightMainPanel(){
  // contains panels for user input
  parameterPanel = new wxPanel
    (rightMainPanel,
     PANEL_Parameters,
     wxDefaultPosition,
     wxDefaultSize,
     wxTAB_TRAVERSAL);
  parameterPanel->SetBackgroundColour(col_panel);

  // contains calculate button and output text control
  sandrPanel = new wxPanel
    (rightMainPanel,
     PANEL_Sandr,
     wxDefaultPosition,
     wxDefaultSize,
     wxTAB_TRAVERSAL);
  sandrPanel->SetBackgroundColour(col_panel);

  InitParametersPanel();
  InitSandr();

  wxBoxSizer *rightSizerV = new wxBoxSizer(wxVERTICAL);
  rightSizerV->Add(parameterPanel,0,wxEXPAND,20);
  rightSizerV->Add(sandrPanel,0,wxEXPAND,20);
  rightMainPanel->SetSizerAndFit(rightSizerV);
}

//////////////////
// BROWSE PANEL //
//////////////////
void MainFrame::InitBrowsePanel(){
  atomfilePanel = new wxPanel(browsePanel, PANEL_Atomfile, wxDefaultPosition, wxDefaultSize, 0);
  radiusfilePanel = new wxPanel(browsePanel, PANEL_Radiusfile, wxDefaultPosition, wxDefaultSize, 0);
  fileOptionsPanel = new wxPanel(browsePanel, PANEL_FileOptions, wxDefaultPosition, wxDefaultSize, 0);

  InitAtomfilePanel();
  InitRadiusfilePanel();
  InitFileOptionsPanel();

  wxStaticBoxSizer *browserSizer = new wxStaticBoxSizer(wxVERTICAL,browsePanel);
  browserSizer->Add(radiusfilePanel,0,wxEXPAND,0);
  browserSizer->Add(atomfilePanel,0,wxEXPAND,0);
  browserSizer->Add(fileOptionsPanel,0,wxEXPAND,0);
  browsePanel->SetSizerAndFit(browserSizer);
}

////////////////////////////
// SEND AND RECEIVE PANEL //
////////////////////////////
void MainFrame::InitSandr(){
  calcButton = new wxButton
    (sandrPanel,
     BUTTON_Calc,
     "Calculate",
     wxDefaultPosition,
     wxDefaultSize,
     0,
     wxDefaultValidator,
     "begin calculation"
    );
	calcButton->Enable(false);

  outputText = new wxTextCtrl
    (sandrPanel,
     TEXT_Output,
     _("Output"),
     wxDefaultPosition,
     wxSize(-1,100), // height of the output text control
     wxTE_MULTILINE | wxTE_READONLY,
     wxDefaultValidator,
     "output result"
    );
  outputText->SetBackgroundColour(col_output);

  wxStaticBoxSizer *sandrSizer = new wxStaticBoxSizer(wxHORIZONTAL,sandrPanel);
  sandrSizer->Add(outputText,5,wxALIGN_LEFT | wxALL,10);
  sandrSizer->Add(calcButton,1,wxALIGN_CENTRE_VERTICAL | wxALL,10);
  sandrPanel->SetSizerAndFit(sandrSizer);
}

////////////////////
// FILE SELECTION //
////////////////////

// function used in InitAtomfilePanel and InitRadiusfilePanel to create and set the sizer
void MainFrame::SetSizerFilePanel(wxPanel* panel, wxButton* button, wxTextCtrl* text){
  wxBoxSizer *fileSizer = new wxBoxSizer(wxHORIZONTAL);
  fileSizer->Add(button,1,wxALIGN_LEFT | wxALIGN_CENTRE_VERTICAL | wxALL,10);
  fileSizer->Add(text,5,wxALL,10);
  panel->SetSizerAndFit(fileSizer);
}

void MainFrame::InitAtomfilePanel(){
  browseButton = new wxButton
    (atomfilePanel, BUTTON_Browse, "Browse", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator);

  filepathText = new wxTextCtrl(atomfilePanel,
								TEXT_Filename,
								wxEmptyString,
								wxDefaultPosition,
								wxDefaultSize,
								0,
								wxDefaultValidator);

  SetSizerFilePanel(atomfilePanel, browseButton, filepathText);
}

void MainFrame::InitRadiusfilePanel(){
  radiusButton = new wxButton
    (radiusfilePanel, BUTTON_Radius, "Browse");

  radiuspathText = new wxTextCtrl(radiusfilePanel,
								  TEXT_Radius,
								  "./inputfile/radii.txt");

  SetSizerFilePanel(radiusfilePanel, radiusButton, radiuspathText);
}

/////////////////////////////////////
// FILE SELECTION OPTIONS AND LOAD //
/////////////////////////////////////

void MainFrame::InitFileOptionsPanel(){
  pdbHetatmCheckbox = new wxCheckBox
    (fileOptionsPanel,
     CHECKBOX_Hetatm,
     "Include HETATM (for pdb files)",
     wxDefaultPosition,
     wxDefaultSize,
     0,
     wxDefaultValidator,
     "include HETATM"
    );
  pdbHetatmCheckbox->Enable(false);
  // Biochemists know what HETATM represent but other chemists might not
  // thus it is better to include HETATM by default as they are mostly useful for non-biochemists
  pdbHetatmCheckbox->SetValue(true);

  loadFilesButton = new wxButton
    (fileOptionsPanel,
     BUTTON_LoadFiles,
     "Reload",
     wxDefaultPosition,
     wxDefaultSize,
     0,
     wxDefaultValidator,
     "load input files"
    );
  loadFilesButton->Enable(false);

  wxBoxSizer *fileOptionsSizer = new wxBoxSizer(wxHORIZONTAL);
  fileOptionsSizer->Add(pdbHetatmCheckbox,1,wxALIGN_LEFT | wxALIGN_CENTRE_VERTICAL | wxALL,10);
  fileOptionsSizer->Add(loadFilesButton,1,wxALIGN_CENTRE_VERTICAL | wxALL,10);
  fileOptionsPanel->SetSizerAndFit(fileOptionsSizer);
}

//////////////////////
// PARAMETERS PANEL //
//////////////////////

void MainFrame::InitParametersPanel(){
  // contains input controls for grid size
  gridsizePanel = new wxPanel(parameterPanel, PANEL_Grid, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
  gridsizePanel->SetBackgroundColour(col_panel);

  // contains input controls for tree depth
  depthPanel = new wxPanel(parameterPanel, PANEL_Depth, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
  depthPanel->SetBackgroundColour(col_panel);

  InitGridPanel();
  InitDepthPanel();

  wxStaticBoxSizer *parameterSizer = new wxStaticBoxSizer(wxVERTICAL,parameterPanel);
  parameterSizer->Add(gridsizePanel,0,wxEXPAND,20);
  parameterSizer->Add(depthPanel,0,wxEXPAND,20);
  parameterPanel->SetSizerAndFit(parameterSizer);
}

void MainFrame::InitGridPanel(){
  gridsizeText = new wxStaticText(gridsizePanel, TEXT_Grid, "Grid step size:");

  // contains input control for grid size and text field for unit
  gridsizeInputPanel = new wxPanel(gridsizePanel, PANEL_Depth, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
  gridsizeInputPanel->SetBackgroundColour(col_panel);

  InitGridinputPanel();

  wxBoxSizer *gridsizeSizer = new wxBoxSizer(wxHORIZONTAL);
  gridsizeSizer->Add(gridsizeText,0,wxALL | wxALIGN_CENTRE_VERTICAL,10);
  gridsizeSizer->Add(gridsizeInputPanel,0,0,0);
  gridsizePanel->SetSizerAndFit(gridsizeSizer);
}

void MainFrame::InitGridinputPanel(){
  gridsizeInputText = new wxTextCtrl(gridsizeInputPanel, TEXT_Gridinput, "0.1");

  gridsizeUnitText = new wxStaticText(gridsizeInputPanel, TEXT_Gridunit, L"\u212B "); // unicode for angstrom

  wxBoxSizer *gridsizeInputsizer = new wxBoxSizer(wxHORIZONTAL);
  gridsizeInputsizer->Add(gridsizeInputText, 0, wxALL | wxALIGN_CENTRE_VERTICAL, 10);
  gridsizeInputsizer->Add(gridsizeUnitText, 0, wxALL | wxALIGN_CENTRE_VERTICAL, 10);
  gridsizeInputPanel->SetSizerAndFit(gridsizeInputsizer);
}

void MainFrame::InitDepthPanel(){
  depthText = new wxStaticText(depthPanel, TEXT_Depth, "Maximum tree depth:");

  depthInput = new wxSpinCtrl
    (depthPanel, SPIN_Depthinput, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 20, 4);

  wxBoxSizer *depthSizer = new wxBoxSizer(wxHORIZONTAL);
  depthSizer->Add(depthText,0,wxALIGN_CENTRE_VERTICAL | wxALL,10);
  depthSizer->Add(depthInput,0,wxALIGN_CENTRE_VERTICAL | wxALL,10);
  depthPanel->SetSizerAndFit(depthSizer);
}

/////////////////////
// ATOM LIST PANEL //
/////////////////////

void MainFrame::InitAtomListPanel(){
  atomListGrid = new wxGrid(atomListPanel, GRID_AtomList, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS);
  atomListGrid->CreateGrid(0, 4, wxGrid::wxGridSelectCells);
  atomListGrid->SetDefaultCellAlignment (wxALIGN_CENTRE, wxALIGN_CENTRE);

  // columns
  atomListGrid->SetColLabelValue(0, "Include");
  atomListGrid->SetColFormatBool(0);

  atomListGrid->SetColLabelValue(1, "Element");

  atomListGrid->SetColLabelValue(2, "Number \nof Atoms");
  atomListGrid->SetColFormatNumber(2);

  atomListGrid->SetColLabelValue(3, L"Radius (\u212B)");
  atomListGrid->SetColFormatFloat(3, 5, 3); // 2nd argument is width, last argument is precision

  wxStaticBoxSizer *atomListSizer = new wxStaticBoxSizer(wxVERTICAL,atomListPanel);
  atomListSizer->Add(atomListGrid,1,wxEXPAND,20); // proportion factor has to be 1, else atom list does not expand
  atomListPanel->SetSizerAndFit(atomListSizer);
}

