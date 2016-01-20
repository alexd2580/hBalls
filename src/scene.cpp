#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <stack>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>

#define __USE_BSD // to get usleep
#include <unistd.h>

#include "scene.hpp"

using namespace std;

Material::Material(uint8_t type_,
                   float roughness_,
                   glm::vec3 passive_,
                   glm::vec3 active_)
    : type(type_), roughness(roughness_), passive(passive_), active(active_)
{
}

/**
 * Pop a model matrix
 */
void Scene::pop_matrix(void) { model_s.pop(); }

/**
 * Overwrites the current active top-stack matrix
 */
void Scene::put_matrix(glm::mat4& mat) { model_s.top() = mat; }

/**
 * Push a matrix, copy the old one to the current one
 * If there was no matrix on the stack, then set it to I
 */
void Scene::push_matrix(void)
{
  if(model_s.size() == 0)
    model_s.push(glm::mat4(1.0));
  else
  {
    glm::mat4 last = model_s.top();
    model_s.push(last);
  }
}

/**
 * Replaces the top stack matrix by an identity matrix.
 */
void Scene::load_identity(void)
{
  glm::mat4 mat(1.0);
  put_matrix(mat);
}

/**
 * Returns a copy of the current active top stack matrix.
 */
__attribute__((pure)) glm::mat4 Scene::get_matrix(void)
{
  return model_s.top();
}

/**
 * Data structure used in vertex buffers.
 * It's an interleaved list, containing
 * dpos -> ndc coords
 * dpos_i -> integer pixel position
 * pos -> world pos
 * normal -> normal vector
 * color -> a float color value [0..1]
 */

typedef struct interleave_ interleave;
struct interleave_ // TODO what is this?
{
  glm::vec3 pos;
  float color;
};

Scene::Scene(size_t const primitive_size)
{
  m_objects_buffer = new float[primitive_size];
  m_objects_float_index = 0;
  m_objects_count = 0;
  push_matrix();
}

Scene::~Scene(void) { delete[] m_objects_buffer; }

void Scene::clear_buffers(void) { m_objects_float_index = 0; }

__attribute__((pure)) size_t Scene::objects_byte_size(void) const
{
  return m_objects_float_index * sizeof(float);
}

__attribute__((pure)) unsigned int Scene::objects_count(void) const
{
  return m_objects_count;
}

__attribute__((pure)) float const* Scene::get_objects(void) const
{
  return m_objects_buffer;
}

void Scene::rotate(float angle, float x, float y, float z)
{
  rotate(angle, glm::vec3(x, y, z));
}

void Scene::rotate(float angle, glm::vec3 rotv)
{
  const glm::mat4 rotate = glm::rotate(model_s.top(), angle, rotv);
  model_s.top() = rotate;
}

void Scene::translate(glm::vec3 dirv)
{
  const glm::mat4 translate = glm::translate(model_s.top(), dirv);
  model_s.top() = translate;
}

void Scene::translate(float x, float y, float z)
{
  translate(glm::vec3(x, y, z));
}

//---------------------------------------

void Scene::push_vec3(glm::vec3 const& v)
{
  m_objects_buffer[m_objects_float_index + 0] = v.x;
  m_objects_buffer[m_objects_float_index + 1] = v.y;
  m_objects_buffer[m_objects_float_index + 2] = v.z;
  m_objects_float_index += 3;
}

void Scene::push_vertex(glm::vec3 const& v)
{
  glm::vec3 res = glm::vec3(model_s.top() * glm::vec4(v, 1.0f));
  push_vec3(res);
}

// Pushes the object to the objects_buffer at byte index freeOffset_r
void Scene::push_header(uint8_t const type, Material const& material)
{
  uint8_t* thisObj_u = (uint8_t*)(m_objects_buffer + m_objects_float_index);
  thisObj_u[0] = type;
  thisObj_u[1] = material.type;
  m_objects_float_index++;
  push_vec3(material.passive);
  push_vec3(material.active);
  m_objects_count++;
}

__attribute__((const)) float min3(float a, float b, float c)
{
  return min(a, min(b, c));
}
__attribute__((const)) float max3(float a, float b, float c)
{
  return max(a, max(b, c));
}

void Scene::triangle(Material const& material,
                     glm::vec3 const& a,
                     glm::vec3 const& b,
                     glm::vec3 const& c)
{
  glm::vec3 lower, upper;

  push_header(TRIANGLE, material);
  push_vertex(a);
  push_vertex(b);
  push_vertex(c);
}

void Scene::sphere(Material const& material, glm::vec3 const& pos, float radius)
{
  glm::vec3 const lower(pos - glm::vec3(radius));
  glm::vec3 const upper(pos + glm::vec3(radius));

  push_header(SPHERE, material);
  push_vertex(pos);
  m_objects_buffer[m_objects_float_index] = radius;
  m_objects_float_index++;
}

void Scene::quad(Material const& material,
                 glm::vec3 const& a,
                 glm::vec3 const& b,
                 glm::vec3 const& c,
                 glm::vec3 const& d)
{
  triangle(material, a, b, c);
  triangle(material, a, c, d);
}

/*
    case TRIANGLE_FAN:
        c_triangle[c_vertex_count] = performModelTransform(vertex);
        c_vertex_count++;
        if(c_vertex_count == 3)
        {
          push_object();
          c_triangle[1] = c_triangle[2];
          c_vertex_count = 2;
        }
        break;
    case TRIANGLE_STRIP:
        c_triangle[c_vertex_count] = performModelTransform(vertex);
        c_vertex_count++;
        if(c_vertex_count == 3)
        {
          push_object();
          c_triangle[0] = c_triangle[2];
          c_vertex_count = 2;
        }
        break;
*/
