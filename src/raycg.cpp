#include"raycg.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>

#include<CL/cl.h>
#include"cl_ocgl.h"

#define __USE_BSD	//to get usleep
#include <unistd.h>


/* SCREEN SIZES */
int size_x;     //
int size_y;     //
                //
int view_min_x; //
int view_max_x; //
int view_min_y; //
int view_max_y; //
/* SCREEN SIZES */

/**
 * The main local buffer
 * frameBuffer -> a buffer for the next frame colors
 */
uint8_t* frameBuffer;
uint8_t* objectsBuffer;
int* offsetsBuffer;

void clearBuffers(void)
{
    //for(int i=0; i<size_y*size_x; i++)
    //    frameBuffer[i] = ' '; i don't need to clear framebuf, cleared on rewrite
    for(int i=0; i<1000; i++)
        offsetsBuffer[i] = -1;
}

/**
 * Matrix4_stack
 */
typedef struct mat4_s_ mat4_s;
struct mat4_s_
{
    mat4_s* next;
    mat4 top;
};

/**
 * Matrix stack for model matrix
 */
mat4_s* model_s;

/**
 * Pop a model matrix
 */
void popMatrix(void)
{
    mat4_s* last = model_s;
    model_s = model_s->next;
    free(last);
}

/**
 * Push a matrix, copy the old one to the current one
 * If there was no matrix on the stack, then set it to I
 */
void pushMatrix(void)
{
    mat4_s* last = model_s;
    model_s = (mat4_s*)malloc(sizeof(mat4_s));
    model_s->next = last;

    if(model_s->next == NULL)
        setIdentity4(&(model_s->top));
    else
        model_s->top = model_s->next->top;
}

void loadIdentity(void)
{
    setIdentity4(&(model_s->top));
}

/**
 * Returnc a copy of the current active top-stack matrix
 */
mat4 getMatrix(void)
{
    return model_s->top;
}

/**
 * Overwrites the current active top-stack matrix
 */
void putMatrix(mat4 mat)
{
    model_s->top = mat;
}


cl_command_queue framebufferqueue;

cl_kernel raytracer;
cl_kernel bufferCleaner;

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
struct interleave_
{
    vec3 pos;
    float color;
};

/**
 * Buffers on GPU
 */

cl_mem /*float*/ fov_mem;
cl_mem /*char */ objects_mem;
cl_mem /*int  */ offsets_mem;
cl_mem /*char */ frameBuf_mem;
cl_mem /*float*/ depthBuf_mem;

void clearRemoteBuffers(void)
{
    setArgument(bufferCleaner, 0, MEM_SIZE, &offsets_mem);
    setArgument(bufferCleaner, 1, MEM_SIZE, &frameBuf_mem);
    setArgument(bufferCleaner, 2, MEM_SIZE, &depthBuf_mem);

    enqueueKernelInQueue(size_x*size_y, bufferCleaner, framebufferqueue);
}


void printInfo(void)
{
    printf("OpenCGL info - current state:\n");
    printf("Size: %dx%d\n", size_x, size_y);
    printf("Viewport: x:%d->%d, y:%d->%d\n", view_min_x, view_max_x, view_min_y, view_max_y);
    printf("\n");
}

void setup(int x, int y, const char* tracepath)
{
    size_x = x;
    size_y = y;

    view_min_x = -size_x/2;
    view_max_x = size_x/2;
    view_min_y = -size_y/2;
    view_max_y = size_y/2;
    
    frameBuffer = (uint8_t*)malloc((size_t)(size_y*size_x*(int)sizeof(uint8_t)));
    objectsBuffer = (uint8_t*)malloc(1000*20*FLOAT_SIZE);
    offsetsBuffer = (int*)malloc(1000*INT_SIZE);
    
    setupCL();
    
    framebufferqueue = createCommandQueue();

    fov_mem = allocateSpace(16*FLOAT_SIZE, NULL);
    objects_mem = allocateSpace(1000*20*FLOAT_SIZE, NULL);
    offsets_mem = allocateSpace(1000*INT_SIZE, NULL);
    frameBuf_mem = allocateSpace(size_y*size_x*(int)sizeof(uint8_t), NULL);
    depthBuf_mem = allocateSpace(size_x*size_y*FLOAT_SIZE, NULL);    

    bufferCleaner = loadProgram("./clearBuffers.cl", "clearBuffers");
    raytracer = loadProgram(tracepath, "trace");

    pushMatrix();
    clearBuffers();
    clearRemoteBuffers();
}

void setPerspective(float fov, float* camPos)
{
/*
fov should be an array of floats:
fovy, aspect, 
posx, posy, posz,
dirx, diry, dirz,
upx, upy, upz,
-- add leftx, lefty, leftz,
size_x, size_y <-- IT'S TWO INTS!!
*/
    float dataCpy[16];
    dataCpy[0] = fov;
    dataCpy[1] = (float)size_x/(float)size_y;
    memcpy(dataCpy+2, camPos, 3*FLOAT_SIZE); //pos 
    vec3 dir = (vec3){ camPos[3], camPos[4], camPos[5] };
    vec3 up = (vec3){ camPos[6], camPos[7], camPos[8] };
    vec3 left = crossProd(&up, &dir);
    up = crossProd(&dir, &left);
    normalize(&dir);
    normalize(&up);
    normalize(&left);
    
    dataCpy[5] = dir.x;
    dataCpy[6] = dir.y;
    dataCpy[7] = dir.z;
    dataCpy[8] = up.x;
    dataCpy[9] = up.y;
    dataCpy[10] = up.z;
    dataCpy[11] = left.x;
    dataCpy[12] = left.y;
    dataCpy[13] = left.z;
    
    int* data_i = (int*)(dataCpy+14);
    data_i[0] = size_x;
    data_i[1] = size_y;
    writeBufferBlocking(framebufferqueue, fov_mem, 14*FLOAT_SIZE+2*INT_SIZE, dataCpy);
}

void freeStack(mat4_s* s)
{
    mat4_s* t;
    while(s != NULL)
    {
        t = s;
        s = s->next;
        free(t);
    }
}

void teardown(void)
{
    closeCL();
    freeStack(model_s);
}

int queuePos_r = 0;
int freeOffset_r = 0;

void flushRTBuffer(void)
{
    printf("Flushing object buffer...\n");
    fflush(stdout);
    writeBufferBlocking(framebufferqueue, objects_mem, 1000*20*FLOAT_SIZE, objectsBuffer);
    writeBufferBlocking(framebufferqueue, offsets_mem, 1000*INT_SIZE, offsetsBuffer);

    setArgument(raytracer, 1, MEM_SIZE, &fov_mem);
    setArgument(raytracer, 2, MEM_SIZE, &objects_mem);
    setArgument(raytracer, 3, MEM_SIZE, &offsets_mem);
    setArgument(raytracer, 4, MEM_SIZE, &frameBuf_mem);
    setArgument(raytracer, 5, MEM_SIZE, &depthBuf_mem);
    setArgument(raytracer, 6, MEM_SIZE, 0);
    
    cl_event eventList[size_y];
    cl_mem id_mem;
    int idi;
    
    for(int i=0; i<size_y; i++)
    {
        idi = i*size_x;
        id_mem = allocateSpace(INT_SIZE, &idi);
        setArgument(raytracer, 0, MEM_SIZE, &id_mem);
        eventList[i] = enqueueKernelInQueue(size_x, raytracer, framebufferqueue);
    }
    
    cl_int status;
    int queue_size = size_y;
    int total = size_y;
    int finished = 0;
    int running = 0;
    int queued = 0;
    
    while(finished != total)
    {
        running = 0;
        queued = 0;
    
        for(int i=0; i<queue_size; i++)
        {
            status = getEventStatus(eventList[i]);
            switch(status)
            {
            case CL_RUNNING:
                running++;
                break;
            case CL_COMPLETE:
                finished++;
                queue_size--;
                eventList[i] = eventList[queue_size];
                i--;
                break;
            default:
                queued++;
                break;            
            }            
        }
            
        printf("\x1b[1A\r");
        printf("                         \r %d / %d queued...\n", queued, size_y);
        //printf("                         \r %d / %d running...\n", running, size_y);
        printf("                         \r %d / %d finished...", finished, size_y);
        fflush(stdout);
        
        usleep(10000);
    }
    putchar('\n');
    
    clearBuffers(); //object-queue
    queuePos_r = 0;
    freeOffset_r = 0;
}

#include"pngwrapper.h"

void printScreen(const char* fpath)
{
    flushRTBuffer();
    printf("Reading buffer\n");
    fflush(stdout);
    readBufferBlocking(framebufferqueue, frameBuf_mem, size_y*size_x*(int)sizeof(uint8_t), frameBuffer);
    printf("Saving to file\n");
    fflush(stdout);
    saveBWToFile(frameBuffer, size_x, size_y, fpath);
    printf("Done\n");
    fflush(stdout);
}

void rotatev(float a, vec3 rotv)
{
    normalize(&rotv);
    float x=rotv.x, y=rotv.y, z=rotv.z;
    mat4 active_m = model_s->top;
    mat4 rotm;
    float ca = (float)cos(a);
    float sa = (float)sin(a);
    
    rotm.x = (vec4){ 
        x*x*(1-ca)+ca, 
        x*y*(1-ca)-z*sa,
        x*z*(1-ca)+y*sa,
        0.0f };
    rotm.y = (vec4){
        y*x*(1-ca)+z*sa,
        y*y*(1-ca)+ca,
        y*z*(1-ca)-x*sa,
        0.0f };
    rotm.z = (vec4){
        z*x*(1-ca)-y*sa,
        z*y*(1-ca)+x*sa,
        z*z*(1-ca)+ca,
        0.0f };
    rotm.w = (vec4){ 0.0f, 0.0f, 0.0f, 1.0f };
    
    matMultMat4(&active_m, &rotm, &(model_s->top));
}

void translatev(vec3 dirv)
{
    translatef(dirv.x, dirv.y, dirv.z);
}

void rotatef(float angle, float x, float y, float z)
{
    rotatev(angle, (vec3){ x, y, z });
}

void translatef(float x, float y, float z)
{
    mat4 active_m = model_s->top;
    mat4 transm;
    transm.x = (vec4){ 1.0f, 0.0f, 0.0f, x };
    transm.y = (vec4){ 0.0f, 1.0f, 0.0f, y };
    transm.z = (vec4){ 0.0f, 0.0f, 1.0f, z };
    transm.w = (vec4){ 0.0f, 0.0f, 0.0f, 1.0f };
    
    matMultMat4(&active_m, &transm, &(model_s->top));
}

vec3 performModelTransform(vec3 a)
{
    vec4 withW = (vec4){ a.x, a.y, a.z, 1.0f };
    vec4 res;
    matMultVec4(&(model_s->top), &withW, &res);
    return *((vec3*)(&res));
}

//---------------------------------------

typedef struct vertex_t_ vertex_t;
struct vertex_t_
{
    vec3 pos;
    vertex_t* next;
};

typedef struct polygonN_ polygonN;
struct polygonN_
{
    uint8_t n;
    vertex_t* vertices;
};

typedef struct sphere_ sphere;
struct sphere_
{
    vec3 pos;
    float r;
};

typedef struct object_ object;
struct object_
{
    uint8_t type;
    uint8_t material;
    uint8_t color;
    void* obj;
};

/**
 * Copies the polygon to the queue, moving the queue*
 * While doing so, decomposes the internal linked list.
 */ 

uint8_t* addPolygon(void* queue_v, polygonN* pol)
{
    uint8_t* queue_u = queue_v;
    *queue_u = pol->n;
    queue_u++;
    float* queue_f = (float*)queue_u;
    vertex_t* tmp;
    
    for(int i=0; i<pol->n; i++)
    {
        tmp = pol->vertices;
        pol->vertices = pol->vertices->next;
        
        queue_f[0] = tmp->pos.x;
        queue_f[1] = tmp->pos.y;
        queue_f[2] = tmp->pos.z;
        queue_f += 3;
        
        free(tmp);
    }
    return (uint8_t*)queue_f;
}

int objectByteSize(object* o)
{
    int size = 0;
    size += 3*(int)sizeof(uint8_t);
    switch(o->type)
    {
    case POLYGON:
        size += (int)sizeof(uint8_t);
        polygonN* p = (polygonN*)o->obj;
        size += p->n * 3*FLOAT_SIZE;
        break;
    case SPHERE:
        size += 3*FLOAT_SIZE; //pos
        size += FLOAT_SIZE; //radius
        break;
    default:
        break;
    }
    return size;
}

void addToRayTraceQueue(object* data)
{
    offsetsBuffer[queuePos_r] = freeOffset_r;
    uint8_t* thisObj_u = objectsBuffer+freeOffset_r;
    thisObj_u[0] = data->type;    
    thisObj_u[1] = data->material;
    thisObj_u[2] = data->color;
    thisObj_u += 3;
    
    float* thisObj_f;
    sphere* s;
    
    switch(data->type)
    {
    case POLYGON:
        thisObj_u = addPolygon(thisObj_u, data->obj);
        freeOffset_r = (int)(thisObj_u - objectsBuffer);
        break;
    case SPHERE:
        thisObj_f = (float*)thisObj_u;
        s = data->obj;
        thisObj_f[0] = s->pos.x;
        thisObj_f[1] = s->pos.y;
        thisObj_f[2] = s->pos.z;
        thisObj_f[3] = s->r;
        thisObj_u = (uint8_t*)(thisObj_f+4);
        freeOffset_r = (int)(thisObj_u - objectsBuffer);
        break;
    default:
        return;
    }
    queuePos_r++;
}

uint8_t c_renderMode;
uint8_t c_color;
uint8_t c_material;

object c_object;
polygonN c_polygon;
sphere c_sphere;
vertex_t** c_newVertex;

void begin(uint8_t mode)
{
    c_renderMode = mode;
    
    c_object.type = mode;
    c_object.material = c_material;
    c_object.color = c_color;
    
    switch(mode)
    {
    case POLYGON:
        c_object.obj = &c_polygon;
        c_polygon.n = 0;
        c_polygon.vertices = NULL;
        c_newVertex = &(c_polygon.vertices);
        break;
    case SPHERE:
        c_object.obj = &c_sphere;
        c_sphere.pos.x = 1.0f/0.0f;
    default:
        break;
    }
}

void colori(uint8_t c)
{
    c_color = c;
}

void materiali(uint8_t c)
{
    c_material = c;
}

void vertexv(vec3 vertex)
{
    switch(c_renderMode)
    {
    case POLYGON:
        c_polygon.n++;
        *c_newVertex = (vertex_t*)malloc(sizeof(vertex_t));
        (*c_newVertex)->pos = performModelTransform(vertex);
        (*c_newVertex)->next = NULL;
        c_newVertex = &((*c_newVertex)->next);
        break;
    case SPHERE:
        if(c_sphere.pos.x >= c_sphere.pos.x+10.0f)
            c_sphere.pos = performModelTransform(vertex);
        else
            c_sphere.r = vertex.x;
        break;
    default:
        break;
    }
}

void vertexf(float x, float y, float z)
{
    vertexv((vec3){ x, y, z });
}

void end(void)
{
    int size = objectByteSize(&c_object);
    if(size+freeOffset_r >= 1000*20*FLOAT_SIZE)
        flushRTBuffer();
    if(queuePos_r >= 1000)
        flushRTBuffer();
        
    addToRayTraceQueue(&c_object);
}


