#include "model.h"
#include "atom.h"
#include "controller.h"
#include "misc.h"
#include "exception.h"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream> // stringstream
#include <cassert>
#include <exception>
#include <iterator>
#include <algorithm>

///////////////////////////////
// AUX FUNCTION DECLARATIONS //
///////////////////////////////

bool isAtomLine(const std::vector<std::string>& substrings);
std::string strToValidSymbol(std::string str);
static inline std::vector<std::string> splitLine(const std::string& line);

struct ElementsFileBundle{ // data bundle for elements file import
  std::unordered_map<std::string, double> rad_map;
  std::unordered_map<std::string, double> weight_map;
  std::unordered_map<std::string, int> atomic_num_map;
};
//////////////////////////
// ELEMENTS FILE IMPORT //
//////////////////////////
ElementsFileBundle extractDataFromElemFile(const std::string& elem_path);

// generates three maps for assigning a radius, weight and atomic number respectively, to an element symbol
// sets the maps to members of the model class
bool Model::importElemFile(const std::string& elem_path){
  ElementsFileBundle data = extractDataFromElemFile(elem_path);
  if (data.rad_map.size() == 0) {return false;}

  setRadiusMap(data.rad_map);
  _elem_weight = data.weight_map;
  _elem_Z = data.atomic_num_map;
  return true;
}

// used for importing only the radius map from the radius file
// needed for running the app from the command line
std::unordered_map<std::string, double> Model::extractRadiusMap(const std::string& elem_path){
  return extractDataFromElemFile(elem_path).rad_map;
}

ElementsFileBundle extractDataFromElemFile(const std::string& elem_path){
  ElementsFileBundle data;

  auto hasCorrectFormat = [](std::vector<std::string> substrings){
    if (substrings.size() != 4){return false;}
    if (substrings[0].find_first_not_of("0123456789") != std::string::npos){return false;}
    for (char i : {2,3}){
      if (substrings[i].find_first_not_of("0123456789E.+e") != std::string::npos){return false;}
    }
    return true;
  };

  std::string line;
  std::ifstream inp_file(elem_path);
  bool invalid_symbol_detected = false;
  bool invalid_radius_value = false;
  bool invalid_weight_value = false;
  while(getline(inp_file,line)){
    std::vector<std::string> substrings = splitLine(line);
    // substrings[0]: Atomic Number
    // substrings[1]: Element Symbol
    // substrings[2]: Radius
    // substrings[3]: Weight
    if(hasCorrectFormat(substrings)){
      substrings[1] = strToValidSymbol(substrings[1]);
      // skip entry if element symbol invalid
      if (substrings[1].empty()){
        invalid_symbol_detected = true;
      }
      else {
        try{data.rad_map[substrings[1]] = std::stod(substrings[2]);}
        catch (const std::invalid_argument& e){
          data.rad_map[substrings[1]] = 0;
          invalid_radius_value = true;
        }
        try{data.weight_map[substrings[1]] = std::stod(substrings[3]);}
        catch (const std::invalid_argument& e){
          data.weight_map[substrings[1]] = 0;
          invalid_weight_value = true;
        }
        try{data.atomic_num_map[substrings[1]] = std::stoi(substrings[0]);}
        catch (const std::invalid_argument& e){
          data.atomic_num_map[substrings[1]] = 0;
        }
      }
    }
  }
  if (invalid_symbol_detected) {Ctrl::getInstance()->displayErrorMessage(106);}
  if (invalid_radius_value) {Ctrl::getInstance()->displayErrorMessage(107);}
  if (invalid_weight_value) {Ctrl::getInstance()->displayErrorMessage(108);}
  return data;
}

//////////////////////
// ATOM FILE IMPORT //
//////////////////////

bool Model::readAtomsFromFile(const std::string& filepath, bool include_hetatm){
  // it is very hard to tell what this function does, because of the heavy use of global variables.
  // this function should ideally end in a set function, and the other functions should have return
  // values -JM
  clearAtomData();

  try{
    readAtomFile(filepath, include_hetatm);
  }
  catch(const ExceptIllegalFileExtension& e) {throw;}
  catch(const ExceptInvalidCellParams& e) {throw;}

  if (_raw_atom_coordinates.size() == 0){ // If no atom is detected in the input file, the file is deemed invalid
    throw ExceptInvalidInputFile();
  }
  return true;
}

void Model::clearAtomData(){
  _atom_amounts.clear();
  _raw_atom_coordinates.clear();
  _space_group = "";
  _sym_matrix_XYZ.clear();
  _sym_matrix_fraction.clear();
  for(int i = 0; i < 6; i++){
    _cell_param[i] = 0;
  }
}

bool Model::readAtomFile(const std::string& filepath, bool include_hetatm){

  if (fileExtension(filepath) == "xyz"){
    readFileXYZ(filepath);
  }
  else if (fileExtension(filepath) == "pdb"){
    readFilePDB(filepath, include_hetatm);
  }
  else if (fileExtension(filepath) == "cif"){
    try{readFileCIF(filepath);}
    catch (const ExceptInvalidCellParams& e){throw;}
  }
  else {throw ExceptIllegalFileExtension();}
  return true;
}

void Model::readFileXYZ(const std::string& filepath){
  std::string line;
  std::ifstream inp_file(filepath);

  bool invalid_entry_encountered = false;
  bool first_atom_line_encountered = false;
  while(getline(inp_file,line)){
    std::vector<std::string> substrings = splitLine(line);
    // substrings[0]: Element Symbol
    // substrings[1,2,3]: Coordinates
    // create new atom and add to storage vector if line format corresponds to Element_Symbol x y z
    if (substrings.empty()){} // skip blank lines
    else if (isAtomLine(substrings)) {
      first_atom_line_encountered = true;

      const std::string valid_symbol = strToValidSymbol(substrings[0]);
      _atom_amounts[valid_symbol]++; // adds one to counter for this symbol

      // Stores the full list of atom coordinates from the input file
      _raw_atom_coordinates.push_back(std::make_tuple(valid_symbol, std::stod(substrings[1]), std::stod(substrings[2]), std::stod(substrings[3])));
    }
    else {
      // due to the .xyz format the first few lines may not be atom entries. an error should only be detected,
      // after the first valid atom entry has been encountered
      if (first_atom_line_encountered){invalid_entry_encountered = true;}
    }
  }
  inp_file.close();
  if (invalid_entry_encountered){Ctrl::getInstance()->displayErrorMessage(105);}
}

void Model::readFilePDB(const std::string& filepath, bool include_hetatm){
  // struct for extracting data from lines. defined here, because it is only needed here
  struct AtomLinePDB {
    AtomLinePDB() = default;
    AtomLinePDB(const std::string& line){
      record_name = line.substr(0,6);
      assert (record_name == "ATOM  " || record_name == "HETATM");
      try{serial_no = std::stoi(line.substr(6,5));}
      catch (const std::invalid_argument& e){serial_no=0;}
      name = line.substr(12,4);
      alt_loc = line[16];
      res_name = line.substr(17,3);
      chain_id = line[21];
      try{res_seq = std::stoi(line.substr(22,4));}
      catch (const std::invalid_argument& e){res_seq=0;}
      insert_code = line[26];
      for (int i = 0; i < 3; ++i){
        ortho_coord[i] = std::stod(line.substr(30+i*8,8));
      }
      try{occupancy = std::stod(line.substr(54,6));}
      catch (const std::invalid_argument& e){occupancy=0;}
      try{temp_factor = std::stod(line.substr(60,6));}
      catch (const std::invalid_argument& e){temp_factor=0;}
      element_symbol = line.substr(76,2);
      // some software generate pdb files with symbol left-justified instead of right-justified
      // therefore, it is better to check both characters and erase any white space
      removeWhiteSpaces(element_symbol);
      charge = line.substr(78,2);
      removeEOL(charge);
    }
    std::string record_name;
    int serial_no;
    std::string name;
    char alt_loc;
    std::string res_name;
    char chain_id;
    int res_seq;
    char insert_code;
    std::array<double,3> ortho_coord;
    double occupancy;
    double temp_factor;
    std::string element_symbol;
    std::string charge;
  };

  std::string line;
  std::ifstream inp_file(filepath);
  bool invalid_symbol_detected = false;
  bool invalid_cell_params = false;
  bool invalid_atom_line = false;
  // iterate through lines
  while(getline(inp_file,line)){
    const std::string record_name = line.substr(0,6);
    if (record_name == "ATOM  " || (include_hetatm && record_name == "HETATM")){
      AtomLinePDB atom_line;
      try {atom_line = AtomLinePDB(line);} // extract all information from line
      catch (const std::invalid_argument& e){ // detect invalid line
        invalid_atom_line = true;
        continue;
      }
      std::string symbol = strToValidSymbol(atom_line.element_symbol);
      if (symbol.empty()) {
        invalid_symbol_detected = true;
        continue;
      }
      _atom_amounts[symbol]++; // adds one to counter for this symbol

      // stores the full list of atom coordinates from the input file
      _raw_atom_coordinates.emplace_back(symbol, atom_line.ortho_coord[0], atom_line.ortho_coord[1], atom_line.ortho_coord[2]);
    }
    else if (record_name == "CRYST1"){
      // for the last substring (space group) mercury recognizes only 10 chars but official PDB format is 11 chars
      std::vector<std::string> substrings = {line.substr(6,9), line.substr(15,9), line.substr(24,9), line.substr(33,7), line.substr(40,7), line.substr(47,7), line.substr(55,11)};
      removeEOL(substrings[6]);
      for (size_t i = 0; i < substrings.size()-1; ++i){
        try{_cell_param[i] = std::stod(substrings[i]);}
        catch (const std::invalid_argument& e){invalid_cell_params = true;}
      }
      _space_group = substrings[6];
      removeWhiteSpaces(_space_group);
    }
  }
  // file has been read
  inp_file.close();
  if (invalid_symbol_detected){Ctrl::getInstance()->displayErrorMessage(105);}
  if (invalid_cell_params){Ctrl::getInstance()->displayErrorMessage(112);}
  if (invalid_atom_line){Ctrl::getInstance()->displayErrorMessage(114);}
}

// this function will successfully extract data from standard cif files such as those generated by Mercury program
// however, due to the permissive format of cif files, this function might fail in some occasions
void Model::readFileCIF(const std::string& filepath){
  std::string line; // line in cif file
  std::string no_ws_line; // line in cif file with whitespaces removed
  std::ifstream inp_file(filepath);

  bool space_group_P1 = false;
  bool symmetry_data_acquired = false;
  bool atom_data_acquired = false;
  char cell_data_acquired = 0;
  char loop = 0; // 0: out of list, 1: in list, 2: symop headers list, 3: symop value list, 4: atom headers list, 5: atom data list
  std::vector<std::string> symop_list;
  std::vector<std::string> atom_headers;
  std::vector<std::string> atom_list;

  auto containsKeyword = [](const std::string& line, const std::string& keyword=""){
    // default: contains any keyword
    if (keyword.empty()){return (line[0] == '_');}
    // contains specific keyword
    else {return (line.find(keyword) != std::string::npos);}
  };

  while(getline(inp_file,line)){ // read file line by line
    // the information we need is case insensitive
    // therefore, it is safer to directly convert the lines to lower case
    std::for_each(line.begin(), line.end(), [](char& c){c = ::tolower(c);});

    // in cif files white_space are sometimes meaningful, sometimes not
    // thus, it is convenient to have a copy of a cif line with no white_space
    no_ws_line = line;
    removeWhiteSpaces(no_ws_line);
    removeEOL(no_ws_line);

    if(no_ws_line[0] == '#' || no_ws_line.empty()){continue;} // skip comment and empty lines

    // KEYWORD SEARCH
    // cif files usually contain explicit symmetry operations
    // however, space group P1 does not have any symmetry operation except the identity
    // therefore, the symmetry operation list may not be present for this group
    // and it is useful to keep track of the space group if it is P1
    if(containsKeyword(line,"_space_group_name_") && containsKeyword(no_ws_line,"'p1'")){
        space_group_P1 = true;
    }
    // list of symmetry operations or atom coordinates will start with "loop_"
    else if(!loop && containsKeyword(line,"loop_")){
      loop = 1; // we have entered a loop
      continue;
    }
    else {
      static const std::array<std::string,6> s_cell_data_keywords = {
        "_cell_length_a",
        "_cell_length_b",
        "_cell_length_c",
        "_cell_angle_alpha",
        "_cell_angle_beta",
        "_cell_angle_gamma"
      };
      std::vector<std::string> substrings = splitLine(line);
      for (size_t i = 0; i < s_cell_data_keywords.size(); ++i){
        if (containsKeyword(line,s_cell_data_keywords[i])){
          try{_cell_param[i] = std::stod(substrings[1]);}
          catch (const std::invalid_argument& e){throw ExceptInvalidCellParams();}
          cell_data_acquired++;
          break;
        }
      }
    }

    // LOOP
    if (loop){
      // identify whether this loop contains interesting data
      if(loop == 1){
        if(!symmetry_data_acquired
            && (containsKeyword(line,"_space_group_symop_") || containsKeyword(line,"_symmetry_equiv_pos_"))){
          // symmetry data loop
          loop = 2;
        }
        else if(!atom_data_acquired && containsKeyword(line,"_atom_site_")){
          // atom data loop
          loop = 4;
        }
        else if(!containsKeyword(no_ws_line)){ // we have left the loop without finding an interesting keyword
          loop = 0;
        }
      }

      if(loop == 2){
        // in list of headers (data name) for symmetry operations
        // there are either 1 or 2 headers in these symop loops
        // it is unnecessary to store these headers as they are irrelevant to the symop data analysis algorithm

        // if no data name is found, the list of symmetry operations started
        if(!containsKeyword(no_ws_line)){
          loop = 3;
        }
      }
      if(loop == 3){
        // in list of symmetry operations
        // if a data name is found, the list of symmetry operation value ended
        if(containsKeyword(no_ws_line)){
          loop = 0;
          symmetry_data_acquired = true;
        }
        else if(containsKeyword(line,"loop_")){
          loop = 1;
          symmetry_data_acquired = true;
        }
        else{
          // store the line containing symmetry operation data for later processing
          symop_list.emplace_back(line);
          continue;
        }
      }
      if(loop == 4){
        // in list of headers (data name) for atoms
        // if no data name is found, the list of atom parameters started
        if(!containsKeyword(no_ws_line)){
          loop = 5;
        }
        else{
          // store atom headers in order of appearance because only certain types of atom data is useful
          // and it is necessary to know in what order they appear
          atom_headers.emplace_back(line);
          continue;
        }
      }
      if(loop == 5){
        // in list of atom parameters
        // if a data name is found, the list of atom parameters ended
        if(containsKeyword(no_ws_line)){
          loop = 0;
          atom_data_acquired = true;
        }
        else if(containsKeyword(line,"loop_")){
          loop = 1;
          atom_data_acquired = true;
        }
        else{
          // store the line containing atom parameters for later processing
          atom_list.emplace_back(line);
          continue;
        }
      }
    }

    // when all useful data is extracted, stop reading the cif file
    if(atom_data_acquired && symmetry_data_acquired && cell_data_acquired > 5){
      break;
    }
  }
  inp_file.close();

  // the atom coordinates are needed in Cartesian coordinates in angstrom
  // thus, the a, b, c fractions need to be converted to Cartesian coordinates
  // find the Cartesian matrix defining the unit cell axes A, B, C and store in _cart_matrix
  // Ax [0][0], Ay [0][1], Az [0][2]
  // Bx [1][0], By [1][1], Bz [1][2]
  // Cx [2][0], Cy [2][1], Cz [2][2]
  orthogonalizeUnitCell();

  if(space_group_P1){ // set identity symmetry element
    for(int i = 0; i < 3; i++){
      for(int j = 0; j < 3; j++){
        if(i == j){
          _sym_matrix_XYZ.emplace_back(1);
        }
        else{
          _sym_matrix_XYZ.emplace_back(0);
        }
      }
      _sym_matrix_fraction.emplace_back(0);
    }
  }
  else{
    convertCifSymmetryElements(symop_list);
  }
  if (!convertCifAtomsList(atom_headers, atom_list)){
    Ctrl::getInstance()->displayErrorMessage(105); // at least one atom line could not be read
  }
}

// used in unittest
std::vector<std::string> Model::listElementsInStructure(){
  std::vector<std::string> list;
  for (auto elem : _atom_amounts){
    list.push_back(elem.first);
  }
  return list;
}

// symmetry elements in cif files are stored as strings that are not convenient for calculations
// this function converts the list of strings in convenient matrix
bool Model::convertCifSymmetryElements(const std::vector<std::string> &symop_list){
  auto evalToken = [](const std::string& token, const char coord){
    if (token.find({'-',coord}) != std::string::npos){
      return -1;
    }
    return (token.find(coord) != std::string::npos)? 1 : 0;
  };
  auto extractFrac = [](const std::string& line){
    size_t divide_pos = line.find('/');
    // prevent accessing positions in string that are out of range
    if(divide_pos != std::string::npos && divide_pos > 0 && divide_pos < line.size()-1){
      char char_num = line[divide_pos-1];
      char char_denom = line[divide_pos+1];
      if (isdigit(char_num) && isdigit(char_denom)){
        double num = char_num - '0';
        double denom = char_denom - '0';
        int sign = (divide_pos > 1 && line[divide_pos-2] == '-')? 1 : -1;
        return sign * num/denom;
      }
    }
    return double(0);
  };

  for(size_t i = 0; i < symop_list.size(); i++){
    // stringstream class check1
    std::stringstream check1(symop_list[i]);
    // Tokenizing separated by comma
    std::string token;
    for (int j = 0; getline(check1, token, ','); j++){
      for (char coord : {'x','y','z'}){
        _sym_matrix_XYZ.push_back(evalToken(token, coord));
      }

      // if there is a fraction in the substring, convert to double
      _sym_matrix_fraction.push_back(extractFrac(token));
    }
  }
  return true;
}

bool Model::convertCifAtomsList(const std::vector<std::string> &atom_headers, const std::vector<std::string> &atom_list){
  bool all_lines_valid = true;
  // in cif files the order of data values is not fixed
  // atom_headers contains the info on the order of data values stored in atom_list
  // fields that we're interested in:
  static const std::vector<std::string> s_keywords = {
    "_atom_site_type_symbol",
    "_atom_site_fract_x",
    "_atom_site_fract_y",
    "_atom_site_fract_z"
  };
  std::vector<size_t> param_pos;

  // save what position the data values can be found
  for (const std::string& keyword : s_keywords){
    for(size_t i = 0; i < atom_headers.size(); ++i){
      if (atom_headers[i].find(keyword) != std::string::npos){
        param_pos.push_back(i);
        break;
      }
    }
  }

  // if not all keywords have been found
  if (s_keywords.size() != param_pos.size()){return false;}

  for(const std::string& line : atom_list){
    std::vector<std::string> substrings = splitLine(line);

    // check if substring has enough elements
    if (substrings.size() <= *std::max_element(param_pos.begin(), param_pos.end())){
      all_lines_valid = false;
      continue;
    }

    // save the fractional positional data from atom_list
    std::array<double,3> abc_frac;
    for(int i = 0; i < 3; i++){
      // check whether the values can be converted to doubles
      try{
        abc_frac[i] = std::stod(substrings[param_pos[i+1]]);
      }
      catch (std::invalid_argument& e){
        all_lines_valid = false;
        continue;
      }
    }

    // apply the matrix to the fractional coordinates to get cartesian coordinates
    std::array<double,3> xyz = {0,0,0};
    for(int i = 0; i < 3; ++i){ // loop for x, y ,z
      for(int j = 0; j < 3; ++j){ // loop for a, b ,c
        xyz[i] += abc_frac[j] * _cart_matrix[j][i];
      }
    }

    // get the element symbol and keep track of their number
    const std::string symbol = strToValidSymbol(substrings[param_pos[0]]);
    _atom_amounts[symbol]++;

    // store the full list of atom coordinates from the input file
    _raw_atom_coordinates.push_back(std::make_tuple(symbol, xyz[0], xyz[1], xyz[2]));
  }
  return all_lines_valid;
}

bool Model::getSymmetryElements(std::string group, std::vector<int> &sym_matrix_XYZ, std::vector<double> &sym_matrix_fraction){
  for (size_t i = 0; i<group.size(); i++) { // convert space group to upper case chars to compare with the list
    group[i] = toupper(group[i]);
  }
  group = "'" + group + "'";
  std::ifstream sym_file(getResourcesDir() + "/space_groups.txt");
  std::string sym_line;
  bool group_found = 0;
  bool sym_matrix = 0;
  while (getline (sym_file, sym_line)){
    if(sym_matrix){
      if(sym_line.find("Space group end") != std::string::npos){
        return true; // end function when all symmetry elements of the matching space group are stored in vectors
      }
      else{
        std::vector<std::string> sym_matrix_line = splitLine(sym_line); // store 12 parameters of the symmetry matrix
        std::stringstream ss;
        if(sym_matrix_line.size() == 12){
          for (int i = 0; i < 9; i++){
            std::stringstream ss(sym_matrix_line[i]);
            int matrix_elem = 0;
            ss >> matrix_elem;
            sym_matrix_XYZ.emplace_back(matrix_elem); // stores the Aa, Ab, Ac, Ba, Bb, Bc, Ca, Cb, Cc matrix elements as +1, 0 or -1
          }
          for (int i = 9; i < 12; i++){
            sym_matrix_fraction.emplace_back(std::stod(sym_matrix_line[i])); // stores the AA, BB, CC matrix elements as fractions 0, 1/6, 1/4, 1/3, 1/2, 2/3, 3/4, 5/6
          }
        }
        else {
          return false; // return false if a matrix line was found with less than 12 elements which should not happen with the space group list file provided
        }
      }
    }
    else if(group_found && sym_line.find("_symmetry_equiv_pos_as_matrix") != std::string::npos){
      sym_matrix = 1; // activate second switch when we reach the symmetry matrix part of the matching space group
    }
    else if(sym_line.find("_symmetry_space_group_name_H-M") != std::string::npos && sym_line.find(group) != std::string::npos){
      group_found = 1; // activate switch when matching space group is found
    }
  }
  return false; // return false when no matching space group was found in the list file or no file found
}

////////////////////////
// METHOD DEFINITIONS //
////////////////////////

// returns the radius of an atom with a given symbol
double Model::findRadiusOfAtom(const std::string& symbol){
  return _radius_map[symbol];
}

double Model::findRadiusOfAtom(const Atom& at){
  return findRadiusOfAtom(at.symbol);
}

double Model::findWeightOfAtom(const std::string& symbol){
  return _elem_weight[symbol];
}

///////////////////
// AUX FUNCTIONS //
///////////////////

// split line into substrings when separated by whitespaces
static inline std::vector<std::string> splitLine(const std::string& line){
  std::istringstream iss(line);
  std::vector<std::string> substrings((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());
  return substrings;
}

// check if a vector of substrings has the format of an atom line
bool isAtomLine(const std::vector<std::string>& substrings) {
  // atom lines must have four items
  if (substrings.size() != 4) {return false;}
  // if first item cannot be converted to a valid element symbol return false
  if (strToValidSymbol(substrings[0]).empty()){return false;}
  for (char i=1; i<4; i++){
    // check whether subsequent items can be converted to a number
    try {
      size_t str_pos = 0; // will contain the last position in the string that was successfully evaluated by std::stod()
      std::stod(substrings[i], &str_pos);
      if (substrings[i].size() != str_pos) {return false;}
    }
    catch (const std::invalid_argument& e) {return false;}
  }
  return true;
}

// reads a string and converts it to valid atom symbol: first character uppercase followed by lowercase characters
std::string strToValidSymbol(std::string str){
  // return empty if str is empty
  if (str.size() == 0){return "";}
  else{
    // return empty if str begins with non-alphabetic character
    if (!isalpha(str[0])){return "";}
    // only for first character in sequence, convert to uppercase
    else {str[0] = toupper(str[0]);}
  }
  // iterate over all characters in string
  for (size_t i = 1; i < str.size(); i++) {
    // convert to lowercase
    if (isalpha(str[i])) {
      str[i] = tolower(str[i]);
    }
    // allow underscores inside element symbols for custom symbols
    //else if (str[i] == '_') {}
    else { // Remove number or charges from atoms so that "Pd2+" becomes "Pd"
      str.erase(i, str.size());
    }
  }
  return str;
}

