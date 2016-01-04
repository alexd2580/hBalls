#ifndef __RAYCG__H__
#define __RAYCG__H__

#include <glm/glm.hpp>
#include <cstdint>
#include <stack>
#include "octree.hpp"

/** PRIMITIVE TYPE **/
#define TRIANGLE ((uint8_t)1)
#define SPHERE ((uint8_t)2)

/** SURFACE TYPE **/
#define DIFFUSE ((uint8_t)1)
#define METALLIC ((uint8_t)2)
#define MIRROR ((uint8_t)3)
#define GLASS ((uint8_t)4)

/**
uint8 primitive_type;
uint8 material_type;
uin16 __padding__
float3 passive;
float3 active;

data...

sphere -> 11*4 byte
triangle -> 16*4 byte
**/

struct Material
{
  Material(uint8_t type, float roughness, glm::vec3 passive, glm::vec3 active);
  uint8_t const type;
  float const roughness;
  glm::vec3 const passive;
  glm::vec3 const active;
};

/**
 * Used to define the scene in a
 */
class Scene
{
private:
  /* SCREEN SIZES */
  unsigned int const size_w;
  unsigned int const size_h;

  int const view_min_x;
  int const view_max_x;
  int const view_min_y;
  int const view_max_y;
  /* SCREEN SIZES */

  /**
   * Matrix stack for model matrix
   */
  std::stack<glm::mat4> model_s;

  /* data items MUST be aligned! max(type T) = 4byte */
  /**
   * The base pointer and the current offset.
   */
  float* objects_buffer;
  unsigned int objects_buffer_i;

  Octree* octree;

  void push_vec3(glm::vec3 const& v);
  void push_vertex(glm::vec3 const& v);
  void push_header(uint8_t const type, Material const& material);

public:
  Scene(unsigned int const w,
        unsigned int const h,
        size_t const primitive_size,
        AABB const& aabb);
  ~Scene(void);

  /**
   * Drop the current scene definition.
   * Reinitializes the octree with its current aabb
   */
  void clear_buffers(void); // clears the scene

  /**
   * Returns the scene buffer size in bytes (the fileld part)
   */
  size_t objects_byte_size(void) const;
  float const* get_objects(void) const;
  Octree const* get_octree(void) const;

  void pop_matrix(void);
  void push_matrix(void);
  void load_identity(void);

  glm::mat4 get_matrix(void);
  void put_matrix(glm::mat4& mat);

  void printInfo(void);

  //----------------------

  void rotate(float angle, glm::vec3 rotv);
  void rotate(float angle, float x, float y, float z);
  void translate(glm::vec3 dirv);
  void translate(float x, float y, float z);

  /* SCENE DESCRIPTION */
  void triangle(Material const& material,
                glm::vec3 const& a,
                glm::vec3 const& b,
                glm::vec3 const& c);

  void sphere(Material const& material, glm::vec3 const& pos, float r);

  void quad(Material const& material,
            glm::vec3 const& a,
            glm::vec3 const& b,
            glm::vec3 const& c,
            glm::vec3 const& d);
};

#endif
