#ifndef ATOM_H

#define ATOM_H

#include "vector.h"
#include <exception>
#include <vector>
#include <array>
#include <iostream>

struct Atom{ 
  Atom() : pos_x(0), pos_y(0), pos_z(0), rad(-1), number(0), symbol("") {}
  Atom(const double& x_inp, const double& y_inp, const double& z_inp, const std::string& symbol_inp, const double& rad_inp, const int& elem_Z_inp)
    : pos_x(x_inp), pos_y(y_inp), pos_z(z_inp), rad(rad_inp), number(elem_Z_inp), symbol(symbol_inp) {}

  double pos_x, pos_y, pos_z, rad;
  unsigned int number;
  std::string symbol;

  const std::array<double,3> getPos() const {
    return {pos_x, pos_y, pos_z};
  }
  
  const Vector getPosVec() const {
    return Vector(pos_x, pos_y, pos_z);
  }

  const double getRad() const {
    return rad;
  }

  const double getCoordinate(const char& dim){
    switch(dim){
      case 0: return pos_x;
      case 1: return pos_y;
      case 2: return pos_z;
    }
    throw std::invalid_argument("");
  }

  void print(){
    printf("Atom {%s, (%1.3f, %1.3f, %1.3f)}", symbol.c_str(), pos_x, pos_y, pos_z);
  }

  bool isValid() const {return (rad >= 0);}
};

#endif
