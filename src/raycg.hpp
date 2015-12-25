#ifndef __RAYCG__H__
#define __RAYCG__H__

#include"data.h"
#include<stdint.h>

#define POLYGON 1
#define SPHERE 2

#define CHAR_SIZE ((int)sizeof(char))
#define INT_SIZE ((int)sizeof(int))
#define FLOAT_SIZE ((int)sizeof(float))
#define MEM_SIZE ((int)sizeof(cl_mem))

//Surface types
#define MATT 1
#define METALLIC 2
#define MIRROR 3
#define GLASS 4

void setup(int, int, const char*);
void teardown(void);

void popMatrix(void);
void pushMatrix(void);
void loadIdentity(void);

mat4 getMatrix(void);
void putMatrix(mat4 mat);

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
void clearRemoteBuffers(void);
void printScreen(const char* fpath);

void rotatev(float angle, vec3 rotv);
void rotatef(float angle, float x, float y, float z);
void translatev(vec3 dirv);
void translatef(float x, float y, float z);

void begin(uint8_t mode);
void vertexv(vec3 posv);
void vertexf(float x, float y, float z);

void colori(uint8_t c);
void materiali(uint8_t c);
void end(void);

#endif

