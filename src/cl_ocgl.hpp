#ifndef __CL_OCGL_H__
#define __CL_OCGL_H__

#include<CL/cl.h>
#include<string>

//gcc -std=c99 openCLTest.c -o openCLTest -lOpenCL

/*
by remote i mean the GPU/CPU-internal memory
and local is userspace
*/


class OpenCL
{
private:
  cl_uint platform_count;
  cl_platform_id* platform_ids;
  cl_uint device_count;
  cl_device_id* device_ids;

  cl_context context;

  static cl_int error;
  static void checkError(void);
  static void checkError(std::string);

  #define print_platform_info(platform, param) \
    _print_platform_info(platform, param, #param)
  void _print_platform_info(unsigned int platform, cl_platform_info param, std::string param_name);

  #define list_devices(platform, type) \
    _list_devices(platform, type, #type)
  void _list_devices(unsigned int platform, cl_device_type type, std::string type_name);

  #define print_device_info(device, param, type) \
    _print_device_info<type>(device, param, #param)
  template<typename T> void _print_device_info(unsigned int device, cl_device_info type, std::string type_name);
public:
  /**
   * Inits platform ids
   * Inits device ids from platform 0
   */
  OpenCL();

  /**
   * Frees platform ids
   * Frees device ids
   */
  ~OpenCL(void);

  /**
   * Creates a remote buffer of byte_size bytes size
   * and fills it with bytes
   */
  cl_mem allocate_space(size_t byte_size, void* bytes);

  /**
   * Loads a program and compiles it to a kernel
   * main function is identified by mainFunction
   */
  cl_kernel load_program(std::string& fpath, std::string& mainFunction);

  /**
   * Blocks until the given event has been completed.
   */
  static void waitForEvent(cl_event& e);

  /**
   * Returns the status code of the event. non-blocking.
   */
  static cl_int getEventStatus(cl_event& e);

  /**
   * the argunemt ar nr.th element to the kernel
   */
  static void setArgument(cl_kernel& kernel, int nr, int arg_size, void* arg);

  /**
   * Starts a command queue.
   *
   */
  cl_command_queue createCommandQueue(void);

  /**
   * Put kernel(action) into queue
   * Returns an event by which the computation can be identified
   */
  static cl_event enqueue_kernel(size_t width, cl_kernel& kernel, cl_command_queue& queue);

  /**
   * Input:
   * queue -> eventqueue
   * buffer -> remote buffer to write to
   * bytes -> number of bytes
   * data -> data*
   */
  static void writeBufferBlocking(cl_command_queue& queue, cl_mem buffer, size_t bytes, void* data);

  /**
   * Input:
   * localBuf -> pointer to local buffer
   * bytes -> data size in bytes
   * remoteBuf -> id by which the remote buffer can be identified
   * queue -> queue in which the action was put
   * Side effect:
   * Waits for event to terminate, and copies bytes bytes from remoteBuf
   * to localBuf
   */
  static void readBufferBlocking(cl_command_queue& queue, cl_mem remoteBuf, size_t bytes, void* localBuf);
};

#endif
