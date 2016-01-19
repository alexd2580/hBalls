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

void Scene::printInfo(void)
{
  cout << "OpenCGL info - current state:" << endl;
  cout << "Size: w=" << size_w << " h=" << size_h << endl;
  cout << "Viewport: x=" << view_min_x << "->" << view_max_x << " ";
  cout << "y=" << view_min_y << "->" << view_max_y << endl;
  cout << endl;
}

Scene::Scene(unsigned int w,
             unsigned int h,
             size_t const primitive_size,
             AABB const& aabb)
    : size_w(w), size_h(h), view_min_x(-int(size_w) / 2),
      view_max_x(size_w / 2), view_min_y(-int(size_h) / 2),
      view_max_y(size_h / 2)
{
  objects_buffer = new float[primitive_size];
  objects_buffer_i = 0;
  octree = new Octree(aabb);
  push_matrix();
}

Scene::~Scene(void)
{
  delete[] objects_buffer;
  delete octree;
}

void Scene::clear_buffers(void)
{
  objects_buffer_i = 0;
  AABB const& aabb = octree->aabb;
  Octree* clean = new Octree(aabb);
  delete octree;
  octree = clean;
}

__attribute__((pure)) size_t Scene::objects_byte_size(void) const
{
  return objects_buffer_i * sizeof(float);
}

__attribute__((pure)) float const* Scene::get_objects(void) const
{
  return objects_buffer;
}

__attribute__((pure)) Octree const* Scene::get_octree(void) const
{
  return octree;
}

/*#include"pngwrapper.hpp"

void printScreen(string& fpath, OpenCL& cl)
{
    flushRTBuffer(cl);
    cout << "Reading buffer" << endl;
    OpenCL::readBufferBlocking(framebufferqueue,
      frameBuf_mem, size_w*size_h*sizeof(uint8_t), frame_buffer);
    cout << "Saving to file" << endl;
    saveBWToFile(frame_buffer, size_w, size_h, fpath);
    cout << "Done" << endl;
    cout.flush();
}*/

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
  objects_buffer[objects_buffer_i + 0] = v.x;
  objects_buffer[objects_buffer_i + 1] = v.y;
  objects_buffer[objects_buffer_i + 2] = v.z;
  objects_buffer_i += 3;
}

void Scene::push_vertex(glm::vec3 const& v)
{
  glm::vec3 res = glm::vec3(model_s.top() * glm::vec4(v, 1.0f));
  push_vec3(res);
}

// Pushes the object to the objects_buffer at byte index freeOffset_r
void Scene::push_header(uint8_t const type, Material const& material)
{
  uint8_t* thisObj_u = (uint8_t*)(objects_buffer + objects_buffer_i);
  thisObj_u[0] = type;
  thisObj_u[1] = material.type;
  objects_buffer_i++;
  push_vec3(material.passive);
  push_vec3(material.active);
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
  glm::vec3 ab = (a + b) / 2.0f;
  glm::vec3 ac = (a + c) / 2.0f;
  glm::vec3 bc = (b + c) / 2.0f;

  glm::vec3 lower, upper;

  lower = glm::vec3(
      min3(ab.x, b.x, bc.x), min3(ab.y, b.y, bc.y), min3(ab.z, b.z, bc.z));
  upper = glm::vec3(
      max3(ab.x, b.x, bc.x), max3(ab.y, b.y, bc.y), max3(ab.z, b.z, bc.z));
  AABB aabb0(lower, upper);
  octree->insert(objects_buffer_i, aabb0);

  push_header(TRIANGLE, material);
  push_vertex(ab);
  push_vertex(b);
  push_vertex(bc);

  lower = glm::vec3(
      min3(ac.x, bc.x, c.x), min3(ac.y, bc.y, c.y), min3(ac.z, bc.z, c.z));
  upper = glm::vec3(
      max3(ac.x, bc.x, c.x), max3(ac.y, bc.y, c.y), max3(ac.z, bc.z, c.z));
  AABB aabb1(lower, upper);
  octree->insert(objects_buffer_i, aabb1);

  push_header(TRIANGLE, material);
  push_vertex(bc);
  push_vertex(c);
  push_vertex(ac);

  lower = glm::vec3(
      min3(ac.x, ab.x, bc.x), min3(ac.y, ab.y, bc.y), min3(ac.z, ab.z, bc.z));
  upper = glm::vec3(
      max3(ac.x, ab.x, bc.x), max3(ac.y, ab.y, bc.y), max3(ac.z, ab.z, bc.z));
  AABB aabb2(lower, upper);
  octree->insert(objects_buffer_i, aabb2);

  push_header(TRIANGLE, material);
  push_vertex(ac);
  push_vertex(ab);
  push_vertex(bc);

  lower = glm::vec3(
      min3(a.x, ab.x, ac.x), min3(a.y, ab.y, ac.y), min3(a.z, ab.z, ac.z));
  upper = glm::vec3(
      max3(a.x, ab.x, ac.x), max3(a.y, ab.y, ac.y), max3(a.z, ab.z, ac.z));
  AABB aabb3(lower, upper);
  octree->insert(objects_buffer_i, aabb3);

  push_header(TRIANGLE, material);
  push_vertex(a);
  push_vertex(ab);
  push_vertex(ac);
}

void Scene::sphere(Material const& material, glm::vec3 const& pos, float radius)
{
  glm::vec3 const lower(pos - glm::vec3(radius));
  glm::vec3 const upper(pos + glm::vec3(radius));
  AABB aabb(lower, upper);
  octree->insert(objects_buffer_i, aabb);

  push_header(SPHERE, material);
  push_vertex(pos);
  objects_buffer[objects_buffer_i] = radius;
  objects_buffer_i++;
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
