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
                   float luminescence_,
                   glm::vec3 const& color_)
    : type(type_), roughness(roughness_), luminescence(luminescence_),
      color(color_)
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

Scene::Scene(ObjectsBuffer& objbuf) : buf(objbuf) { push_matrix(); }

void Scene::clear_buffers(void)
{
  buf.surf_float_index = 0;
  buf.lamp_float_index = 0;
  buf.surf_count = 0;
  buf.lamp_count = 0;
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

void Scene::push_vec3(glm::vec3 const& v, unsigned int const& index)
{
  buf.buffer[index + 0] = v.x;
  buf.buffer[index + 1] = v.y;
  buf.buffer[index + 2] = v.z;
}

void Scene::push_vertex(glm::vec3 const& v, unsigned int const& index)
{
  glm::vec3 res = glm::vec3(model_s.top() * glm::vec4(v, 1.0f));
  push_vec3(res, index);
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

  unsigned int index;
  if(material.luminescence > 0.0001f) // lamp
  {
    buf.lamp_float_index -= PRIM_SIZE;
    buf.lamp_count++;
    index = buf.lamp_float_index;
  }
  else
  {
    index = buf.surf_float_index;
    buf.surf_float_index += PRIM_SIZE;
    buf.surf_count++;
  }

  uint8_t* buf_u = (uint8_t*)(buf.buffer + index);
  buf_u[0] = material.type;
  buf.buffer[index + 1] = material.roughness;
  buf.buffer[index + 2] = material.luminescence;
  push_vec3(material.color, index + 3);

  push_vertex(a, index + 6);
  push_vertex(b, index + 9);
  push_vertex(c, index + 12);

  glm::vec3 norm = glm::cross(b - a, c - a);
  push_vertex(norm, index + 15);
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
