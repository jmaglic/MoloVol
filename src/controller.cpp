
#include "controller.h"
#include "base.h"
#include "atom.h" // i don't know why
#include "model.h"
#include "misc.h"
#include "exception.h"
#include "special_chars.h"
#include <chrono>
#include <utility>
#include <map>

///////////////////////
// STATIC ATTRIBUTES //
///////////////////////

Ctrl* Ctrl::s_instance = NULL;
MainFrame* Ctrl::s_gui = NULL;

///////////////////////////
// STATIC MEMBERS ACCESS //
///////////////////////////

std::string Ctrl::getDefaultElemPath(){
  return getResourcesDir() + "/" + s_elem_file;
}

std::string Ctrl::getVersion(){
  return s_version;
}

////////////////
// TOGGLE CLI //
////////////////

void Ctrl::hush(const bool quiet){
  _quiet = quiet;
}

void Ctrl::version(){
  notifyUser("Version: " + getVersion() + "\n");
}

////////////////
// TOGGLE GUI //
////////////////

void Ctrl::disableGUI(){
  _to_gui = false;
}

void Ctrl::enableGUI(){
  _to_gui = true;
}

bool Ctrl::isGUIEnabled(){
  return _to_gui;
}

/////////////
// METHODS //
/////////////

void Ctrl::registerView(MainFrame* inp_gui){
  s_gui = inp_gui;
}

Ctrl* Ctrl::getInstance(){
  if(s_instance == NULL){
    s_instance = new Ctrl();
  }
  return s_instance;
}

bool Ctrl::loadElementsFile(){
  // create an instance of the model class
  // ensures, that there is only ever one instance of the model class
  if(_current_calculation == NULL){
    _current_calculation = new Model();
  }

  std::string elements_filepath = s_gui->getElementsFilepath();
  // even if there is no valid radii file, the program can be used by manually setting radii in the GUI after loading a structure
  if(!_current_calculation->importElemFile(elements_filepath)){
    displayErrorMessage(101);
  }
  // refresh atom list using new radius map
  s_gui->displayAtomList(_current_calculation->generateAtomList());
  return true;
}

bool Ctrl::loadAtomFile(){
  // create an instance of the model class
  // ensures, that there is only ever one instance of the model class
  if(_current_calculation == NULL){
    _current_calculation = new Model();
  }

  bool successful_import;
  try{successful_import = _current_calculation->readAtomsFromFile(s_gui->getAtomFilepath(), s_gui->getIncludeHetatm());}

  catch (const ExceptIllegalFileExtension& e){
    displayErrorMessage(103);
    successful_import = false;
  }
  catch (const ExceptInvalidInputFile& e){
    displayErrorMessage(102);
    successful_import = false;
  }
  catch (const ExceptInvalidCellParams& e){
    displayErrorMessage(109);
    successful_import = false;
  }

  s_gui->displayAtomList(_current_calculation->generateAtomList()); // update gui

  return successful_import;
}

/////////////////
// CALCULATION //
/////////////////

// default function call: transfer data from GUI to Model
bool Ctrl::runCalculation(){
  // reset abort flag
  setAbortFlag(false);
  s_gui->extSetProgressBar(0);
  // create an instance of the model class
  // ensures, that there is only ever one instance of the model class
  if(_current_calculation == NULL){
    _current_calculation = new Model();
  }

  // PARAMETERS
  // save parameters in model
  try{
    if(!_current_calculation->setParameters(
        s_gui->getAtomFilepath(),
        s_gui->getOutputDir(),
        s_gui->getIncludeHetatm(),
        s_gui->getAnalyzeUnitCell(),
        s_gui->getCalcSurfaceAreas(),
        s_gui->getProbeMode(),
        s_gui->getProbe1Radius(),
        s_gui->getProbe2Radius(),
        s_gui->getGridsize(),
        s_gui->getDepth(),
        s_gui->getMakeReport(),
        s_gui->getMakeSurfaceMap(),
        s_gui->getMakeCavityMaps(),
        s_gui->generateRadiusMap(),
        s_gui->getIncludedElements())){
      return false;
    }
  }
  catch (const std::invalid_argument& e){
    displayErrorMessage(109);
    return false;
  }

  // CALCULATION
  CalcReportBundle data = _current_calculation->generateData();
  calculationDone(data.success);

  updateStatus((data.success && !Ctrl::getInstance()->getAbortFlag())? "Calculation done." : "Calculation aborted.");

  // OUTPUT
  displayResults(data);

  if (data.success){
    // export if appropriate option is toggled
    if(data.make_report){exportReport();}
    if(data.make_full_map){exportSurfaceMap(false);}
    if(data.make_cav_maps){exportSurfaceMap(true);}
  }

  return data.success;
}

// for starting a calculation from the command line
bool Ctrl::runCalculation(
    const double probe_radius_s,
    const double probe_radius_l,
    const double grid_resolution,
    const std::string& structure_file_path,
    const std::string& elements_file_path,
    const std::string& output_dir_path,
    const int tree_depth,
    const bool opt_include_hetatm,
    const bool opt_unit_cell,
    const bool opt_surface_area,
    const bool opt_probe_mode,
    const bool exp_report,
    const bool exp_total_map,
    const bool exp_cavity_maps,
    const unsigned display_flag){
  if(_current_calculation == NULL){_current_calculation = new Model();}

  try{_current_calculation->readAtomsFromFile(structure_file_path, opt_include_hetatm);}
  catch (const ExceptInvalidInputFile& e){
    displayErrorMessage(102);
    return false;
  }
  catch (const ExceptIllegalFileExtension& e){
    displayErrorMessage(103);
    return false;
  }
  catch (const ExceptInvalidCellParams& e){
    displayErrorMessage(109);
    return false;
  }

  std::vector<std::string> included_elements = _current_calculation->listElementsInStructure();

  if(!_current_calculation->importElemFile(elements_file_path)){
    displayErrorMessage(903);
    return false;
  }

  _current_calculation->setParameters(
    structure_file_path,
    output_dir_path,
    opt_include_hetatm,
    opt_unit_cell,
    opt_surface_area,
    opt_probe_mode,
    probe_radius_s,
    probe_radius_l,
    grid_resolution,
    tree_depth,
    exp_report,
    exp_total_map,
    exp_cavity_maps,
    _current_calculation->getRadiusMap(),
    _current_calculation->listElementsInStructure());

  CalcReportBundle data = _current_calculation->generateData();

  updateStatus((data.success && !Ctrl::getInstance()->getAbortFlag())? "Calculation done." : "Calculation aborted.");

  displayInput(data, display_flag);
  displayResults(data, display_flag);

  if (data.success){
    // export if appropriate option is toggled
    if(data.make_report){exportReport();}
    if(data.make_full_map){exportSurfaceMap(false);}
    if(data.make_cav_maps){exportSurfaceMap(true);}
  }
  return data.success;
}

////////////////////////
// USER COMMUNICATION //
////////////////////////

void Ctrl::clearOutput(){
  if (_to_gui) {
    s_gui->extClearOutputText();
    s_gui->extClearOutputGrid();
  }
}

void Ctrl::displayInput(CalcReportBundle& data, const unsigned display_flag){
  auto yesno = [](bool b){
    return std::string(b? "yes" : "no");
  };
  if (display_flag & (mvOUT_INP | mvOUT_OPT)){
    if(!_to_gui){
      if (display_flag & mvOUT_INP){
        notifyUser("<INPUT>\n");
        if (display_flag & mvOUT_STRUCTURE){
          notifyUser("Structure file: " + data.atom_file_path + "\n");
        }
        if (display_flag & mvOUT_RESOLUTION){
          notifyUser("Grid resolution: " + std::to_string(data.grid_step) + " ");
          notifyUser(Symbol::angstrom());
          notifyUser("\n");
        }
        if (display_flag & mvOUT_DEPTH){
          notifyUser("Octree depth: " + std::to_string(data.max_depth) + "\n");
        }
        if (data.probe_mode){
          if (display_flag & mvOUT_RADIUS_S){
            notifyUser("Small probe radius: " + std::to_string(data.r_probe1) + " ");
            notifyUser(Symbol::angstrom());
            notifyUser("\n");
          }
          if (display_flag & mvOUT_RADIUS_L){
            notifyUser("Large probe radius: " + std::to_string(data.r_probe2) + " ");
            notifyUser(Symbol::angstrom());
            notifyUser("\n");
          }
        }
        else{
          if (display_flag & mvOUT_RADIUS_S){
            notifyUser("Probe radius: " + std::to_string(data.r_probe1) + " ");
            notifyUser(Symbol::angstrom());
            notifyUser("\n");
          }
        }
      }
      if (display_flag & mvOUT_OPT){
        notifyUser("<OPTIONS>\n");
        if (fileExtension(data.atom_file_path)=="pdb"){
          if (display_flag & mvOUT_OPT_HETATM){
            notifyUser("Include HETATM: " + yesno(data.inc_hetatm) + "\n");
          }
          if (display_flag & mvOUT_OPT_UNITCELL){
            notifyUser("Analyze unit cell: " + yesno(data.analyze_unit_cell) + "\n");
          }
        }
        if (display_flag & mvOUT_OPT_PROBEMODE){
          notifyUser("Enable two-probe mode: " + yesno(data.probe_mode) + "\n");
        }
        if (display_flag & mvOUT_OPT_SURFACE){
          notifyUser("Calculate surface areas: " + yesno(data.calc_surface_areas) + "\n");
        }
      }
    }
  }
}

void Ctrl::displayResults(CalcReportBundle& data, const unsigned display_flag){
  clearOutput();
  if (display_flag & (mvOUT_FORMULA | mvOUT_TIME)){
    if(!_to_gui){notifyUser("<OUTPUT>\n");}
  }
  if(data.success){
    std::wstring vol_unit = Symbol::angstrom() + Symbol::cubed();

    if (display_flag & mvOUT_FORMULA){
      notifyUser("Result for: ");
      notifyUser(Symbol::generateChemicalFormulaUnicode(data.chemical_formula));
      notifyUser("\n");
    }
    if (display_flag & mvOUT_TIME){
      notifyUser("Elapsed time: " + std::to_string(data.getTime()) + " s\n");
    }

    if (display_flag & mvOUT_VOL){
      notifyUser("<VOLUME>\n");
    }
    if (display_flag & mvOUT_VOL_VDW){
      notifyUser("Van der Waals volume: " + std::to_string(data.volumes[0b00000011]) + " ");
      notifyUser(vol_unit);
      notifyUser("\n");
    }
    if (display_flag & mvOUT_VOL_INACCESSIBLE){
      notifyUser("Probe inaccessible volume: " + std::to_string(data.volumes[0b00000101]) + " ");
      notifyUser(vol_unit);
      notifyUser("\n");
    }
    std::string prefix = data.probe_mode? "Small p" : "P";
    if (display_flag & mvOUT_VOL_CORE_S){
      if(!data.probe_mode && !data.analyze_unit_cell){
        notifyUser(prefix +"robe core volume: No physical meaning, contains all volume outside the structure.\n");
      }
      else{
        notifyUser(prefix +"robe core volume: " + std::to_string(data.volumes[0b00001001]) + " ");
        notifyUser(vol_unit);
        notifyUser("\n");
      }
    }
    if (display_flag & mvOUT_VOL_SHELL_S){
      notifyUser(prefix +"robe shell volume: " + std::to_string(data.volumes[0b00010001]) + " ");
      notifyUser(vol_unit);
      notifyUser("\n");
    }
    if(data.probe_mode){
      if (display_flag & mvOUT_VOL_SHELL_L){
        if(!data.analyze_unit_cell){
          notifyUser("Large probe core volume: No physical meaning, contains all volume outside the structure.\n");
        }
        else{
          notifyUser("Large probe core volume: " + std::to_string(data.volumes[0b00100001]) + " ");
          notifyUser(vol_unit);
          notifyUser("\n");
        }
      }
      if (display_flag & mvOUT_VOL_SHELL_L){
        notifyUser("Large probe shell volume: " + std::to_string(data.volumes[0b01000001]) + " ");
        notifyUser(vol_unit);
        notifyUser("\n");
      }
    }
    if(data.calc_surface_areas && !Ctrl::getInstance()->getAbortFlag()){
      std::wstring surf_unit = Symbol::angstrom() + Symbol::squared();
      if (display_flag & mvOUT_SURF){
        notifyUser("<SURFACE>\n");
      }
      if (display_flag & mvOUT_SURF_VDW){
        notifyUser("Van der Waals surface: " + std::to_string(data.getSurfVdw()) + " ");
        notifyUser(surf_unit);
        notifyUser("\n");
      }
      if(data.probe_mode){
        if (display_flag & mvOUT_SURF_MOL){
          notifyUser("Molecular surface: " + std::to_string(data.getSurfMolecular()) + " ");
          notifyUser(surf_unit);
          notifyUser("\n");
        }
      }
      if (display_flag & mvOUT_SURF_EXCLUDED_S){
        notifyUser(prefix+"robe excluded surface: " + std::to_string(data.getSurfProbeExcluded()) + " ");
        notifyUser(surf_unit);
        notifyUser("\n");
      }
      if (display_flag & mvOUT_SURF_ACCESSIBLE_S){
        notifyUser(prefix+"robe accessible surface: " + std::to_string(data.getSurfProbeAccessible()) + " ");
        notifyUser(surf_unit);
        notifyUser("\n");
      }
    }
    if (display_flag & mvOUT_CAVITIES){
      if(!data.cavities.empty()){displayCavityList(data);}
    }
  }
  else{
    if(!getAbortFlag()){displayErrorMessage(200);}
  }
}

void Ctrl::displayCavityList(CalcReportBundle& data, const unsigned display_flag){
  if (_to_gui){
    s_gui->extDisplayCavityList(data.cavities, data.analyze_unit_cell, data.probe_mode);
  }
  else{
    std::wstring vol_unit = Symbol::angstrom() + Symbol::cubed();
    std::wstring surf_unit = Symbol::angstrom() + Symbol::squared();

    int width = 20;

    std::string header[] = {"Cavity ID", "Volume", "Core Surface", "Shell Surface", "Position"};
    std::wstring units[] = {L"", L"(" + vol_unit + L")",L"(" + surf_unit + L")", L"(" + surf_unit + L")", L"(" + Symbol::angstrom() + L"," + Symbol::angstrom() + L"," + Symbol::angstrom() + L")"};

    // HEADER
    for (std::string elem : header){
      notifyUser(field(width,elem));
    }
    notifyUser("\n");
    // UNITS
    for (std::wstring elem : units){
      notifyUser(wfield(width,elem));
    }
    notifyUser("\n");

    for (size_t i = 0; i < data.cavities.size(); ++i){
      // CAVITY VALUES
      // in single probe mode, the first cavity with id 1 comprises outside empty space and is meaningless
      std::string volume = (!data.probe_mode && !data.analyze_unit_cell && data.cavities[i].id == 1)? "outside"
        : std::to_string(data.cavities[i].getVolume());
      std::string values[] = {std::to_string(i+1), volume, std::to_string(data.cavities[i].getSurfCore()), std::to_string(data.cavities[i].getSurfShell()), data.cavities[i].getPosition()};

      for (std::string elem : values){
        notifyUser(field(width,elem));
      }
      notifyUser("\n");
    }
  }
}

void Ctrl::notifyUser(std::string str){
  if (_to_gui){
    s_gui->extAppendOutput(str);
  }
  else {
    std::cout << str;
  }
}

void Ctrl::notifyUser(std::wstring wstr){
  if (_to_gui){
    s_gui->extAppendOutputW(wstr);
  }
  else {
    std::cout << wstr;
  }
}

void Ctrl::updateStatus(const std::string str){
  if (_to_gui) {
    s_gui->extSetStatus(str);
  }
  else if(_quiet) {}
  else{
    std::cout << str << std::endl;
  }
}

void Ctrl::updateProgressBar(const int percentage){
  assert (percentage <= 100);
  if (_to_gui) {
    s_gui->extSetProgressBar(percentage);
  }
  else if(_quiet) {}
  else {
    std::cout << std::to_string(percentage) + "\%"  << std::endl;
  }
}

////////////
// EXPORT //
////////////

void Ctrl::exportReport(std::string path){
  _current_calculation->createReport(path);
  if(_current_calculation->optionAnalyzeUnitCell()){
    _current_calculation->writeCrystStruct(path);
  }
}

void Ctrl::exportReport(){
  _current_calculation->createReport();
  if(_current_calculation->optionAnalyzeUnitCell()){
    _current_calculation->writeCrystStruct();
  }
}

void Ctrl::exportSurfaceMap(const std::string path, bool cavities){
  if(cavities){
    _current_calculation->writeCavitiesMaps(path);
  }
  else{
    _current_calculation->writeTotalSurfaceMap(path);
  }
}

void Ctrl::exportSurfaceMap(bool cavities){
  if(cavities){
    _current_calculation->writeCavitiesMaps();
  }
  else{
    _current_calculation->writeTotalSurfaceMap();
  }
}

////////////////////////
// CALCULATION STATUS //
////////////////////////

void Ctrl::newCalculation(){
  _calculation_finished = false;
}

void Ctrl::calculationDone(const bool state){
  _calculation_finished = state;
}

bool Ctrl::isCalculationDone(){
  return _calculation_finished;
}

void Ctrl::setAbortFlag(const bool state){
  _abort_calculation = state;
}

bool Ctrl::getAbortFlag(){
  return _abort_calculation;
}

// checks whether worker thread has received a signal to stop the calculation and
// updates the progress of the calculation
void Ctrl::updateCalculationStatus(){
  if (_to_gui){
    setAbortFlag(s_gui->receivedAbortCommand());
  }
}

////////////////////
// ERROR MESSAGES //
////////////////////

static const std::map<int, std::string> s_error_codes = {
  {0, "Unidentified error code."}, // no user should ever see this
  // 1xx: Invalid Input
  {100, "Import failed!"},
  {101, "Invalid elements file. Please select a valid file or set radii manually. Atomic weights will be set to 0."},
  {102, "Invalid structure file. Please select a valid file. You may need to enable the option HETATM for PDB files."},
  {103, "Invalid file format. Please make sure that the input files have the correct file extensions."},
  {104, "Invalid probe radius input. The large probe must have a larger radius than the small probe."},
  {105, "Invalid entry in structure file encountered. Some atoms have not been imported. Please check the format of the input file."},
  {106, "Invalid element symbol(s) in elements file detected. Some radii may be assigned incorrectly. Please make sure that all element symbols begin with an alphabetic character."},
  {107, "Invalid radius value in elements file detected. Some radii may be set to 0. Please make sure that all radii are numeric."},
  {108, "Invalid atomic weight value in elements file detected. Some atomic weights may be set to 0. Please make sure that all atomic weights are numeric."},
  {109, "Invalid numeric input. Please make sure that all input values have a valid number format."},
  // 11x: unit cell files
  {111, "Space group not found. Check the structure file, or untick the Unit Cell Analysis tickbox."},
  {112, "Invalid unit cell parameters. Check the structure file, or untick the Unit Cell Analysis tickbox."},
  {113, "Space group or symmetry not found. Check the structure and space group files or untick the Unit Cell Analysis tickbox"},
  {114, "Invalid ATOM or HETATM line encountered. Import may be incomplete. Check the structure file."},
  {115, "Invalid option(s). You may have selected an option that is incompatible with the structure file format."},
  // 2xx: Issue during Calculation
  {200, "Calculation failed!"},
  {201, "Total number of cavities (255) exceeded. Consider changing the probe size. Calculation will proceed."},
  // 3xx: Issue with Output
  {300, "Output failed!"},
  {301, "Data missing to export file. Calculation may be still running or has not been started."},
  {302, "Invalid output directory. Please select a valid output directory."},
  {303, "An unidentified issue has been encountered while writing the surface map."},
  // 9xx: Issues with command line arguments
  {900, "Command line interface failed!"},
  {901, "At least one required command line argument missing."},
  {902, "Invalid output display option. At least one parameter belonging to '-o' is invalid and will be ignored."},
  {903, "Elements file import failed. Calculation aborted."}
};

void Ctrl::displayErrorMessage(const int error_code){
  if (_to_gui){
    s_gui->extOpenErrorDialog(error_code, getErrorMessage(error_code));
  }
  else{
    printErrorMessage(error_code);
  }
}

void Ctrl::printErrorMessage(const int error_code){
  std::cout << error_code << ": " << getErrorMessage(error_code) << std::endl;
}

std::string Ctrl::getErrorMessage(const int error_code){
  if (s_error_codes.find(error_code) == s_error_codes.end()) {
    return s_error_codes.find(0)->second;
  }
  else {
    return s_error_codes.find(error_code)->second;
  }
}
