#ifndef __RAYCG__H__
#define __RAYCG__H__

#include<glm/glm.hpp>
#include<cstdint>

#define UINT8_T(x) ((uint8_t)x)

/** PRIMITIVE TYPE **/
// also used by opencl kernel
#define END             UINT8_T(0)
#define TRIANGLE        UINT8_T(1)
#define SPHERE          UINT8_T(2)

/** SURFACE TYPE **/
#define DIFFUSE   UINT8_T(1)
#define METALLIC  UINT8_T(2)
#define MIRROR    UINT8_T(3)
#define GLASS     UINT8_T(4)

/**
uint8 type;
uint8 material;
uin16 __padding__
float3 passive;
float3 active;

data...

sphere -> 11*4 byte
triangle -> 16*4 byte
**/

#define MAX_PRIMITIVES 1000

#define SIZE_OF(x) ((size_t)sizeof(x))

#define CHAR_SIZE SIZE_OF(char)
#define INT_SIZE SIZE_OF(int32_t)
#define FLOAT_SIZE SIZE_OF(float)

namespace Scene
{
  extern unsigned int size_w;
  extern unsigned int size_h;

  /**
   * Local Buffers
   */
  extern float*     objects_buffer_start; //data items MUST be aligned! max(type T) = 4byte
  extern float*     objects_buffer_i;
  extern bool       buffer_full;

  void init(unsigned int, unsigned int);
  void clearBuffers(void); // clears the scene

  void pop_matrix(void);
  void push_matrix(void);
  void load_identity(void);

  glm::mat4 get_matrix(void);
  void put_matrix(glm::mat4& mat);

  void printInfo(void);

  //----------------------

  void rotatev(float angle, glm::vec3 rotv);
  void rotatef(float angle, float x, float y, float z);
  void translatev(glm::vec3 dirv);
  void translatef(float x, float y, float z);

  //Scene description

  void triangle
  (
    uint8_t material,
    glm::vec3 passive,
    glm::vec3 active,
    glm::vec3 a,
    glm::vec3 b,
    glm::vec3 c
  );

  void sphere
  (
    uint8_t material,
    glm::vec3 passive,
    glm::vec3 active,
    glm::vec3 pos,
    float r
  );

  void quad
  (
    uint8_t material,
    glm::vec3 passive,
    glm::vec3 active,
    glm::vec3 a,
    glm::vec3 b,
    glm::vec3 c,
    glm::vec3 d
  );
}

#endif
