#include <iostream>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <string>

#define __USE_BSD // to get usleep
#include <unistd.h>

/*struct timespec abstime;
clock_gettime(CLOCK_REALTIME, &abstime);
abstime.tv_sec += cooldown;
pthread_cond_timedwait(&notifier, &mutex, &abstime);*/

#include "scene_helper.hpp"
#include "sdl.hpp"
#include "cl.hpp"

using namespace std;
using namespace OpenCL;

void push_camera(cl::CommandQueue const& queue,
                 Camera const& camera,
                 int const size_w,
                 int const size_h,
                 ObjectsBuffer const& obj,
                 RemoteBuffer const& data_mem)
{
  /**
  * Main kernel function.
  * general_data is an array of 19 4-byte units:
  * fovy, aspect :: Float
  * posx, posy, posz :: Float
  * dirx, diry, dirz :: Float
  * upx, upy, upz :: Float
  * leftx, lefty, leftz :: Float
  * size_w, size_h :: UInt
  * num_surfs :: UInt
  * num_lamps :: UInt
  * off_lamps :: UInt
   */
  float data[17];
  data[0] = camera.fov;
  data[1] = float(size_w) / float(size_h);
  data[2] = camera.pos.x;
  data[3] = camera.pos.y;
  data[4] = camera.pos.z;
  data[5] = camera.dir.x;
  data[6] = camera.dir.y;
  data[7] = camera.dir.z;
  data[8] = camera.up.x;
  data[9] = camera.up.y;
  data[10] = camera.up.z;
  data[11] = camera.left.x;
  data[12] = camera.left.y;
  data[13] = camera.left.z;

  int* data_i = (int*)(data + 14);
  data_i[0] = size_w;
  data_i[1] = size_h;
  data_i[2] = obj.surf_count;
  data_i[3] = obj.lamp_count;
  data_i[4] = obj.lamp_float_index;

  writeBufferBlocking(queue, data_mem, data);
}

void push_data(cl::CommandQueue const& queue,
               ObjectsBuffer const& obj,
               RemoteBuffer const& objects_mem)
{
  // TODO fix writeBuffer, so that is doesn't copy EVERYTHING

  writeBufferBlocking(queue, objects_mem, obj.buffer);
}

void create_scene(Scene& scene)
{
  cout << "[Main] Queueing models." << endl;

  Material const red(DIFFUSE, 1.0f, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
  Material const green(DIFFUSE, 1.0f, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
  Material const blue(DIFFUSE, 1.0f, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));

  Material const mirror(MIRROR, 0.0f, 0.0f, glm::vec3(1.0f));
  Material const metal(METALLIC, 0.2f, 0.0f, glm::vec3(1.0f));
  Material const lamp(DIFFUSE, 0.0f, 30.0f, glm::vec3(1.0f));

  Material const white(DIFFUSE, 1.0f, 0.0f, glm::vec3(1.0f));

  scene.push_matrix();
  scene.translate(1.5f, 1.5f, -1.5f);
  room(scene, 3.f, 3.f, 3.f, white);
  scene.pop_matrix();

  scene.push_matrix();
  scene.translate(2.8f, 2.8f, -1.5f);
  box(scene, 0.4f, 0.4f, 0.4f, lamp);
  scene.pop_matrix();

  scene.push_matrix();
  scene.translate(0.2f, 0.0f, -3.f + 0.85f);
  Table::render(scene);
  scene.pop_matrix();

  cout << "[Main] Done." << endl;
}

int main(void)
{
  cout << "[Main] Entry." << endl;

  unsigned int const size_w = 100;
  unsigned int const size_h = 100;

  uint32_t* frame_buffer = new uint32_t[size_w * size_h];
  size_t const max_primitives = 1000;
  float* primitive_buffer = new float[max_primitives * PRIM_SIZE];
  ObjectsBuffer obuf(primitive_buffer, max_primitives);

  if(frame_buffer == nullptr || primitive_buffer == nullptr)
  {
    cerr << "[Main] Coundn't allocate local buffers" << endl;
    return 1;
  }

  /** SDL **/
  if(SDL::init(size_w, size_h) != 0)
  {
    cerr << "[Main] SDL initialization failed." << endl;
    return 1;
  }

  try
  {
    /** OpenCL **/
    Environment env(1, CL_DEVICE_TYPE_ALL);

    /** Kernel **/
    string tracer("./cl/ray_frag.cl");
    string tracer_main("trace");
    OpenCL::Kernel path_tracer(tracer, tracer_main);

    /** Scene **/
    Scene scene(obuf);

    create_scene(scene);

    /**
     * PRNG
     * adapted from svenstaro's trac0r
     * 01.01.2016 => github.com/svenstaro/trac0r
     */
    unsigned long prng[17];
    for(int i = 0; i < 16; i++)
      prng[i] = (unsigned long)rand() << 32 | (unsigned long)rand();
    prng[16] = 0;

    unsigned int max_bounces = 3;
    unsigned int float_size = 4; // byte. as defined in specification
    unsigned int global_float_ptr_size =
        get_device_info_(env.m_devices[0], CL_DEVICE_ADDRESS_BITS, cl_uint) / 8;

    /**
    * Total size for bdpt_buffer in bytes.
    * float3 has to be aligned to float4 =/
    */
    size_t aux_bytes_per_pixel = global_float_ptr_size // size of a float ptr
                                 + 4 * float_size; // size of bounce position

    cout << "[Main] Size of aux pixel unit: " << aux_bytes_per_pixel << " byte"
         << endl;

    size_t bdpt_byte = max_bounces       // how many bounces
                       * size_w * size_h // per pixel (per id)
                       * 2               // from eye and from lamp
                       * aux_bytes_per_pixel;

    size_t bdpt_kib = bdpt_byte / 1024;
    size_t bdpt_printed = bdpt_kib >= 1024 ? bdpt_kib / 1024 : bdpt_kib;
    string unit = bdpt_kib >= 1024 ? " MiB" : " KiB";
    cout << "[Main] Size of aux buffer: " << bdpt_printed << unit << endl;

    /** Buffers **/
    RemoteBuffer /*float */ data_mem = env.allocate(19 * sizeof(float));
    RemoteBuffer /*float */ objects_mem =
        env.allocate(max_primitives * PRIM_SIZE * sizeof(float));
    RemoteBuffer /*float */ octree_mem = env.allocate(bdpt_byte);
    RemoteBuffer /*char4 */ frame_c_mem =
        env.allocate(size_h * size_w * sizeof(uint32_t));
    RemoteBuffer /*float4*/ frame_f_mem =
        env.allocate(size_h * size_w * 4 * sizeof(float));
    RemoteBuffer /*float */ samples_mem = env.allocate(sizeof(float));
    RemoteBuffer /*PRNG  */ prng_mem =
        env.allocate(17 * sizeof(unsigned long), prng);

    /** Prepare Kernel **/
    path_tracer.make(env);
    path_tracer.set_argument(0, data_mem);
    path_tracer.set_argument(1, objects_mem);
    path_tracer.set_argument(2, octree_mem);
    path_tracer.set_argument(3, frame_c_mem);
    path_tracer.set_argument(4, frame_f_mem);
    path_tracer.set_argument(5, samples_mem);
    path_tracer.set_argument(6, prng_mem);

    cout << "[Main] PathTracer compiled" << endl;

    /** Camera **/
    Camera c;
    c.pos = glm::vec3(-0.3f, 1.2f, 0.0f);
    c.dir = glm::vec3(0.5f, -0.2, -1);
    c.up = glm::vec3(0.0f, 1.0f, 0.0f);

    c.fov = glm::quarter_pi<float>();

    c.dir = glm::normalize(c.dir);
    c.up = glm::normalize(c.up);
    c.left = glm::cross(c.up, c.dir);
    c.up = glm::cross(c.dir, c.left);

    /** CommandQueue **/
    cl::CommandQueue queue = env.create_queue();

    /** Push data to remote buffers **/
    push_camera(queue, c, size_w, size_h, obuf, data_mem);
    push_data(queue, obuf, objects_mem);

    float samples = 0.0f;
    while(!SDL::die)
    {
      SDL::handleEvents();
      samples++;
      writeBufferBlocking(queue, samples_mem, &samples);
      /** RUN KERNEL **/
      path_tracer.enqueue(size_w, size_h, queue);

      queue.finish();
      /** Read result from char-framebuffer **/
      readBufferBlocking(queue, frame_c_mem, frame_buffer);
      /** Draw it **/
      SDL::drawFrame(frame_buffer);
      cout << "[Main] Samples: " << (int)samples << endl;
      cout.flush();
    }
  }
  catch(OpenCLException& e)
  {
    e.print();
  }

  delete[] frame_buffer;
  delete[] primitive_buffer;

  SDL::close();
  cout << "[Main] Exit." << endl;
  return 0;
}
