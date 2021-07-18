#include "model.h"
#include "atom.h"
#include "controller.h"
#include "misc.h"
#include "container3d.h"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <filesystem>

///////////////////
// RESULT REPORT //
///////////////////

std::string makeExportFileName(const std::string, const CalcReportBundle&, const char, const unsigned char=0);
void Model::createReport(){
  createReport(makeExportFileName(_output_folder, _data, 'r'));
}

void Model::createReport(std::string path){
  const double AVOGADRO = 6.02214076e23;
  std::string small_p = (_data.probe_mode)? "Small probe" : "Probe";
  std::string small_p_tab = (_data.probe_mode)? "" : "\t";
  std::ofstream output_report(path);
  output_report << "\n";
  output_report << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n";
  output_report << "   MM           MM            LL           VV           VV           LL   \n";
  output_report << "   MMM         MMM            LL            VV         VV            LL   \n";
  output_report << "   MMMM       MMMM            LL             VV       VV             LL   \n";
  output_report << "   MM MM     MM MM            LL              VV     VV              LL   \n";
  output_report << "   MM  MM   MM  MM    OOOO    LL    OOOO       VV   VV       OOOO    LL   \n";
  output_report << "   MM   MM MM   MM   OO  OO   LL   OO  OO       VV VV       OO  OO   LL   \n";
  output_report << "   MM    MMM    MM   OO  OO   LL   OO  OO        VVV        OO  OO   LL   \n";
  output_report << "   MM     M     MM    OOOO    LL    OOOO          V          OOOO    LL   \n\n";
  output_report << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n";
  output_report << "Source code available at https://github.com/jmaglic/MoloVol under the MIT licence\n";
  output_report << "Copyright © 2020-2021 Jasmin B. Maglic, Roy Lavendomme\n\n";
  output_report << "MoloVol program: calculation results report\n";
  output_report << "version: " + Ctrl::getVersion() + "\n\n";
  output_report << "Time of the calculation: " << _time_stamp << "\n";
  output_report << "Duration of the calculation: " << _data.getTime() << " s\n\n";
  output_report << "Structure file analyzed: " << _data.atom_file_path << "\n";
  output_report << "Chemical formula: " + _data.chemical_formula << "\n";
  output_report << "Molar mass: " << _data.molar_mass << " g/mol\n";

  output_report << "\n\n\t////////////////////////////\n";
  output_report << "\t// Calculation parameters //\n";
  output_report << "\t////////////////////////////\n\n";
  if(fileExtension(_data.atom_file_path) == "pdb"){
    output_report << std::string((_data.inc_hetatm)? "Include" : "Exclude") + " HETATM from pdb file\n";
  }
  output_report << "Crystal unit cell analysis: " + std::string((_data.analyze_unit_cell)? "Yes" : "No") + "\n";
  if(_data.probe_mode){
    output_report << "Probe mode: two probes\n";
    output_report << "Small probe radius: " << getProbeRad1() << " A\n";
    output_report << "Large probe radius: " << getProbeRad2() << " A\n";
  }
  else{
    output_report << "Probe mode: one probe\n";
    output_report << "Probe radius: " << getProbeRad1() << " A\n";
  }
  output_report << "Grid resolution: " << _data.grid_step << " A\n";
  output_report << "Optimization depth: " << _data.max_depth << "\n";
  output_report << "Elements radii:\n";
  for(std::unordered_map<std::string, double>::iterator it = _radius_map.begin(); it != _radius_map.end(); it++){
    if(isIncluded(it->first, _data.included_elements)){
      output_report << it->first << " : " << it->second << " A\n";
    }
  }

  output_report << "\n\n\t//////////////////////////////\n";
  output_report << "\t// Total Volumes calculated //\n";
  output_report << "\t//////////////////////////////\n\n";
  // factor to convert A^3 to cm^3/g
  double volume_macro_factor = AVOGADRO * 1e-24 / _data.molar_mass;
  double unit_cell_vol = 0;

  if(_data.analyze_unit_cell){
    output_report << "Orthogonal(ized) unit cell axes: " << _cart_matrix[0][0] << " A, " << _cart_matrix[1][1] << " A, " << _cart_matrix[2][2] << " A\n";
    unit_cell_vol = _cart_matrix[0][0]*_cart_matrix[1][1]*_cart_matrix[2][2];
    output_report << "Total unit cell volume: " << unit_cell_vol << " A^3\n";
    output_report << "Density: " << _data.molar_mass * 1e24 / (unit_cell_vol * AVOGADRO) << " g/cm^3\n\n";
  }

  // layout function for individual rows
  const int col_width[] = {33,14,6,21,8}; // last column gets as much as needed
  auto row = [col_width,this](std::string cell0, double cell1, std::string cell2, std::string cell3="", double cell4=0, std::string cell5=""){
    std::string str = "";
    str += field(col_width[0],cell0) + " ";
    str += field(col_width[1],std::to_string(cell1),'r') + " ";
    if (_data.analyze_unit_cell && cell4!=0) {
      str += field(col_width[2],cell2) + " ";
      str += field(col_width[3],cell3) + " ";
      str += field(col_width[4],std::to_string(cell4),'r') + " ";
      str += cell5;
    }
    else{
    str += cell2;
    }
    return str + "\n";
  };

  // layout function for volume data display
  auto vol_block = [row, unit_cell_vol, volume_macro_factor](std::string text, double vol, std::string subtext=""){
    std::string str = "";
    str += row(text + ":", vol,"A^3","(unit cell fraction:", vol/unit_cell_vol, ")");
    str += row(subtext,vol * volume_macro_factor, "cm^3/g");
    return str + "\n";
  };

  output_report << vol_block("Van der Waals volume", _data.volumes[0b00000011]);
  output_report << vol_block("Excluded void volume", _data.volumes[0b00000101]);
  output_report << vol_block("Molecular volume", _data.volumes[0b00000011] + _data.volumes[0b00000101], "(vdw + probe inaccessible)");
  if(!_data.probe_mode && !_data.analyze_unit_cell){
    output_report << small_p << " core volume: No physical meaning, contains all volume outside the structure.\n\n";
  }
  else{
    output_report << vol_block(small_p + " core volume", _data.volumes[0b00001001]);
  }
  output_report << vol_block(small_p + " shell volume", _data.volumes[0b00010001]);
  if(_data.probe_mode || _data.analyze_unit_cell){
    output_report << vol_block(small_p + " occupied volume", _data.volumes[0b00001001] + _data.volumes[0b00010001], "(core + shell)");
  }

  if(_data.probe_mode){
    if(!_data.analyze_unit_cell){
      output_report << "Large probe core volume: No physical meaning, contains all volume outside the structure.\n\n";
    }
    else{
      output_report << vol_block("Large probe core volume", _data.volumes[0b00100001]);
    }
    output_report << vol_block("Large probe shell volume", _data.volumes[0b01000001]);
    if(_data.analyze_unit_cell){
      output_report << vol_block("Large probe occupied volume", _data.volumes[0b00100001] + _data.volumes[0b01000001], "(core + shell)");
    }
  }

  if(_data.calc_surface_areas){
    output_report << "\n\t////////////////////////////////////\n";
    output_report << "\t// Total Surface Areas calculated //\n";
    output_report << "\t////////////////////////////////////\n\n";
    // factor to convert A^2 to m^2/g
    double area_macro_factor = AVOGADRO * 1e-20 / _data.molar_mass;

    // layout function for surface data display
    auto surf_block = [row,area_macro_factor](std::string text, double surf, std::string subtext=""){
      std::string str = "";
      str += row(text + ":", surf, "A^2");
      str += row(subtext, surf * area_macro_factor, "m^2/g");
      return str + "\n";
    };

    output_report << surf_block("Van der Waals surface", _data.surf_vdw);
    output_report << surf_block(small_p + " excluded surface", _data.surf_probe_excluded, "(similar to Connolly surface)");
    output_report << surf_block(small_p + " accessible surface", _data.surf_probe_accessible, "(similar to Lee-Richards surface)");
    if(_data.probe_mode){
      output_report << surf_block("Molecular outer surface", _data.surf_molecular, "(both probes excluded surface)");
    }
  }

  if(!_data.cavities.empty()){
    output_report << "\n\n\t///////////////////////////////\n";
    output_report << "\t// Cavities and pockets data //\n";
    output_report << "\t///////////////////////////////\n\n";

    if(_data.cavities.size() >= 255){
      output_report << "!!! WARNING !!!\n";
      output_report << "Maximum number of cavities reached. Some cavities might be missing.\n";
      output_report << "To solve this issue, change probe radii (e.g. smaller large probe and/or larger small probe).\n\n";
    }
    output_report << "Note 1:\tPockets and isolated cavities are not differentiated yet.\n";
    output_report << "\tThis feature might be added in future versions.\n";
    output_report << "Note 2:\tSeparate cavities are defined by space accessible to the core of the small probe.\n";
    output_report << "\tTwo cavities can be in contact but if a probe cannot pass from one to the other, they are considered separated.\n";
    output_report << "Note 3:\tIn single probe mode, pockets are counted in the 'outside space'.\n";
    output_report << "Note 4:\tSome very small isolated chunks of small probe cores can be detected and lead to small cavities.\n";
    output_report << "Note 5:\tProbe occupied volume correspond to empty space as defined by the molecular surface (similar to the Connolly surface).\n";
    output_report << "Note 6:\tProbe accessible volume correspond to empty space as defined\n";
    output_report << "\tby the surface accessible to its core (similar to the Lee-Richards surface).\n";
    output_report << "Note 7:\tFor a detailed shape of each cavity, check the surface maps.\n\n";

    output_report << "Cavity\tOccupied\tAccessible\t" << (_data.calc_surface_areas ? "Excluded\tAccessible\t" : "") << "Cavity center coordinates (A)\n";
    output_report << "ID\tVolume (A^3)\tVolume (A^3)\t" << (_data.calc_surface_areas ? "Surface (A^2)\tSurface (A^2)\t" : "") << "x\ty\tz\n";
    for(unsigned int i = 0; i < _data.cavities.size(); i++){
      std::array<double,3> cav_center = _data.getCavCenter(i);
      // default precision is 6, which means that double values will take less than a tab space
      output_report << i+1 << "\t";
      // in single probe mode, the first cavity with id 1 comprises outside empty space and is meaningless
      if(!_data.probe_mode && !_data.analyze_unit_cell && _data.cavities[i].id == 1){
        output_report << "outside\t\t"
                      << "outside\t\t";
      }
      else{
        output_report << _data.cavities[i].getVolume() << "\t\t"
                      << _data.cavities[i].core_vol << "\t\t";
      }
      if(_data.calc_surface_areas){
        output_report << _data.cavities[i].surf_shell << "\t\t"
                      << _data.cavities[i].surf_core << "\t\t";
      }
      output_report << cav_center[0] << "\t" << cav_center[1] << "\t" << cav_center[2] << "\n";
    }
  }
  output_report << "\n\n\t/////////////////////////////\n";
  output_report << "\t// Surface map information //\n";
  output_report << "\t/////////////////////////////\n\n";
  output_report << "The surface map files generated (.dx, OpenDX format) can be opened with\n";
  output_report << "PyMOL, USCF Chimera or USCF ChimeraX.\n\n";
  output_report << "Use the following isosurface levels to visualize the desired surface:\n";
  output_report << "Level 0.5 : Van der Waals surface\n";
  if(_data.probe_mode){
    output_report << "Level 1.5 : Molecular surface (both probes excluded, similar to the Connolly surface)\n";
    output_report << "Level 3.0 : Internal cavities and pockets (small probe excluded, similar to the Connolly surface but only 'inside')\n";
    output_report << "Level 5.0 : Small probe accessible surface (similar to Lee-Richards molecular surface but only 'inside')\n";
  }
  else{
    output_report << "Level 2.0 : Molecular and cavity surface (probe excluded, similar to the Connolly surface)\n";
    output_report << "Level 5.0 : Probe accessible surface (similar to the Lee-Richards molecular surface)\n";
  }
  output_report << "\nFor help on how to vizualize maps check the user manual or:\n";
  output_report << " - in PyMOL, simply open the map file then click 'A' in the right panel, choose mesh or surface and select the level.\n";
  output_report << "   For more information, check https://pymolwiki.org/index.php/Isomesh and https://pymolwiki.org/index.php/Isosurface \n";
  output_report << " - in USCF Chimera, check https://www.cgl.ucsf.edu/chimera/docs/ContributedSoftware/volumeviewer/volumeviewer.html \n";
  output_report << " - in USCF ChimeraX, check https://www.cgl.ucsf.edu/chimerax/docs/user/tools/volumeviewer.html \n";

  output_report << "\nIf you wish to visualize the structure file in PyMOL with the same element radii as in the MoloVol calculation,\n";
  output_report << "paste the following command lines (all at once) in the command prompt of PyMOL after opening the structure file.\n\n";

  for(std::unordered_map<std::string, double>::iterator it = _radius_map.begin(); it != _radius_map.end(); it++){
    if(isIncluded(it->first, _data.included_elements)){
      output_report << "alter (elem " << it->first << "),vdw=" << it->second << "\n";
    }
  }
  output_report << "rebuild\n\n";

  output_report.close();
}

//////////////////////
// STRUCTURE OUTPUT //
//////////////////////


void Model::writeCrystStruct(std::string path){
  path.erase(path.end()-4, path.end());
  writeXYZfile(_data.orth_cell, path+"_struct_orthogonal_cell.xyz", "Orthogonal cell");
  writeXYZfile(_data.supercell, path+"_struct_partial_supercell.xyz", "Partial supercell");
}

void Model::writeCrystStruct(){
  std::string path = makeExportFileName(_output_folder, _data, 'c');
  writeXYZfile(_data.orth_cell, path, "Orthogonal cell");
  path = makeExportFileName(_output_folder, _data, 'p');
  writeXYZfile(_data.supercell, path, "Partial supercell");
}

void Model::writeXYZfile(const std::vector<std::tuple<std::string, double, double, double>> &atom_coordinates, const std::string path, const std::string output_type){
  std::ofstream output_structure(path);
  output_structure << output_type << "\nStructure generated with MoloVol\n\n";
  for(size_t i = 0; i < atom_coordinates.size(); i++){
    output_structure << std::get<0>(atom_coordinates[i]) << " ";
    output_structure << std::to_string(std::get<1>(atom_coordinates[i])) << " ";
    output_structure << std::to_string(std::get<2>(atom_coordinates[i])) << " ";
    output_structure << std::to_string(std::get<3>(atom_coordinates[i])) << "\n";
  }
  output_structure.close();
}

////////////////////////
// SURFACE MAP OUTPUT //
////////////////////////

void Model::writeTotalSurfaceMap(){
  writeTotalSurfaceMap(makeExportFileName(_output_folder, _data, 's'));
}

void Model::writeTotalSurfaceMap(const std::string file_path){
  // save commonly used variable
  std::array<unsigned long int,3> n_elements = _cell.getGrid(0).getNumElements();
  double vxl_length = _cell.getVxlSize();
  std::array<double,3> cell_min = _cell.getMin();
  std::array<double,3> origin;
  std::array<unsigned int,3> start_index = {0,0,0};
  std::array<unsigned int,3> end_index;
  for(int i = 0; i < 3; i++){
    end_index[i] = n_elements[i];
  }

  // in two probes mode, the outer space occupied by the second probe is useless for the surface map
  // thus the surface map can be reduced on each side by the radius of probe 2
  if(_data.probe_mode){
    for(int i = 0; i < 3; i++){
      start_index[i] += getProbeRad2()/vxl_length;
      end_index[i] -= getProbeRad2()/vxl_length;
      n_elements[i] = end_index[i] - start_index[i];
    }
  }

  // in unit cell analysis mode, only the content of the unit cell should be displayed in the surface map
  // one extra voxel is added on each side to see the surface up to the boundary of the unit cell
  if(_data.analyze_unit_cell){
    for(int i = 0; i < 3; i++){
      // +0.5 to avoid rounding errors
      start_index[i] = int(0.5 - cell_min[i]/vxl_length)-1;
      end_index[i] = 2 + start_index[i] + int(0.5 + _cart_matrix[i][i]/vxl_length);
      n_elements[i] = end_index[i] - start_index[i];
    }
  }

  for (int i = 0; i < 3; i++){
    origin[i] = cell_min[i] + ((double(start_index[i]) + 0.5) * vxl_length);
  }
  writeSurfaceMap(file_path, vxl_length, n_elements, origin, start_index, end_index);
}

void Model::writeCavitiesMaps(){
  writeCavitiesMaps("make_auto_name");
}

void Model::writeCavitiesMaps(const std::string file_path){
  // save commonly used variable
  double vxl_length = _cell.getVxlSize();
  std::array<double,3> cell_min = _cell.getMin();
  std::array<double,3> origin;
  std::array<unsigned int,3> start_index;
  std::array<unsigned int,3> end_index;

  // loop over each cavity id
  for(size_t id = 0; id < _data.cavities.size(); id++){
    std::array<unsigned long int,3> n_elements = _cell.getGrid(0).getNumElements();
    start_index = _data.cavities[id].min_index;
    end_index = _data.cavities[id].max_index;
    // increase size of surface map grid by 1 voxel in each direction to avoid having surfaces on the border of the map
    for(char i = 0; i < 3; i++){
      if(start_index[i] > 0){start_index[i]--;}
      // increase end_index twice because it should be above the range of indexes checked like vector and array sizes in C++
      if(end_index[i] < n_elements[i]){end_index[i]++;}
      if(end_index[i] < n_elements[i]){end_index[i]++;}
      n_elements[i] = end_index[i] - start_index[i];
      origin[i] = cell_min[i] + ((double(start_index[i]) + 0.5) * vxl_length);
    }
    std::string cavity_file_name;
    if (file_path == "make_auto_name"){
      cavity_file_name = makeExportFileName(_output_folder, _data, 's', id+1);
    }
    else{
      cavity_file_name = file_path;
      cavity_file_name.insert(file_path.find_last_of('.'), "_cav" + std::to_string(id+1));
    }
    /*
    // alternative with cav id + name of file
    size_t path_end = file_path.find_last_of("\\/");
    std::string cavity_file_name = file_path.substr(0,path_end+1) + "cav" + std::to_string(id+1) + "_"
      + file_path.substr(path_end+1,file_path.length()-path_end+1);
    */

    writeSurfaceMap(cavity_file_name, vxl_length, n_elements, origin, start_index, end_index, true, id);
  }
}

void Model::writeSurfaceMap(const std::string file_path,
                            double vxl_length,
                            std::array<unsigned long int,3> n_elements,
                            std::array<double,3> origin,
                            std::array<unsigned int,3> start_index,
                            std::array<unsigned int,3> end_index,
                            const bool partial_map,
                            const unsigned char id){
  bool issue_encountered = false;
  // assemble data
  Container3D<Voxel>* surface_map = &_cell.getGrid(0);

  // create map for assigning numbers to types
  const std::map<char,int> typeToNum =
    {{0b00000011, 0},
     {0b00000101, 1},
     {0b00001001, 6},
     {0b00010001, 4},
     {0b00100001, 2},
     {0b01000001, 2}};

  // create and open new file
  std::ofstream output_file;
  output_file.open(file_path);
  // comments
  output_file << "# OpenDX density file generated by MoloVol\n";
  output_file << "# Contains 3D surface map data to read in PyMOL, Chimera or ChimeraX\n";
  output_file << "# Data is written in C array order: In grid[x,y,z] the axis z is fastest\n";
  output_file << "# varying, then y, then finally x.\n";
  // line
  output_file << "object 1 class gridpositions counts";
  for (char i = 0; i < 3; i++){
    output_file << ' ' << n_elements[i];
  }
  output_file << "\n";
  // line
  output_file << "origin ";
  for (char i = 0; i < 3; i++){
    output_file << ' ' << origin[i];
  }
  output_file << '\n';
  // 3 lines
  for (char i = 0; i < 3; i++){
    output_file << "delta";
    for (char j = 0; j < 3; j++){
      output_file << ' ' << ((i==j)? vxl_length : 0);
    }
    output_file << '\n';
  }
  // line
  output_file << "object 2 class gridconnections counts";
  for (char i = 0; i < 3; i++){
    output_file << ' ' << n_elements[i];
  }
  output_file << '\n';
  // line
  output_file << "object 3 class array type double rank 0 items " << (n_elements[0]*n_elements[1]*n_elements[2]) << " data follows\n";
  // data
  int column = 0;
  for(unsigned long int x = start_index[0]; x < end_index[0]; x++){
    for(unsigned long int y = start_index[1]; y < end_index[1]; y++){
      for(unsigned long int z = start_index[2]; z < end_index[2]; z++){
        if (typeToNum.count(surface_map->getElement(x,y,z).getType()) != 0){
          if (partial_map? surface_map->getElement(x,y,z).getID() == _data.cavities[id].id : true){
            output_file << typeToNum.find(surface_map->getElement(x,y,z).getType())->second;
          }
          else {
            output_file << 0;
          }
        }
        else { // TODO inform the user that there is something odd with the surface map
          issue_encountered = true;
          output_file << -2;
        }
        output_file << ((column == 2)? '\n' : ' ');
        column = (column+1)%3;
      }
    }
  }
  if (column != 0) {
    output_file << '\n';
  }
  output_file << "attribute \"dep\" string \"positions\"\n";
  output_file << "object \"density\" class field\n";
  output_file << "component \"positions\" value 1\n";
  output_file << "component \"connections\" value 2\n";
  output_file << "component \"data\" value 3";

  // close the file
  output_file.close();
  if (issue_encountered) {Ctrl::getInstance()->displayErrorMessage(303);}
}

///////////////
// FILE NAME //
///////////////

static const std::map<char,std::string> s_file_descriptor{
  {'r' , "MoloVol-report"},
  {'s' , "surface-map"},
  {'c' , "struct-orthogonal-cell"},
  {'p' , "struct-partial-supercell"}
};

static const std::map<char,std::string> s_file_extension{
  {'r' , ".txt"},
  {'s' , ".dx"},
  {'c' , ".xyz"},
  {'p' , ".xyz"}
};

bool fileExists(const std::string&);
std::string rstripZeros(const std::string str);
std::string makeExportFileName(const std::string dir, const CalcReportBundle& data, const char filetype, const unsigned char n_cav){
  assert(s_file_descriptor.count(filetype));
  std::string filename = "";
  filename += fileName(data.atom_file_path);
  filename += "_" + s_file_descriptor.at(filetype);
  filename += "_grid-" + rstripZeros(std::to_string(data.grid_step));
  filename += "_rad1-" + rstripZeros(std::to_string(data.r_probe1));
  if (data.probe_mode){
    filename += "_rad2-" + rstripZeros(std::to_string(data.r_probe2));
  }
  if(filetype == 's'){
    filename += (n_cav)? "_cav" + std::to_string(n_cav) : "_full-structure";
  }
  std::string fullname = dir + "/" + filename + s_file_extension.at(filetype);
  int i = 2;
  while (fileExists(fullname)){
    fullname = dir + "/" + filename + "_" + std::to_string(i) + s_file_extension.at(filetype);
    i++;
  }
  return fullname;
}

std::string rstripZeros(std::string str){
  return str.erase(str.find_first_of('0',str.find_first_of('.')+2), str.find_first_of('.')+2 - str.length());
}

bool fileExists(const std::string& path){
  if (FILE *file = fopen(path.c_str(), "r")){
    fclose(file);
    return true;
  }
  else {return false;}
}
