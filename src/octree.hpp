#ifndef __FUNNYOCTREE__
#define __FUNNYOCTREE__

#include <glm/glm.hpp>
#include <vector>

class AABB
{
public:
  glm::vec3 const lower;
  glm::vec3 const upper;

  AABB(glm::vec3 const& lower_, glm::vec3 const& upper_);
  virtual ~AABB(void) {}

  bool is_subspace_of(AABB const& space) const;
  bool is_superspace_of(AABB const& space) const;
};

class Octree
{
public:
  AABB const aabb;
  Octree* sub[8];

  Octree(AABB const& aabb_);
  Octree(glm::vec3 const& lower, glm::vec3 const& upper);

  virtual ~Octree(void);

  std::vector<unsigned int> primitives;

  Octree* insert(unsigned int val, AABB const& space);
  void print_info(void) const;

  /**
  Format:
  float3 lower
  float3 upper
  unsigned int size
  (size * unsigned int) ids
  (8 * int) offsets <-
    float-size units difference to beginning of thus subtree
    -1 if there is no subtree there.
  => the subtrees

  **/

  /**
   * Returns the space that storing this octree
   * required in float-sized units
   * @param buffer_f - A pointer to the start of the available space.
   */
  unsigned int print_to_array(float* buffer_f) const;

  static Octree* reconstruct(float* data_f);
};

#endif
