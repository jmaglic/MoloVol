
#include "model.h"
#include "controller.h"
#include "atom.h"
#include "special_chars.h"
#include <array>
#include <string>
#include <vector>
#include <algorithm>

///////////////////////////////
// AUX FUNCTION DECLARATIONS //
///////////////////////////////

inline bool isIncluded(const std::string&, const std::vector<std::string>&);

void Model::defineCell(const double& grid_step, const int& max_depth){
  cell = Space(atoms, grid_step, max_depth);
  return;
}

///////////////////////////
// CALCULATION FUNCTIONS //
///////////////////////////
// Not sure about the name of the section

void Model::setAtomListForCalculation(const std::vector<std::string>& included_elements){
  atoms.clear();
  for(int i = 0; i < raw_atom_coordinates.size(); i++){
    if(isIncluded(std::get<0>(raw_atom_coordinates[i]), included_elements)){
      Atom at = Atom(std::get<1>(raw_atom_coordinates[i]),
                     std::get<2>(raw_atom_coordinates[i]),
                     std::get<3>(raw_atom_coordinates[i]),
                     std::get<0>(raw_atom_coordinates[i]),
                     radius_map[std::get<0>(raw_atom_coordinates[i])],
                     elem_Z[std::get<0>(raw_atom_coordinates[i])]);
      atoms.push_back(at);
    }
  }
}

void Model::storeAtomsInTree(){
  atomtree = AtomTree(atoms);
}

void Model::findCloseAtoms(const double& r_probe){
  //TODO
  return;
}

void Model::calcVolume(){
  cell.placeAtomsInGrid(atomtree);
  double volume = cell.getVolume();

  std::string message_to_user
    = "Van der Waals Volume: " + std::to_string(volume) + " " + Symbol::angstrom() + Symbol::cubed();
  Ctrl::getInstance()->notifyUser(message_to_user);
}

std::vector<std::tuple<std::string, int, double>> Model::generateAtomList(){
  std::vector<std::tuple<std::string, int, double>> atoms_for_list;
  for(auto elem : atom_amounts){
    atoms_for_list.emplace_back(elem.first, elem.second, raw_radius_map[elem.first]);
  }
  return atoms_for_list;
}

void Model::setRadiusMap(std::unordered_map<std::string, double> map){
  radius_map = map;
}

void Model::debug(){
  std::array<double,3> cell_min = cell.getMin();
  std::array<double,3> cell_max = cell.getMax();

  for(int dim = 0; dim < 3; dim++){
    std::cout << "Cell Limit in Dim " << dim << ":" << cell_min[dim] << " and " << cell_max[dim] << std::endl;
  }
}

std::vector<uint8_t> Model:: getMatrix(){
	return this->cell.getMatrix();
}

std::array<unsigned int,3> Model::getResolution(){
	return this->cell.getResolution();
}

///////////////////
// AUX FUNCTIONS //
///////////////////

inline bool isIncluded(const std::string& element_symbol, const std::vector<std::string>& included_elements) {
  return (std::find(included_elements.begin(), included_elements.end(), element_symbol) != included_elements.end());
}
