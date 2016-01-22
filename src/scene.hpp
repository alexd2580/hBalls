#ifndef __RAYCG__H__
#define __RAYCG__H__

#include <glm/glm.hpp>
#include <cstdint>
#include <stack>

/** PRIMITIVE TYPE **/
#define TRIANGLE ((uint8_t)1)
#define SPHERE ((uint8_t)2)

/** SURFACE TYPE **/
#define DIFFUSE ((uint8_t)1)
#define METALLIC ((uint8_t)2)
#define MIRROR ((uint8_t)3)
#define GLASS ((uint8_t)4)

/**
## HEADER ##
type            :: Uint8
material        :: Uint8
__padding__     :: Uint16
roughness       :: Float
-- 0 -> mirror-like; 1 -> diffuse;
luminescence    :: Float
-- values can (and for lamps they should) be greater than 1
color           :: Float3

## BODY ##
payload     :: Data

sizeof(Sphere) = (6+4) * 4 byte
sizeof(Triengle) = (6+9) * 4 byte
**/

struct Material
{
  Material(uint8_t type,
           float roughness,
           float luminescence,
           glm::vec3 const& color);
  uint8_t const type;
  float const roughness;
  float const luminescence;
  glm::vec3 const color;
};

struct Camera
{
  glm::vec3 pos;
  glm::vec3 dir;
  glm::vec3 up;
  float fov;
  glm::vec3 left;
};

/**
 * data items MUST be aligned! max(type T) = 4byte
 * The objects buffer and offsets for surfaces and lamps.
 */
struct ObjectsBuffer
{
  float* buffer;
  unsigned int surf_float_index;
  unsigned int lamp_float_index;
  unsigned int surf_count;
  unsigned int lamp_count;
};
/**
 * Used to define the scene in a
 */
class Scene
{
private:
  /**
   * ObjectsBuffer
   */
  ObjectsBuffer buf;

  /**
   * Matrix stack for model matrix
   */
  std::stack<glm::mat4> model_s;

  void push_vec3(glm::vec3 const& v, unsigned int const& index);
  void push_vertex(glm::vec3 const& v, unsigned int const& index);
  unsigned int push_header(uint8_t const type, Material const& material);

public:
  Scene(uint32_t primitive_size);
  ~Scene(void);

  /**
   * Drop the current scene definition.
   * Reinitializes the octree with its current aabb
   */
  void clear_buffers(void); // clears the scene

  /**
   * Returns the relevant data for addressing the objects buffer.
   */
  ObjectsBuffer const& get_objects(void) const;

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
