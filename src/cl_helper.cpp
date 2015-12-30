
#include<iostream>
#include<vector>
#include"cl_helper.hpp"
#include"scene.hpp"

using namespace std;

namespace CLHelper
{
  /**
   * Buffers on GPU
   */
  cl_mem /*float */ fov_mem;
  cl_mem /*float */ objects_mem;
  cl_mem /*char4 */ frame_c_mem;
  cl_mem /*float4*/ frame_f_mem;
  cl_mem /*float */ samples_mem;
  cl_mem /*PRNG  */ prng_mem;

  cl_command_queue queue;
  cl_kernel raytracer;
  cl_kernel bufferCleaner;

  unsigned int size_w;
  unsigned int size_h;

  void clearBuffers(OpenCL& cl)
  {
    cl.enqueue_kernel(size_w*size_h, bufferCleaner, queue);
  }

  void init(unsigned int w, unsigned int h, string& tracepath, string& cleanpath, OpenCL& cl)
  {
    cout << "[CLHelper] Initializing." << endl;

    size_w = w;
    size_h = h;

    frame_c_mem = cl.allocate_space(size_h*size_w*sizeof(uint32_t), nullptr);
    frame_f_mem = cl.allocate_space(size_h*size_w*4*FLOAT_SIZE, nullptr);
    fov_mem = cl.allocate_space(16*FLOAT_SIZE, nullptr);
    objects_mem = cl.allocate_space(MAX_PRIMITIVES*14*FLOAT_SIZE, nullptr);
    samples_mem = cl.allocate_space(FLOAT_SIZE, nullptr);
    prng_mem = cl.allocate_space(17*ULONG_SIZE, nullptr);

    queue = cl.createCommandQueue();

    string clearBufferMain("clearBuffers");
    bufferCleaner = cl.load_program(cleanpath, clearBufferMain);
    string tracerMain("trace");
    raytracer = cl.load_program(tracepath, tracerMain);

    OpenCL::setArgument(bufferCleaner, 0, MEM_SIZE, &frame_c_mem);
    OpenCL::setArgument(bufferCleaner, 1, MEM_SIZE, &frame_f_mem);

    OpenCL::setArgument(raytracer, 0, MEM_SIZE, &fov_mem);
    OpenCL::setArgument(raytracer, 1, MEM_SIZE, &objects_mem);
    OpenCL::setArgument(raytracer, 2, MEM_SIZE, &frame_c_mem);
    OpenCL::setArgument(raytracer, 3, MEM_SIZE, &frame_f_mem);
    OpenCL::setArgument(raytracer, 4, MEM_SIZE, &samples_mem);
    OpenCL::setArgument(raytracer, 5, MEM_SIZE, &prng_mem);

    // Init prng
    unsigned long prng[17];
    for (auto i = 0; i < 16; i++)
      prng[i] = (unsigned long)rand() << 32 | (unsigned long)rand();
    prng[16] = 0;
    OpenCL::writeBufferBlocking(queue, prng_mem, 17*ULONG_SIZE, prng);

    clearBuffers(cl);
    cout << "[CLHelper] Done." << endl;
  }

  void close(void)
  {
    cout << "[CLHelper] Closing." << endl;
    clReleaseKernel(bufferCleaner);
    clReleaseKernel(raytracer);

    clReleaseCommandQueue(queue);
    cout << "[CLHelper] Closed." << endl;
  }

  void pushScene(void)
  {
    cout << "[CLHelper] Pushing scene definition." << endl;

    size_t size = Scene::objects_buffer_i - Scene::objects_buffer_start;
    size = (size+1) * FLOAT_SIZE;
    *((uint8_t*)Scene::objects_buffer_i) = END;

    OpenCL::writeBufferBlocking(queue, objects_mem, size, Scene::objects_buffer_start);
  }

  void render(void)
  {
      cout << "[CLHelper] Rendering." << endl;
      OpenCL::enqueue_kernel(size_w, size_h, raytracer, queue);
      clFinish(queue);
  }
}
