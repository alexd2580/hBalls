
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
  OpenCL::RemoteBuffer /*float */ fov_mem;
  OpenCL::RemoteBuffer /*float */ objects_mem;
  OpenCL::RemoteBuffer /*char4 */ frame_c_mem;
  OpenCL::RemoteBuffer /*float4*/ frame_f_mem;
  OpenCL::RemoteBuffer /*float */ samples_mem;
  OpenCL::RemoteBuffer /*PRNG  */ prng_mem;

  cl_command_queue queue;
  OpenCL::Kernel* path_tracer;
  OpenCL::Kernel* buffer_cleaner;

  unsigned int size_w;
  unsigned int size_h;

  void clearBuffers(void)
  {
    buffer_cleaner->enqueue_kernel(size_w*size_h, queue);
  }

  void init(unsigned int w, unsigned int h, string& tracepath, string& cleanpath, OpenCL::Environment& cl)
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
    string tracerMain("trace");

    buffer_cleaner = new OpenCL::Kernel(cleanpath, clearBufferMain);
    path_tracer = new OpenCL::Kernel(tracepath, tracerMain);

    buffer_cleaner->make(cl);
    path_tracer->make(cl);

    buffer_cleaner->set_argument(0, frame_c_mem);
    buffer_cleaner->set_argument(1, frame_f_mem);

    path_tracer->set_argument(0, fov_mem);
    path_tracer->set_argument(1, objects_mem);
    path_tracer->set_argument(2, frame_c_mem);
    path_tracer->set_argument(3, frame_f_mem);
    path_tracer->set_argument(4, samples_mem);
    path_tracer->set_argument(5, prng_mem);

    // Init prng
    unsigned long prng[17];
    for (auto i = 0; i < 16; i++)
      prng[i] = (unsigned long)rand() << 32 | (unsigned long)rand();
    prng[16] = 0;
    OpenCL::writeBufferBlocking(queue, prng_mem, prng);

    clearBuffers();
    cout << "[CLHelper] Done." << endl;
  }

  void close(void)
  {
    cout << "[CLHelper] Closing." << endl;
    delete buffer_cleaner;
    delete path_tracer;

    clReleaseCommandQueue(queue);
    cout << "[CLHelper] Closed." << endl;
  }

  void pushScene(void)
  {
    cout << "[CLHelper] Pushing scene definition." << endl;

    size_t size = Scene::objects_buffer_i - Scene::objects_buffer_start;
    size = (size+1) * FLOAT_SIZE;
    *((uint8_t*)Scene::objects_buffer_i) = END;

    OpenCL::writeBufferBlocking(queue, objects_mem, Scene::objects_buffer_start);
  }

  void render(void)
  {
      path_tracer->enqueue_kernel(size_w, size_h, queue);
      clFinish(queue);
  }
}
