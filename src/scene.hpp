#ifndef __RAYCG__H__
#define __RAYCG__H__

#include<glm/glm.hpp>
#include<cstdint>

#define UINT8_T(x) ((uint8_t)x)

// used by opencl kernel
#define TRIANGLE UINT8_T(1)
#define SPHERE UINT8_T(2)

// local defines
#define TRIANGLE_FAN UINT8_T(3)
#define TRIANGLE_STRIP UINT8_T(4)
#define QUAD UINT8_T(5)

//Surface types
#define MATT 1
#define METALLIC 2
#define MIRROR 3
#define GLASS 4

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
  extern uint32_t*       objects_buffer; //data items MUST be aligned! max(type T) = 4byte
  extern int32_t*        offsets_buffer;
  extern bool            buffer_full;

  void init(unsigned int, unsigned int);
  void clearBuffers(void);

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
  void set_mode(uint8_t mode);

  void vertexv(glm::vec3 posv);
  void vertexf(float x, float y, float z);
  void spherev(glm::vec3 posv, float r);
  void spheref(float x, float y, float z, float r);

  void colori(uint8_t c);
  void materiali(uint8_t c);

}
#endif
