#ifndef __CL_HELPER_USW__
#define __CL_HELPER_USW__

#include<cstdint>
#include<CL/cl.h>

#include"cl.hpp"

#define MEM_SIZE SIZE_OF(cl_mem)

#define ULONG_SIZE SIZE_OF(unsigned long)

namespace CLHelper
{
  /**
   * Buffers on GPU
   */
  extern cl_mem /*float */ fov_mem;
  extern cl_mem /*float */ objects_mem;
  extern cl_mem /*char4 */ frame_c_mem;
  extern cl_mem /*float4*/ frame_f_mem;
  extern cl_mem /*float4*/ samples_mem;
  extern cl_mem /*PRNG  */ prng_mem;

  extern cl_command_queue queue;
  extern cl_kernel raytracer;
  extern cl_kernel bufferCleaner;

  void clearBuffers(OpenCL&);
  void init
  (
    unsigned int w,
    unsigned int h,
    std::string& tracepath,
    std::string& cleanpath,
    OpenCL& cl
  );
  void close(void);
  void pushScene(void);
  void render(void);

}

#endif
