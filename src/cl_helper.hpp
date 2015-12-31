#ifndef __CL_HELPER_USW__
#define __CL_HELPER_USW__

#include<cstdint>
#include<CL/cl.h>

#include"cl.hpp"

#define ULONG_SIZE SIZE_OF(unsigned long)

/**
 * This namepace holds the openc environment as well
 * as the path_tracer and its buffers
 */
namespace CLHelper
{
  /**
   * Buffers on GPU
   */
   extern OpenCL::RemoteBuffer /*float */ fov_mem;
   extern OpenCL::RemoteBuffer /*float */ objects_mem;
   extern OpenCL::RemoteBuffer /*char4 */ frame_c_mem;
   extern OpenCL::RemoteBuffer /*float4*/ frame_f_mem;
   extern OpenCL::RemoteBuffer /*float */ samples_mem;
   extern OpenCL::RemoteBuffer /*PRNG  */ prng_mem;

  extern cl_command_queue queue;
  extern OpenCL::Kernel* path_tracer;
  extern OpenCL::Kernel* buffer_cleaner;

  void clearBuffers(void);
  void init
  (
    unsigned int w,
    unsigned int h,
    std::string& tracepath,
    std::string& cleanpath,
    OpenCL::Environment& cl
  );
  void close(void);
  void pushScene(void);
  void render(void);

}

#endif
