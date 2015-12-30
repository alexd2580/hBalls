#include<iostream>
#include<cstdlib>
#include<cstring>
#include<cmath>
#include<string>
#include<stack>
#include<vector>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/matrix_access.hpp>


#define __USE_BSD	//to get usleep
#include <unistd.h>

#include"scene.hpp"

using namespace std;

namespace Scene
{

/* SCREEN SIZES */
unsigned int size_w;
unsigned int size_h;

int view_min_x;
int view_max_x;
int view_min_y;
int view_max_y;
/* SCREEN SIZES */

/**
 * Local Buffers
 */
uint8_t*        frame_buffer;

float*          objects_buffer_start; //data items MUST be aligned! max(type T) = 4byte
float*          objects_buffer_i;
float*          objects_buffer_end;

/**
 * Matrix stack for model matrix
 */
stack<glm::mat4> model_s;

/**
 * Pop a model matrix
 */
void pop_matrix(void)
{
    model_s.pop();
}

/**
 * Overwrites the current active top-stack matrix
 */
void put_matrix(glm::mat4& mat)
{
    model_s.top() = mat;
}

/**
 * Push a matrix, copy the old one to the current one
 * If there was no matrix on the stack, then set it to I
 */
void push_matrix(void)
{
    if(model_s.size() == 0)
      model_s.push(glm::mat4(1.0));
    else
    {
      glm::mat4 last = model_s.top();
      model_s.push(last);
    }
}

void load_identity(void) // TODO find a better implementation
{
  glm::mat4 mat(1.0);
  put_matrix(mat);
}

/**
 * Returns a copy of the current active top-stack matrix
 */
__attribute__((pure)) glm::mat4 get_matrix(void)
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

void printInfo(void)
{
  cout << "OpenCGL info - current state:" << endl;
  cout << "Size: w=" << size_w << " h=" << size_h << endl;
  cout << "Viewport: x=" << view_min_x << "->" << view_max_x << " ";
  cout << "y=" << view_min_y << "->" << view_max_y << endl;
  cout << endl;
}

void init(unsigned int w, unsigned int h)
{
    size_w = w;
    size_h = h;

    view_min_x = -((int)size_w/2);
    view_max_x = size_w/2;
    view_min_y = -((int)size_h/2);
    view_max_y = size_h/2;

    frame_buffer  = (uint8_t*)        malloc(size_h*size_w*sizeof(uint8_t));
    objects_buffer_start = (float*)   malloc(MAX_PRIMITIVES*16*FLOAT_SIZE);
    objects_buffer_i = objects_buffer_start;
    objects_buffer_end = objects_buffer_start + MAX_PRIMITIVES*16*FLOAT_SIZE;

    push_matrix();
}

void clearBuffers(void)
{
  objects_buffer_i = objects_buffer_start;
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

void rotatef(float angle, float x, float y, float z)
{
    rotatev(angle, glm::vec3(x,y,z));
}

void rotatev(float angle, glm::vec3 rotv)
{
  const glm::mat4 rotate = glm::rotate(model_s.top(), angle, rotv);
  model_s.top() = rotate;
}

void translatev(glm::vec3 dirv)
{
  const glm::mat4 translate = glm::translate(model_s.top(), dirv);
  model_s.top() = translate;
}

void translatef(float x, float y, float z)
{
    translatev(glm::vec3(x,y,z));
}

glm::vec3 performModelTransform(glm::vec3 a)
{
    glm::vec4 withW(a, 1.0f);
    glm::vec4 res = model_s.top() * withW;
    return glm::vec3(res);
}

//---------------------------------------

void push_vec3(glm::vec3& v)
{
  objects_buffer_i[0] = v.x;
  objects_buffer_i[1] = v.y;
  objects_buffer_i[2] = v.z;
  objects_buffer_i += 3;
}

// Pushes the object to the objects_buffer at byte index freeOffset_r
void push_header
(
  uint8_t type,
  uint8_t material,
  glm::vec3 passive,
  glm::vec3 active
)
{
    uint8_t* thisObj_u = (uint8_t*)objects_buffer_i;
    thisObj_u[0] = type;
    thisObj_u[1] = material;
    objects_buffer_i++;
    push_vec3(passive);
    push_vec3(active);
}

void triangle
(
  uint8_t material,
  glm::vec3 passive,
  glm::vec3 active,
  glm::vec3 a,
  glm::vec3 b,
  glm::vec3 c
)
{
  push_header(TRIANGLE, material, passive, active);
  push_vec3(a);
  push_vec3(b);
  push_vec3(c);
}

void sphere
(
  uint8_t material,
  glm::vec3 passive,
  glm::vec3 active,
  glm::vec3 pos,
  float radius
)
{
  push_header(TRIANGLE, material, passive, active);
  push_vec3(pos);
  *objects_buffer_i = radius;
  objects_buffer_i++;
}

void quad
(
  uint8_t material,
  glm::vec3 passive,
  glm::vec3 active,
  glm::vec3 a,
  glm::vec3 b,
  glm::vec3 c,
  glm::vec3 d
)
{
  push_header(TRIANGLE, material, passive, active);
  push_vec3(a);
  push_vec3(b);
  push_vec3(c);
  push_header(TRIANGLE, material, passive, active);
  push_vec3(a);
  push_vec3(c);
  push_vec3(d);
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
}
