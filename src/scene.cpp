#include <cmath>
#include <cstdlib>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <stack>
#include <string>
#include <vector>

#include "scene.hpp"

using namespace std;

Material::Material(SurfaceType type_,
                   float roughness_,
                   float luminescence_,
                   glm::vec3 const& color_)
    : type(type_), roughness(roughness_), luminescence(luminescence_),
      color(color_)
{
}

TriangleBuffer::TriangleBuffer(size_t const float_count)
{
  buffer = new float[float_count];
  index = 0;
}

TriangleBuffer::~TriangleBuffer(void) { delete[] buffer; }

void TriangleBuffer::push_vec3(glm::vec3 const& v)
{
  buffer[index + 0] = v.x;
  buffer[index + 1] = v.y;
  buffer[index + 2] = v.z;
  index += 3;
}

void TriangleBuffer::clear(void) { index = 0; }

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

Scene::Scene(size_t const surf_count, size_t const lamp_count)
    : m_surf_buffer(surf_count * triangle_size),
      m_lamp_buffer(lamp_count * triangle_size)
{
  m_surf_count = 0;
  m_lamp_count = 0;
  push_matrix();
}

void Scene::clear_buffers(void)
{
  m_surf_buffer.clear();
  m_lamp_buffer.clear();

  m_surf_count = 0;
  m_lamp_count = 0;
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

void Scene::push_vertex(TriangleBuffer& buf, glm::vec3 const& v)
{
  glm::vec3 res = glm::vec3(model_s.top() * glm::vec4(v, 1.0f));
  buf.push_vec3(res);
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
  bool is_lamp = material.luminescence > 0.00001f;
  TriangleBuffer& buf = is_lamp ? m_lamp_buffer : m_surf_buffer;
  unsigned int& count = is_lamp ? m_lamp_count : m_surf_count;

  uint8_t* buffer_u = (uint8_t*)(buf.buffer + buf.index);
  buffer_u[0] = (uint8_t)material.type;
  buf.buffer[buf.index + 1] = material.roughness;
  buf.buffer[buf.index + 2] = material.luminescence;
  buf.index += 3;
  buf.push_vec3(material.color);

  push_vertex(buf, a);
  push_vertex(buf, b);
  push_vertex(buf, c);

  count++;
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
