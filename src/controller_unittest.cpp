#include "controller.h"
#include "base.h"
#include "atom.h" // i don't know why
#include "model.h"
#include "misc.h"
#include <cmath>
#include <map>

bool Ctrl::unittestExcluded(){
  if(_current_calculation == NULL){_current_calculation = new Model();}

  // parameters for unittest:
  const std::string atom_filepaths[] =
  {getResourcesDir() + "/probetest_pair.xyz", getResourcesDir() + "/probetest_triplet.xyz", getResourcesDir() + "/probetest_quadruplet.xyz"};
  const std::string elem_filepath = Ctrl::getDefaultElemPath();
  const double grid_step = 0.1;
  const int max_depth = 4;
  const double rad_probe1 = 1.2;
  const double expected_volumes[] = {1.399000, 4.393000, 9.054000};

  double error[3];
  CalcReportBundle data[3];
  for (int i = 0; i < 3; i++){
    _current_calculation->readAtomsFromFile(atom_filepaths[i], false);
    std::vector<std::string> included_elements = _current_calculation->listElementsInStructure();

    _current_calculation->setParameters(
        atom_filepaths[i],
        "./output",
        false,
        false,
        false,
        false,
        rad_probe1,
        0,
        grid_step,
        max_depth,
        false,
        false,
        false,
        _current_calculation->extractRadiusMap(elem_filepath),
        included_elements);

    data[i] = _current_calculation->generateData();
    if(data[i].success){
      error[i] = abs(data[i].volumes[0b00000101]-expected_volumes[i])/data[i].volumes[0b00000101];
    }
    else{
      std::cout << "Calculation failed" << std::endl;
      return false;
    }
  }

  for (int i = 0; i < 3; i++){
    printf("f: %40s, g: %4.1f, d: %4i, r: %4.1f\n", atom_filepaths[i].c_str(), grid_step, max_depth, rad_probe1);
    printf("Error: %20.10f, Time: %10.5f\n", error[i], data[i].getTime());
  }

  return true;
}

bool Ctrl::unittestProtein(){
  if(_current_calculation == NULL){_current_calculation = new Model();}

  // parameters for unittest:
  const std::string atom_filepath = getResourcesDir() + "/6s8y.xyz";
  const std::string elem_filepath = Ctrl::getDefaultElemPath();
  const double grid_step = 0.1;
  const int max_depth = 4;
  const double rad_probe1 = 1.2;
  const double expected_vdwVolume = 14337.422000;
  const double expected_time = 67;

  double error_vdwVolume;
  double diff_time;
  CalcReportBundle data;
  _current_calculation->readAtomsFromFile(atom_filepath, false);
  std::vector<std::string> included_elements = _current_calculation->listElementsInStructure();

  _current_calculation->setParameters(
      atom_filepath,
      "./output",
      false,
      false,
      false,
      false,
      rad_probe1,
      0,
      grid_step,
      max_depth,
      false,
      false,
      false,
      _current_calculation->extractRadiusMap(elem_filepath),
      included_elements);

  data = _current_calculation->generateData();
  if(data.success){
    error_vdwVolume = abs(data.volumes[0b00000011]-expected_vdwVolume)/data.volumes[0b00000011];
    diff_time = data.getTime() - expected_time;

    printf("f: %40s, g: %4.1f, d: %4i, r: %4.1f\n", atom_filepath.c_str(), grid_step, max_depth, rad_probe1);
    printf("Error vdW: %20.10f, Excluded: %20.10f, Time: %10.5f s\n", error_vdwVolume, data.volumes[0b00000101], diff_time);
    printf("Type Assignment: %10.5f s, Volume Tally: %10.5f s\n", data.getTime(1), data.getTime(2));
  }
  else{
    std::cout << "Calculation failed" << std::endl;
  }
  return true;
}

bool Ctrl::unittestRadius(){
  if(_current_calculation == NULL){_current_calculation = new Model();}

  // parameters for unittest:
  const std::string atom_filepath = getResourcesDir() + "/probetest_pair.xyz";
  const std::string elem_filepath = Ctrl::getDefaultElemPath();
  double rad_probe2 = 1.2;
  bool two_probe = true;

  std::unordered_map<std::string, double> rad_map = _current_calculation->extractRadiusMap(elem_filepath);
  {int max_depth = 4;
    //for (int max_depth = 4; max_depth < ; max_depth++){
    {double grid_step = 0.1;
      //for (double grid_step = 1; grid_step>0.01; grid_step-=0.01){
      {double rad_probe1 = 0;
        //for (double rad_probe1 = 2; rad_probe1 < 2.01; rad_probe1 += 0.1){


        CalcReportBundle data;
        _current_calculation->readAtomsFromFile(atom_filepath, false);
        std::vector<std::string> included_elements = _current_calculation->listElementsInStructure();

        _current_calculation->setParameters(
            atom_filepath,
            "./output",
            false,
            false,
            false,
            two_probe,
            rad_probe1,
            rad_probe2,
            grid_step,
            max_depth,
            false,
            false,
            false,
            rad_map,
            included_elements);

        data = _current_calculation->generateData();

        if(data.success){
          printf("f: %40s, g: %4.2f, d: %4i, r: %4.1f\n", atom_filepath.c_str(), grid_step, max_depth, rad_probe1);
          printf("vdW: %20.10f, Excluded: %20.10f\n", data.volumes[0b00000011], data.volumes[0b00000101]);
          printf("Time elapsed: %10.5f s\n", data.getTime());
        }
        else{
          std::cout << "Calculation failed" << std::endl;
        }
      }
    }
  }
  return true;
}

bool Ctrl::unittest2Probe(){
  if(_current_calculation == NULL){_current_calculation = new Model();}

  // parameters for unittest:
  const std::string atom_filepath = getResourcesDir() + "/probetest_quadruplet.xyz";
  const std::string elem_filepath = Ctrl::getDefaultElemPath();
  double rad_probe2 = 2;
  double rad_probe1 = 0.5;
  bool two_probe = true;
  int max_depth = 4;
  double grid_step = 0.1;

  CalcReportBundle data;
  _current_calculation->readAtomsFromFile(atom_filepath, false);
  std::vector<std::string> included_elements = _current_calculation->listElementsInStructure();

  _current_calculation->setParameters(
      atom_filepath,
      "./output",
      false,
      false,
      false,
      two_probe,
      rad_probe1,
      rad_probe2,
      grid_step,
      max_depth,
      false,
      false,
      false,
      _current_calculation->extractRadiusMap(elem_filepath),
      included_elements);

  data = _current_calculation->generateData();

  if(data.success){
    printf("f: %40s, g: %4.2f, d: %4i, r1: %4.1f, r2: %4.1f\n",
        atom_filepath.c_str(), grid_step, max_depth, rad_probe1, rad_probe2);
    printf("vdW: %20.10f, Excluded: %20.10f\n", data.volumes[0b00000011]-28.866, data.volumes[0b00000101]-6.224);
    printf("P1 Core: %20.10f, P1 Shell: %20.10f\n", data.volumes[0b00001001]-0.202, data.volumes[0b00010001]-5.702);
    printf("P2 Core: %20.10f, P2 Shell: %20.10f\n", data.volumes[0b00100001]-681.534, data.volumes[0b01000001]-309.664);
    printf("Time elapsed: %10.5f s\n", data.getTime()-2.12);
  }
  else{
    std::cout << "Calculation failed" << std::endl;
  }
  return true;
}

bool Ctrl::unittestSurface(){
  if(_current_calculation == NULL){_current_calculation = new Model();}

  // parameters for unittest:
  const std::string atom_filepath = getResourcesDir() + "/6s8y.xyz";
  const std::string elem_filepath = Ctrl::getDefaultElemPath();
  double rad_probe2 = 2;
  double rad_probe1 = 1.2;
  bool two_probe = false;
  bool surf_areas = true;
  int max_depth = 4;
  double grid_step = 0.1;

  CalcReportBundle data;
  _current_calculation->readAtomsFromFile(atom_filepath, false);
  std::vector<std::string> included_elements = _current_calculation->listElementsInStructure();

  _current_calculation->setParameters(
      atom_filepath,
      "./output",
      false,
      false,
      surf_areas,
      two_probe,
      rad_probe1,
      rad_probe2,
      grid_step,
      max_depth,
      false,
      false,
      false,
      _current_calculation->extractRadiusMap(elem_filepath),
      included_elements);

  data = _current_calculation->generateData();

  if(data.success){
    printf("f: %40s, g: %4.2f, d: %4i, r1: %4.1f\n", atom_filepath.c_str(), grid_step, max_depth, rad_probe1);
    printf("vdW: %20.10f, Excluded: %20.10f\n", data.volumes[0b00000011]-14337.45, data.volumes[0b00000101]-3952.565);
    printf("P1 Core: %20.10f, P1 Shell: %20.10f\n", data.volumes[0b00001001]-103215.383, data.volumes[0b00010001]-9157.002);
    std::vector<double> cav_core = {7993.6335860991,5.8886820000,2.8789670000,5.2640810000,3.3330790000,1.2919890000,0.6405240000,1.1345240000,0.8640930000,1.4862960000,0.2028680000,0.1426710000,0.1014340000,0.0508800000,0.0508800000,0.0508800000,0.0508800000,0.0508800000,0.0508800000};
    std::vector<double> cav_shell = {6881.3565860501,48.9658290000,52.5750410000,46.1779240000,41.7236070000,33.4249720000,29.8395440000,29.0761200000,25.4241840000,28.5505040000,22.4953940000,21.2451750000,20.8986260000,16.7730830000,11.5013900000,10.8661130000,10.3173070000, 5.0289150000, 3.0740820000};

    for (size_t i = 0; i< data.cavities.size(); ++i){
      printf("Cavity surf core: %20.10f A^3\n", data.getCavSurfCore(i)-cav_core[i]);
      printf("Cavity surf shell: %20.10f A^3\n", data.getCavSurfShell(i)-cav_shell[i]);
    }
    printf("Time elapsed: %10.5f s\n", data.getTime()-0.83+0.74-136.8);
  }
  else{
    std::cout << "Calculation failed" << std::endl;
  }
  return true;
}

bool Ctrl::unittestFloodfill(){
  if(_current_calculation == NULL){_current_calculation = new Model();}

  // parameters for unittest:
  const std::string atom_filepath = getResourcesDir() + "/Pd6L4_open_cage_Fujita.xyz";
  const std::string elem_filepath = Ctrl::getDefaultElemPath();
  double rad_probe2 = 4;
  double rad_probe1 = 1.2;
  bool two_probe = true;
  bool surf_areas = false;
  int max_depth = 4;
  double grid_step = 0.3;

  CalcReportBundle data;
  _current_calculation->readAtomsFromFile(atom_filepath, false);
  std::vector<std::string> included_elements = _current_calculation->listElementsInStructure();

  _current_calculation->setParameters(
      atom_filepath,
      "./output",
      false,
      false,
      surf_areas,
      two_probe,
      rad_probe1,
      rad_probe2,
      grid_step,
      max_depth,
      false,
      false,
      false,
      _current_calculation->extractRadiusMap(elem_filepath),
      included_elements);

  data = _current_calculation->generateData();

  if(data.success){
    printf("f: %40s, g: %4.2f, d: %4i, r1: %4.1f\n", atom_filepath.c_str(), grid_step, max_depth, rad_probe1);
    printf("vdW: %20.10f, Excluded: %20.10f\n", data.volumes[0b00000011]-28.948, data.volumes[0b00000101]-1.86);
    printf("P1 Core: %20.10f, P1 Shell: %20.10f\n", data.volumes[0b00001001]-247.748, data.volumes[0b00010001]-49.124);
    //std::vector<double> cav = {294.75,1.34,0.252,0.187,0.187,0.078,0.078};
    for (size_t i = 0; i< data.cavities.size(); ++i){
      //printf("Cavity vol: %20.10f A^3\n", data.getCavVolume(i)-cav[i]);
      printf("Cavity vol: %20.10f A^3\n", data.getCavVolume(i));
    }
    printf("Time elapsed: %10.5f s\n", data.getTime()-0.83+0.74);
  }
  else{
    std::cout << "Calculation failed" << std::endl;
  }
  return true;
}
