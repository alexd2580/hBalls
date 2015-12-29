#ifndef __CL_HELPER_USW__
#define __CL_HELPER_USW__

#include<cstdint>
#include<CL/cl.h>

#include"cl.hpp"

#define MEM_SIZE SIZE_OF(cl_mem)

namespace CLHelper
{
  /**
   * Buffers on GPU
   */
  extern cl_mem /*float*/ fov_mem;
  extern cl_mem /*char */ objects_mem;
  extern cl_mem /*int  */ offsets_mem;
  extern cl_mem /*char */ frameBuf_mem;
  extern cl_mem /*float*/ depthBuf_mem;

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
  void pushScene(void);
  void render(OpenCL&);

}

#endif
