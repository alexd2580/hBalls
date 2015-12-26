#ifndef __RAYCG__H__
#define __RAYCG__H__

#include<glm/glm.hpp>
#include<cstdint>
#include<string>

#include"cl_ocgl.hpp"

#define UINT8_T(x) ((uint8_t)x)

#define POLYGON UINT8_T(1)
#define SPHERE UINT8_T(2)

#define SIZE_OF(x) ((size_t)sizeof(x))

#define CHAR_SIZE SIZE_OF(char)
#define INT_SIZE SIZE_OF(int)
#define FLOAT_SIZE SIZE_OF(float)
#define MEM_SIZE SIZE_OF(cl_mem)

//Surface types
#define MATT 1
#define METALLIC 2
#define MIRROR 3
#define GLASS 4

void setup(unsigned int, unsigned int, std::string&, OpenCL& cl);

void pop_matrix(void);
void push_matrix(void);
void load_identity(void);

glm::mat4 get_matrix(void);
void put_matrix(glm::mat4& mat);

void printInfo(void);

//----------------------

/*
fov should be an array of floats:
fovy, aspect,
posx, posy, posz,
dirx, diry, dirz,
upx, upy, upz,
size_x, size_y <-- IT'S TWO INTS!!
*/
void setPerspective(float fov, float* camPos);

void clearBuffers(void);
void clearRemoteBuffers(OpenCL&);
void printScreen(std::string& fpath, OpenCL& cl);

void rotatev(float angle, glm::vec3 rotv);
void rotatef(float angle, float x, float y, float z);
void translatev(glm::vec3 dirv);
void translatef(float x, float y, float z);

//Scene description
void begin(uint8_t mode);

void vertexv(glm::vec3 posv);
void vertexf(float x, float y, float z);
void floatf(float f); // for the radius

void colori(uint8_t c);
void materiali(uint8_t c);

bool end(void);

#endif