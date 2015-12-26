#include<iostream>
#include<cstdlib>
#include<cstring>
#include<cmath>
#include<string>
#include<stack>
#include<vector>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_access.hpp>

#define __USE_BSD	//to get usleep
#include <unistd.h>

#include"raycg.hpp"
#include"cl_ocgl.hpp"

using namespace std;

#define MAX_PRIMITIVES 1000

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
uint8_t*        objects_buffer;
unsigned int*   offsets_buffer;
bool            buffer_full;

unsigned int  queuePos_r;
unsigned int  freeOffset_r;

/**
 * Buffers on GPU
 */
cl_mem /*float*/ fov_mem;
cl_mem /*char */ objects_mem;
cl_mem /*int  */ offsets_mem;
cl_mem /*char */ frameBuf_mem;
cl_mem /*float*/ depthBuf_mem;

/**
 * Matrix stack for model matrix
 */
stack<glm::mat4> model_s;

void clearBuffers(void)
{
  //for(int i=0; i<size_y*size_x; i++)
  //    frameBuffer[i] = ' '; i don't need to clear framebuf, cleared on rewrite
  for(int i=0; i<1000; i++)
  offsets_buffer[i] = -1;
}

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
glm::mat4 get_matrix(void)
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
struct interleave_
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

cl_command_queue framebufferqueue;

cl_kernel raytracer;
cl_kernel bufferCleaner;

void clearRemoteBuffers(OpenCL& cl)
{
    cl.setArgument(bufferCleaner, 0, MEM_SIZE, &offsets_mem);
    cl.setArgument(bufferCleaner, 1, MEM_SIZE, &frameBuf_mem);
    cl.setArgument(bufferCleaner, 2, MEM_SIZE, &depthBuf_mem);

    cl.enqueue_kernel(size_w*size_h, bufferCleaner, framebufferqueue);
}

void setup(unsigned int w, unsigned int h, string& tracepath, OpenCL& cl)
{
    size_w = w;
    size_h = h;

    view_min_x = -((int)size_w/2);
    view_max_x = size_w/2;
    view_min_y = -((int)size_h/2);
    view_max_y = size_h/2;

    frame_buffer  = (uint8_t*)        malloc(size_h*size_w*sizeof(uint8_t));
    objects_buffer = (uint8_t*)       malloc(MAX_PRIMITIVES*20*FLOAT_SIZE);
    offsets_buffer = (unsigned int*)  malloc(MAX_PRIMITIVES*INT_SIZE);

    queuePos_r = 0;
    freeOffset_r = 0;

    frameBuf_mem = cl.allocate_space(size_h*size_w*(int)sizeof(uint8_t), nullptr);
    fov_mem = cl.allocate_space(16*FLOAT_SIZE, nullptr);
    objects_mem = cl.allocate_space(MAX_PRIMITIVES*20*FLOAT_SIZE, nullptr);
    offsets_mem = cl.allocate_space(MAX_PRIMITIVES*INT_SIZE, nullptr);
    depthBuf_mem = cl.allocate_space(size_w*size_h*FLOAT_SIZE, nullptr);

    framebufferqueue = cl.createCommandQueue();

    string clearBufferKernel("./cl/clearBuffers.cl");
    string clearBufferMain("clearBuffers");
    bufferCleaner = cl.load_program(clearBufferKernel, clearBufferMain);
    string tracerMain("trace");
    raytracer = cl.load_program(tracepath, tracerMain);

    push_matrix();
    clearBuffers();
    clearRemoteBuffers(cl);
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
    dataCpy[1] = (float)size_w/(float)size_h;
    memcpy(dataCpy+2, camPos, 3*FLOAT_SIZE); //pos
    glm::vec3 dir(camPos[3], camPos[4], camPos[5]);
    glm::vec3 up(camPos[6], camPos[7], camPos[8]);
    glm::vec3 left = glm::cross(up, dir);
    up = cross(dir, left);
    normalize(dir);
    normalize(up);
    normalize(left);

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
    data_i[0] = size_w;
    data_i[1] = size_h;
    OpenCL::writeBufferBlocking(framebufferqueue, fov_mem, 14*FLOAT_SIZE+2*INT_SIZE, dataCpy);
}

void flushRTBuffer(OpenCL& cl)
{
    cout << "Flushing object buffer..." << endl;
    OpenCL::writeBufferBlocking(framebufferqueue,
      objects_mem, MAX_PRIMITIVES*20*FLOAT_SIZE, objects_buffer);
    OpenCL::writeBufferBlocking(framebufferqueue,
      offsets_mem, MAX_PRIMITIVES*INT_SIZE, offsets_buffer);

    OpenCL::setArgument(raytracer, 1, MEM_SIZE, &fov_mem);
    OpenCL::setArgument(raytracer, 2, MEM_SIZE, &objects_mem);
    OpenCL::setArgument(raytracer, 3, MEM_SIZE, &offsets_mem);
    OpenCL::setArgument(raytracer, 4, MEM_SIZE, &frameBuf_mem);
    OpenCL::setArgument(raytracer, 5, MEM_SIZE, &depthBuf_mem);
    OpenCL::setArgument(raytracer, 6, MEM_SIZE, 0);

    cl_event* eventList = new cl_event[size_h];
    cl_mem id_mem;
    int idi;

    for(unsigned int i=0; i<size_h; i++)
    {
        idi = i*size_w;
        id_mem = cl.allocate_space(INT_SIZE, &idi);
        OpenCL::setArgument(raytracer, 0, MEM_SIZE, &id_mem);
        eventList[i] = OpenCL::enqueue_kernel(size_w, raytracer, framebufferqueue);
    }

    cl_int status;
    unsigned int queue_size = size_h;
    unsigned int total = size_h;
    unsigned int finished = 0;
    unsigned int running = 0;
    unsigned int queued = 0;

    while(finished != total)
    {
        running = 0;
        queued = 0;

        for(unsigned int i=0; i<queue_size; i++)
        {
            status = OpenCL::getEventStatus(eventList[i]);
            switch(status)
            {
            //case CL_QUEUED:
            //case CL_SUBMITTED:
                break;
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
                break;
            }
        }

        cerr << "\x1b[1A\r";
        cerr << "                              \r"
             << "Queued:   " << queued << " / " << size_h << endl;
        //printf("                         \r %d / %d running...\n", running, size_y);
        cerr << "                              \r"
             << "Finished: " << finished << " / " << size_h << " ...";

        usleep(10000);
    }
    cout << endl;

    delete[] eventList;

    clearBuffers(); //object-queue
    queuePos_r = 0;
    freeOffset_r = 0;
}

#include"pngwrapper.hpp"

void printScreen(string& fpath, OpenCL& cl)
{
    flushRTBuffer(cl);
    cout << "Reading buffer" << endl;
    OpenCL::readBufferBlocking(framebufferqueue,
      frameBuf_mem, size_w*size_h*sizeof(uint8_t), frame_buffer);
    cout << "Saving to file" << endl;
    saveBWToFile(frame_buffer, size_w, size_h, fpath);
    cout << "Done" << endl;
}

void rotatef(float angle, float x, float y, float z)
{
    float len = sqrt(x*x+y*y+z*z);
    x /= len;
    y /= len;
    z /= len;

    glm::mat4& active_m = model_s.top();
    glm::mat4 rotm;
    float ca = (float)cos(angle);
    float sa = (float)sin(angle);

    glm::row(rotm, 0, glm::vec4(
        x*x*(1-ca)+ca,
        x*y*(1-ca)-z*sa,
        x*z*(1-ca)+y*sa,
        0.0f));
    glm::row(rotm, 1, glm::vec4(
        y*x*(1-ca)+z*sa,
        y*y*(1-ca)+ca,
        y*z*(1-ca)-x*sa,
        0.0f));
    glm::row(rotm, 2, glm::vec4(
        z*x*(1-ca)-y*sa,
        z*y*(1-ca)+x*sa,
        z*z*(1-ca)+ca,
        0.0f));
    glm::row(rotm, 3, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

    active_m *= rotm;
}

void rotatev(float angle, glm::vec3 rotv)
{
  rotatef(angle, rotv.x, rotv.y, rotv.z);
}

void translatef(float x, float y, float z)
{
    glm::mat4& active_m = model_s.top();
    glm::mat4 transm;
    glm::row(transm, 0, glm::vec4(1.0f, 0.0f, 0.0f, x));
    glm::row(transm, 1, glm::vec4(0.0f, 1.0f, 0.0f, y));
    glm::row(transm, 2, glm::vec4(0.0f, 0.0f, 1.0f, z));
    glm::row(transm, 3, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

    active_m *= transm;
}

void translatev(glm::vec3 dirv)
{
    translatef(dirv.x, dirv.y, dirv.z);
}

glm::vec3 performModelTransform(glm::vec3 a)
{
    glm::vec4 withW(a, 1.0f);
    glm::vec4 res = model_s.top() * withW;
    return glm::vec3(res);
}

//---------------------------------------

vector<glm::vec3> c_polygon;

struct
{
    glm::vec3 pos;
    float r;
} c_sphere;

struct
{
    uint8_t type;
    uint8_t material;
    uint8_t color;
} c_object;

size_t objectByteSize(void)
{
    size_t size = 0;
    size += 3*sizeof(uint8_t);
    switch(c_object.type)
    {
    case POLYGON:
        size += sizeof(uint8_t);
        size += c_polygon.size() * 3*FLOAT_SIZE;
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

/**
 * Copies the polygon to the queue, moving the queue* to the next free byte
 */
uint8_t* push_polygon(uint8_t* queue_u)
{
    *queue_u = (uint8_t)c_polygon.size();
    queue_u++;
    float* queue_f = (float*)queue_u;

    for(auto i=c_polygon.begin(); i!=c_polygon.end(); i++)
    {
        queue_f[0] = i->x;
        queue_f[1] = i->y;
        queue_f[2] = i->z;
        queue_f += 3;
    }
    return (uint8_t*)queue_f;
}

/**
 * Copies the sphere to the queue, moving the queue* to the next free byte
 */
uint8_t* push_sphere(uint8_t* queue_u)
{
    float* queue_f = (float*)queue_u;
    queue_f[0] = c_sphere.pos.x;
    queue_f[1] = c_sphere.pos.y;
    queue_f[2] = c_sphere.pos.z;
    queue_f[3] = c_sphere.r;
    return (uint8_t*)(queue_f+4);
}

// Pushes the object to the objects_buffer at byte index freeOffset_r
void push_object(void)
{
    offsets_buffer[queuePos_r] = freeOffset_r;
    uint8_t* thisObj_u = objects_buffer+freeOffset_r;
    thisObj_u[0] = c_object.type;
    thisObj_u[1] = c_object.material;
    thisObj_u[2] = c_object.color;
    thisObj_u += 3;

    switch(c_object.type)
    {
    case POLYGON:
        thisObj_u = push_polygon(thisObj_u);
        break;
    case SPHERE:
        thisObj_u = push_sphere(thisObj_u);
        break;
    default:
        return;
    }

    freeOffset_r = (unsigned int)(thisObj_u - objects_buffer);
    queuePos_r++;
}

uint8_t c_renderMode;
uint8_t c_color;
uint8_t c_material;

void begin(uint8_t mode)
{
    c_renderMode = mode;

    c_object.type = mode;
    c_object.material = c_material;
    c_object.color = c_color;
}

void colori(uint8_t c)
{
    c_color = c;
}

void materiali(uint8_t c)
{
    c_material = c;
}

void vertexv(glm::vec3 vertex)
{
    switch(c_renderMode)
    {
    case POLYGON:
        c_polygon.push_back(performModelTransform(vertex));
        break;
    case SPHERE:
        c_sphere.pos = performModelTransform(vertex);
        break;
    default:
        break;
    }
}

void floatf(float f)
{
  c_sphere.r = f;
}

void vertexf(float x, float y, float z)
{
    vertexv(glm::vec3(x, y, z));
}

bool end(void)
{
    size_t size = objectByteSize();

    bool max_index_reached = queuePos_r >= MAX_PRIMITIVES;
    bool no_space_left = size+freeOffset_r >= MAX_PRIMITIVES*20*FLOAT_SIZE;

    if(no_space_left || max_index_reached)
    {
        buffer_full = true;
        return false;
    }

    push_object();
    return true;
}
