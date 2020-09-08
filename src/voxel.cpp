#include "voxel.h"
#include "misc.h"
#include "atom.h"
#include "atomtree.h"
#include <cmath> // abs, pow
#include <cassert>

///////////////////
// AUX FUNCTIONS //
///////////////////

inline double calcSphereOfInfluence(const double& grid_size, const double& max_depth){
  return pow(3,0.5)*(grid_size * pow(2,max_depth))/2; // TODO: avoid expensive pow function
}

/////////////////
// CONSTRUCTOR //
/////////////////

Voxel::Voxel(){
  type = 'e';
}

////////////
// ACCESS //
////////////

Voxel& Voxel::get(const short x, const short y, const short z){
  assert(x<2);
  assert(y<2);
  assert(z<2);
  return data[4 * z + 2 * y + x];
}

Voxel& Voxel::get(const short i) {
  assert(i <= 8);
  return data[i];
}

char Voxel::getType() const{
  return type;
}

//////////////
// SET TYPE //
//////////////

char Voxel::determineType
   (std::array<double,3> vxl_pos, // voxel centre
   const double& grid_size,
   const double max_depth,
   const AtomTree& atomtree)
{
  double at_rad = atomtree.getMaxRad();
  double vxl_rad = calcSphereOfInfluence(grid_size, max_depth);
    
  traverseTree(atomtree.getRoot(), 0, at_rad, vxl_rad, vxl_pos, grid_size, max_depth); 

  if(type == 'm'){
    // TODO define method for this part Voxel::splitVoxel()
    // split into 8 subvoxels
	short resultcount = 0;
    for(int i = 0; i < 8; i++){
      data.push_back(Voxel());
      // modify position
      std::array<int,3> factors = {
        (i%2) >= 1 ? 1 : -1,
        (i%4) >= 2 ? 1 : -1,
         i    >= 4 ? 1 : -1};
     
      std::array<double,3> new_pos;
      for(int dim = 0; dim < 3; dim++){
        new_pos[dim] = vxl_pos[dim] + factors[dim] * grid_size * std::pow(2,max_depth-2);//why -2?
      }
      resultcount += data[i].determineType(new_pos, grid_size, max_depth-1, atomtree);
    }
	  //determine if all children have the same type
	  if (resultcount == 'a'*8){
		  type = 'a';
	  } else if (resultcount == 'e'*8) {
		  type = 'e';
	  }
  }
	return type;
}

// use the properties of the binary tree to recursively traverse the tree and 
// only check the voxel type with respect to relevant atoms. at the end of this 
// method, the type of the voxel will have been set.
void Voxel::traverseTree
  (const AtomNode* node, 
   int dim, const double& at_rad, 
   const double& vxl_rad, 
   const std::array<double,3> vxl_pos, 
   const double& grid_size, 
   const double& max_depth){
  if (node == NULL){
    return;
  }
  
  std::array<double,3> at_pos = node->atom->getPos();
  double dist1D = distance(at_pos, vxl_pos, dim);

  // check if voxel is close enough to atom
  if (abs(dist1D) > (vxl_rad + at_rad)){ // if not continue to next child
      traverseTree(dist1D < 0 ? node->left_child : node->right_child,
				   (dim+1)%3, at_rad, vxl_rad, vxl_pos, grid_size, max_depth);
  } else{ // if voxel is close enough, check distance to the node's atom. if needed,
    // continue with both children
    determineTypeSingleAtom(*(node->atom), vxl_pos, grid_size, max_depth);
    if (type != 'a'){
      traverseTree(node->left_child, (dim+1)%3, at_rad, vxl_rad, vxl_pos, grid_size, max_depth);
    }
    if (type != 'a'){
      traverseTree(node->right_child, (dim+1)%3, at_rad, vxl_rad, vxl_pos, grid_size, max_depth);
    }
  }
  return;
}

void Voxel::determineTypeSingleAtom
  (const Atom& atom, 
   std::array<double,3> vxl_pos, // voxel centre
   const double& grid_size,
   const double max_depth)
{
  // only mixed and empty type voxels need to be considered
  if(type == 'a'){
    return;
  }
  if(max_depth == 0){ // for bottom level voxels

    // check if centre is inside atom
    double dist_vxl_at = distance(vxl_pos, atom.getPos());
    if (atom.rad > dist_vxl_at){// voxel centre is inside atom radius
      type = 'a';
      return;
    }
    // else type remains unchanged, i.e., empty 
  }
  else{ // for higher level voxels
    
    // determine the radius of the sphere that contains the voxel
    // the voxels side length is given by the grid_size (i.e. the side length
    // of a bottom level voxel) multiplied with 2^max_depth. This value multiplied
    // with the sqrt of 3 gives the diagonal of the voxel. The radius is half the
    // diagonal.
    double radius_of_influence = pow(3,0.5)*(grid_size * pow(2,max_depth))/2; // TODO: avoid expensive pow function

    double dist_vxl_at = distance(vxl_pos, atom.getPos());
    
    // is voxel inside atom? 
    if(atom.rad > (dist_vxl_at + radius_of_influence)){ 
      type = 'a'; // in atom
      return;
    }
    // is voxel partially inside atom?
    else if(atom.rad > (dist_vxl_at - radius_of_influence)){
      type = 'm'; // mixed
      return;
    }
    // else type remains unchanged
  }
  return;
}

///////////
// TALLY //
///////////

size_t Voxel::tallyVoxelsOfType(const char volume_type, const int max_depth){
  // if voxel is of type "mixed" (i.e. data vector is not empty)
  if(!data.empty()){
    // then total number of voxels is given by tallying all subvoxels
    size_t total = 0;
    for(int i = 0; i < 8; i++){
      total += data[i].tallyVoxelsOfType(volume_type, max_depth-1);
    }
    return total;
  }
  // if voxel is of the type of interest, tally the voxel in units of bottom level voxels
  else if(type == volume_type){
    return pow(pow(2,max_depth),3); // return number of bottom level voxels
  }
  // if neither empty nor of the type of interest, then the voxel doesn't count towards the total
  return 0;
}
