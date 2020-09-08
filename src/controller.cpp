
#include "controller.h"
#include "base.h"
#include "atom.h" // i don't know why
#include "model.h"
#include <chrono>
#include <utility>

///////////////////////
// STATIC ATTRIBUTES //
///////////////////////

Ctrl* Ctrl::instance = NULL;
MainFrame* Ctrl::gui = NULL;

/////////////
// METHODS //
/////////////

void Ctrl::registerView(MainFrame* inp_gui){
  gui = inp_gui;
}

Ctrl* Ctrl::getInstance(){
  if(instance == NULL){
    instance = new Ctrl();
  }
  return instance;
}

bool Ctrl::loadRadiusFile(){
  // create an instance of the model class
  // ensures, that there is only ever one instance of the model class
  if(current_calculation == NULL){
    current_calculation = new Model();
  }

  std::string radius_filepath = gui->getRadiusFilepath();
  current_calculation->readRadiiAndAtomNumFromFile(radius_filepath);
  // Refresh atom list with new radius map
  gui->displayAtomList(current_calculation->generateAtomList());
  return true;
}

bool Ctrl::loadAtomFile(){

  // create an instance of the model class
  // ensures, that there is only ever one instance of the model class
  if(current_calculation == NULL){
    current_calculation = new Model();
  }

  std::string atom_filepath = gui->getAtomFilepath();

  if (!current_calculation->readAtomsFromFile(atom_filepath, gui->getIncludeHetatm())){
    // refresh the atom list which is now empty due to invalid input file
    gui->displayAtomList(current_calculation->generateAtomList());
    return false;
  }
  else {
    // get atom list from model and pass onto view
    gui->displayAtomList(current_calculation->generateAtomList());
    return true;
  }
}

bool Ctrl::runCalculation(){
  // create an instance of the model class
  // ensures, that there is only ever one instance of the model class
  if(current_calculation == NULL){
    current_calculation = new Model();
  }

  // radius map is generated from grid in gui, then passed to model for calculation
  current_calculation->setRadiusMap(gui->generateRadiusMap());
  current_calculation->setAtomListForCalculation(gui->getIncludedElements());
  current_calculation->storeAtomsInTree();

  Ctrl::notifyUser("Result for " + gui->generateChemicalFormulaFromGrid());

  // get user inputs
  const double grid_step = gui->getGridsize();
  const int max_depth = gui->getDepth();
  const double r_probe = gui->getProbeRadius();
  // set space size (size of box containing all atoms)
  current_calculation->defineCell(grid_step, max_depth);
  current_calculation->findCloseAtoms(r_probe);

  // measure time and run calculation
  auto start = std::chrono::steady_clock::now();
  current_calculation->calcVolume();
  auto end = std::chrono::steady_clock::now();
  std::chrono::duration<double> elapsed_seconds = end-start;

  notifyUser("Elapsed time: " + std::to_string(elapsed_seconds.count()) + " s");

  return true;
}

void Ctrl::notifyUser(std::string str){
  str = "\n" + str;
  gui->appendOutput(str);
}

Model* Ctrl::getModel(){
	return this->current_calculation;
}
