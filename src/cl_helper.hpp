#ifndef __CL_HELPER_USW__
#define __CL_HELPER_USW__

#include <cstdint>
#include <CL/cl.h>

#include "cl.hpp"
#include "scene.hpp"

/**
 * This namepace holds the OpenCL environment, the remote buffers and the
 * command queue.
 */
namespace CLHelper
{
/**
 * Buffers on GPU
 */
extern OpenCL::RemoteBuffer /*float */ fov_mem;
extern OpenCL::RemoteBuffer /*float */ objects_mem;
extern OpenCL::RemoteBuffer /*float */ octree_mem;
extern OpenCL::RemoteBuffer /*char4 */ frame_c_mem;
extern OpenCL::RemoteBuffer /*float4*/ frame_f_mem;
extern OpenCL::RemoteBuffer /*float */ samples_mem;
extern OpenCL::RemoteBuffer /*PRNG  */ prng_mem;

/**
 * Initializes the buffers, compiles the kernels and assigns their arguments.
 */
void init(unsigned int const w,
          unsigned int const h,
          OpenCL::Kernel& tracer,
          size_t const primitive_size,
          size_t const octree_size);

/**
 * Frees environment resources.
 */
void close(void);

/**
 * Writes the scene definition to the remote buffers.
 */
void push_scene(Scene const& scene);

/**
 * Runs the tracer kernel.
 * @param tracr - The tracer kernel.
 */
void render(OpenCL::Kernel const& tracer);

void write_buffer(OpenCL::RemoteBuffer const& buffer, void* data);
void read_buffer(OpenCL::RemoteBuffer const& buffer, void* data);
}

#endif
