
#include <iostream>
#include <vector>
#include <cstring>

#include "cl_helper.hpp"

using namespace std;
using namespace OpenCL;

namespace CLHelper
{
/**
 * Local buffer
 */
float* octree_buffer;

/**
 * The OpenCL environment.
 */
Environment environment;

/**
 * Buffers on GPU
 */
RemoteBuffer /*float */ fov_mem;
RemoteBuffer /*float */ objects_mem;
RemoteBuffer /*float */ octree_mem;
RemoteBuffer /*char4 */ frame_c_mem;
RemoteBuffer /*float4*/ frame_f_mem;
RemoteBuffer /*float */ samples_mem;
RemoteBuffer /*PRNG  */ prng_mem;

cl::CommandQueue queue;

unsigned int size_w;
unsigned int size_h;

void init(unsigned int const w,
          unsigned int const h,
          Kernel& tracer,
          size_t const primitive_size,
          size_t const octree_size)
{
  cout << "[CLHelper] Initializing." << endl;

  environment = Environment();

  size_w = w;
  size_h = h;

  octree_buffer = new float[primitive_size];

  // Init prng
  unsigned long prng[17];
  for(int i = 0; i < 16; i++)
  {
    prng[i] = (unsigned long)rand() << 32 | (unsigned long)rand();
  }
  prng[16] = 0;

  fov_mem = environment.allocate_space(16 * sizeof(float), nullptr);
  objects_mem =
      environment.allocate_space(primitive_size * sizeof(float), nullptr);
  octree_mem = environment.allocate_space(octree_size * sizeof(float), nullptr);
  frame_c_mem =
      environment.allocate_space(size_h * size_w * sizeof(uint32_t), nullptr);
  frame_f_mem =
      environment.allocate_space(size_h * size_w * 4 * sizeof(float), nullptr);
  samples_mem = environment.allocate_space(sizeof(float), nullptr);
  prng_mem = environment.allocate_space(17 * sizeof(unsigned long), prng);

  queue = environment.createCommandQueue();

  tracer.make(environment);

  tracer.set_argument(0, fov_mem);
  tracer.set_argument(1, objects_mem);
  tracer.set_argument(2, octree_mem);
  tracer.set_argument(3, frame_c_mem);
  tracer.set_argument(4, frame_f_mem);
  tracer.set_argument(5, samples_mem);
  tracer.set_argument(6, prng_mem);

  cout << "[CLHelper] Done." << endl;
}

void close(void)
{
  cout << "[CLHelper] Closing." << endl;
  // release buffers or something
  if(octree_buffer != nullptr)
    delete[] octree_buffer;
  cout << "[CLHelper] Closed." << endl;
}

void push_scene(Scene const& scene)
{
  cout << "[CLHelper] Pushing scene definition." << endl;

  size_t size = scene.objects_byte_size();
  (void)size;
  // TODO fix writeBuffer, so that is doesn't copy EVERYTHING
  float const* data = scene.get_objects();
  writeBufferBlocking(queue, objects_mem, data);

  Octree const* octree = scene.get_octree();
  size = octree->print_to_array(octree_buffer);
  cout << "[CLHelper] Octree has size " << size << endl;
  // TODO size
  writeBufferBlocking(queue, octree_mem, octree_buffer);
}

void render(Kernel const& tracer)
{
  tracer.enqueue(size_w, size_h, queue);
  queue.finish();
}

void write_buffer(RemoteBuffer const& buffer, void* data)
{
  writeBufferBlocking(queue, buffer, data);
}

void read_buffer(RemoteBuffer const& buffer, void* data)
{
  readBufferBlocking(queue, buffer, data);
}
}
