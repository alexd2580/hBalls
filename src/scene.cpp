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

#include"raycg.hpp"
#include"cl_ocgl.hpp"

using namespace std;

namespace Scene
{

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
uint32_t*       objects_buffer; //data items MUST be aligned! max(type T) = 4byte
int32_t*        offsets_buffer;
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
    objects_buffer = (uint32_t*)      malloc(MAX_PRIMITIVES*10*FLOAT_SIZE);
    offsets_buffer = (int32_t*)       malloc(MAX_PRIMITIVES*INT_SIZE);

    queuePos_r = 0;
    freeOffset_r = 0;

    frameBuf_mem = cl.allocate_space(size_h*size_w*sizeof(uint8_t), nullptr);
    fov_mem = cl.allocate_space(16*FLOAT_SIZE, nullptr);
    objects_mem = cl.allocate_space(MAX_PRIMITIVES*10*FLOAT_SIZE, nullptr);
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

void flushRTBuffer(OpenCL& cl)
{
    cout << "Flushing object buffer..." << endl;
    OpenCL::writeBufferBlocking(framebufferqueue,
      objects_mem, MAX_PRIMITIVES*10*FLOAT_SIZE, objects_buffer);

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

        cout << "                              \r"
             << "Queued:   " << queued << " / " << size_h << endl;
        //printf("                         \r %d / %d running...\n", running, size_y);
        cout << "                              \r"
             << "Finished: " << finished << " / " << size_h << " ...";
        cout << "\x1b[1A\r";
        cout.flush();
        usleep(10000);
    }
    cout << endl << endl;

    clFinish(framebufferqueue);

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
    cout.flush();
}

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

glm::vec3 c_triangle[3];

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

__attribute__((pure)) size_t objectByteSize(void)
{
    size_t size = 0;
    size += FLOAT_SIZE; //3*sizeof(uint8_t);
    switch(c_object.type)
    {
    case TRIANGLE:
        size += 3 * 3*FLOAT_SIZE;
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
float* push_triangle(float* queue_f)
{
    for(int i=0; i<3; i++)
    {
      queue_f[0] = c_triangle[i].x;
      queue_f[1] = c_triangle[i].y;
      queue_f[2] = c_triangle[i].z;
      queue_f += 3;
    }
    return queue_f;
}

/**
 * Copies the sphere to the queue, moving the queue* to the next free byte
 */
float* push_sphere(float* queue_f)
{
    queue_f[0] = c_sphere.pos.x;
    queue_f[1] = c_sphere.pos.y;
    queue_f[2] = c_sphere.pos.z;
    queue_f[3] = c_sphere.r;
    return queue_f+4;
}

// Pushes the object to the objects_buffer at byte index freeOffset_r
void push_object(void)
{
    offsets_buffer[queuePos_r] = freeOffset_r;
    uint8_t* thisObj_u = (uint8_t*)(objects_buffer+freeOffset_r);
    thisObj_u[0] = c_object.type;
    thisObj_u[1] = c_object.material;
    thisObj_u[2] = c_object.color;

    float* thisObj_f = (float*)(objects_buffer+freeOffset_r+1);

    switch(c_object.type)
    {
    case TRIANGLE:
        thisObj_f = push_triangle(thisObj_f);
        break;
    case SPHERE:
        thisObj_f = push_sphere(thisObj_f);
        break;
    default:
        return;
    }

    freeOffset_r = (int32_t)((uint32_t*)thisObj_f - objects_buffer);
    queuePos_r++;
}

uint8_t c_renderMode;
uint8_t c_vertex_count;

void set_mode(uint8_t mode)
{
    c_renderMode = mode;

    switch(mode)
    {
    case SPHERE:
      c_object.type = SPHERE;
      break;
    default:
      c_object.type = TRIANGLE;
      break;
    }

    c_vertex_count = 0;
}

void colori(uint8_t c)
{
    c_object.color = c;
}

void materiali(uint8_t c)
{
    c_object.material = c;
}

void vertexv(glm::vec3 vertex)
{
    switch(c_renderMode)
    {
    case TRIANGLE:
        c_triangle[c_vertex_count] = performModelTransform(vertex);
        c_vertex_count++;
        if(c_vertex_count == 3)
        {
          push_object();
          c_vertex_count = 0;
        }
        break;
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
    case QUAD:
        if(c_vertex_count == 3)
        {
          push_object();
          c_triangle[1] = c_triangle[2];
          c_triangle[2] = performModelTransform(vertex);
          push_object();
          c_vertex_count = 0;
        }
        else
        {
          c_triangle[c_vertex_count] = performModelTransform(vertex);
          c_vertex_count++;
        }
        break;
    case SPHERE:
        break;
    default:
        break;
    }
}

void vertexf(float x, float y, float z)
{
  vertexv(glm::vec3(x, y, z));
}

void spherev(glm::vec3 pos, float r)
{
  c_sphere.pos = pos;
  c_sphere.r = r;
  push_object();
}

void spheref(float x, float y, float z, float r)
{
  spherev(glm::vec3(x, y, z), r);
}

}
