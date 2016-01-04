#ifndef __CL_OCGL_H__
#define __CL_OCGL_H__

#include <CL/cl.h>
#include <string>

// gcc -std=c99 openCLTest.c -o openCLTest -lOpenCL

/*
by remote i mean the GPU/CPU-internal memory
and local is userspace
*/

namespace OpenCL
{

char const* translate_cl_error(cl_int error);

class OpenCLException
{
private:
  cl_int error;
  std::string message;

public:
  OpenCLException(cl_int err, std::string& msg);
  virtual ~OpenCLException(void) {}
  void print(void);
};

struct RemoteBuffer
{
  size_t size;
  cl_mem buffer;
};

/**
 * @param queue - The work queue
 * @param remote_buffer - The remote buffer to write to
 * @param data -> A pointer to *remote_buffer.size* bytes of data
 */
void writeBufferBlocking(cl_command_queue const& queue,
                         RemoteBuffer const& remote_buffer,
                         void const* data);

/**
 * @param queue - The work queue
 * @param remote_buffer - The remote buffer to write to
 * @param data -> A pointer to *remote_buffer.size* bytes
 */
void readBufferBlocking(cl_command_queue const& queue,
                        RemoteBuffer const& remote_buffer,
                        void* data);

class Environment
{
public:
  /**
   * Dummy constructor.
   */
  Environment();

  /**
   * The actual constructor.
   */
  Environment(cl_platform_id const platform,
              cl_uint const dev_count,
              cl_device_id* const dev_ids,
              cl_context const cont);
  virtual ~Environment(void){};

  void release(void) const;

  cl_platform_id const m_platform;
  cl_uint const device_count;
  cl_device_id* const device_ids;

  cl_context const m_context;

  /**
   * Creates a remote buffer of *byte_size* bytes size and fills it with *bytes*
   */
  RemoteBuffer allocate_space(size_t byte_size, void* bytes) const;

  /**
   * Creates a command queue.
   * @return A new command queue object
   */
  cl_command_queue createCommandQueue(void) const;
};

class Kernel
{
private:
  std::string const file_path;
  std::string const main_function;
  cl_program m_program;
  cl_kernel m_kernel;

  void load(Environment const& c);
  void build(Environment const& c);
  void create_kernel(void);

public:
  /**
   * Prepares a Kernel object.
   * @param fpath - File path to the kernel source code
   * @param mname - Name of the main function
   */
  Kernel(std::string const& fpath, std::string const& mname);
  virtual ~Kernel(void){};

  /**
   * Releases the associated ressources.
   */
  void release(void);

  /**
   * Loads the program and compiles it to a kernel
   * @param c - The OpenCL context
   */
  void make(Environment const& c);

  /**
   * Assigns the kernel parameters.
   * @param nr - The position of the parameter [0..n-1]
   * @param size - The size of the argument
   * @param data - A pointer to the argument value
   */
  void set_argument(unsigned int nr, size_t size, void* data);
  void set_argument(unsigned int nr, RemoteBuffer buf);

  /**
   * Put kernel(action) into queue
   * Returns an event by which the computation can be identified
   */
  cl_event enqueue_kernel(size_t width, cl_command_queue& queue) const;
  cl_event
  enqueue_kernel(size_t width, size_t height, cl_command_queue& queue) const;
};

/**
 * Inits platform ids
 * Inits device ids from platform 0
 */
Environment const* init(void);

#define print_platform_info(platform, param)                                   \
  _print_platform_info(platform, param, #param)
void _print_platform_info(cl_platform_id platform,
                          cl_platform_info param,
                          std::string param_name);

#define list_devices(platform, type) _list_devices(platform, type, #type)
std::pair<cl_uint, cl_device_id*> _list_devices(cl_platform_id platform,
                                                cl_device_type type,
                                                std::string type_name);

#define print_device_info(device, param, type)                                 \
  _print_device_info<type>(device, param, #param)
template <typename T>
void _print_device_info(cl_device_id device,
                        cl_device_info param,
                        std::string param_name);

/**
 * Blocks until the given event has been completed.
 */
void waitForEvent(cl_event& e);

/**
 * Returns the status code of the event. non-blocking.
 */
cl_int getEventStatus(cl_event& e);
}

#endif
