#ifndef VOXEL_H

#define VOXEL_H

#include <vector>
#include <array>

class AtomTree;
struct Atom;
struct AtomNode;
class Voxel{
  public:
    Voxel();
	Voxel& get(const short x, const short y, const short z);
	Voxel& get(const short i);
    char getType() const;

   /* replaced by other determineType function
    void determineType
      (const std::vector<Atom>& atoms, 
       std::array<double,3> pos,
       const double& grid_size,
       const double max_depth);
    */

    char determineType
       (std::array<double,3> pos,
       const double& grid_size,
       const double max_depth,
       const AtomTree& atomtree);
    
    void determineTypeSingleAtom
      (const Atom& atom, 
       std::array<double,3> pos, // voxel centre
       const double& grid_size,
       const double max_depth);
   
    void traverseTree
      (const AtomNode* node, 
       int dim, 
       const double& at_rad, 
       const double& vxl_rad, 
       const std::array<double,3> vxl_pos,
       const double& grid_size, 
       const double& max_depth);
    size_t tallyVoxelsOfType(const char volume_type, const int max_depth);

  private:
    std::vector<Voxel> data; // empty or exactly 8 elements
    char type;
    // types
    // 'a' : inside of atom
    // 'e' : empty
    // 'm' : mixed
};

#endif
