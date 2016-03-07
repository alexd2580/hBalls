#ifndef __RAYCG__H__
#define __RAYCG__H__

#include <cstdint>
#include <glm/glm.hpp>
#include <stack>

/**
 * Type of the surface.
 */
enum class SurfaceType : uint8_t
{
  diffuse = 1,
  metallic = 2,
  mirror = 3,
  glass = 4
};

struct Material
{
  Material(SurfaceType type,
           float roughness,
           float luminescence,
           glm::vec3 const& color);
  SurfaceType const type;
  float const roughness;
  float const luminescence;
  glm::vec3 const color;
};

static const unsigned int triangle_size = 3 * 3 + 3 + 1 + 1 + 1;

struct Camera
{
  glm::vec3 pos;
  glm::vec3 dir;
  glm::vec3 up;
  float fov;
  glm::vec3 left;
};

struct TriangleBuffer
{
  float* buffer;
  unsigned int index;

  TriangleBuffer(size_t const float_count);
  ~TriangleBuffer(void);

  void push_vec3(glm::vec3 const& v);
  void clear(void);
};

class Scene
{
private:
  /**
   * Matrix stack for model matrix.
   */
  std::stack<glm::mat4> model_s;

  void push_vertex(TriangleBuffer& buf, glm::vec3 const& v);

  unsigned int m_surf_count;
  unsigned int m_lamp_count;

public:
  /* data items MUST be aligned! max(type T) = 4byte */
  /**
   * The base pointers and their current offsets.
   */
  TriangleBuffer m_surf_buffer;
  TriangleBuffer m_lamp_buffer;

  Scene(size_t const surf_count, size_t const lamp_count);
  virtual ~Scene(void) = default;

  /**
   * Drop the current scene definition.
   */
  void clear_buffers(void);

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

  void quad(Material const& material,
            glm::vec3 const& a,
            glm::vec3 const& b,
            glm::vec3 const& c,
            glm::vec3 const& d);
};

#endif
