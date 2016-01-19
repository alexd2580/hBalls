#ifndef __CL_OCGL_H__
#define __CL_OCGL_H__

#include <CL/cl.hpp>
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
  cl::Buffer buffer;
};

/**
 * @param queue - The work queue
 * @param remote_buffer - The remote buffer to write to
 * @param data -> A pointer to *remote_buffer.size* bytes of data
 */
void writeBufferBlocking(cl::CommandQueue const& queue,
                         RemoteBuffer const& remote_buffer,
                         void const* data);

/**
 * @param queue - The work queue
 * @param remote_buffer - The remote buffer to write to
 * @param data -> A pointer to *remote_buffer.size* bytes
 */
void readBufferBlocking(cl::CommandQueue const& queue,
                        RemoteBuffer const& remote_buffer,
                        void* data);

class Environment
{
public:
  Environment(void);
  virtual ~Environment(void){};

  cl::Platform m_platform;
  std::vector<cl::Device> m_devices;
  cl::Context m_context;

  /**
   * Creates a remote buffer of *byte_size* bytes size and fills it with *bytes*
   */
  RemoteBuffer allocate_space(size_t byte_size, void* bytes) const;

  /**
   * Creates a command queue.
   * @return A new command queue object
   */
  cl::CommandQueue createCommandQueue(void) const;
};

class Kernel
{
private:
  std::string const file_path;
  std::string const main_function;
  cl::Program m_program;
  cl::Kernel m_kernel;

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
   * Loads the program and compiles it to a kernel
   * @param c - The OpenCL context
   */
  void make(Environment const& c);

  /**
   * Assigns the kernel parameters.
   * @param nr - The position of the parameter [0..n-1]
   * @param data - A pointer to the argument value
   */
  template <typename T> void set_argument(unsigned int nr, T const& data);
  void set_argument(unsigned int nr, RemoteBuffer const& buf);

  /**
   * Put kernel(action) into queue
   * Returns an event by which the computation can be identified
   */
  cl::Event enqueue(size_t const width, cl::CommandQueue const& queue) const;
  cl::Event enqueue(size_t const width,
                    size_t const height,
                    cl::CommandQueue const& queue) const;
};

#define print_platform_info(platform, param)                                   \
  _print_platform_info(platform, param, #param)
void _print_platform_info(cl::Platform const& platform,
                          cl_platform_info const param,
                          std::string const param_name);

#define list_devices(devices, platform, type)                                  \
  _list_devices(devices, platform, type, #type)
void _list_devices(std::vector<cl::Device>& devices,
                   cl::Platform const& platform,
                   cl_device_type const type,
                   std::string const type_name);

#define print_device_info(device, param, type)                                 \
  _print_device_info<type>(device, param, #param)
template <typename T>
void _print_device_info(cl::Device const& device,
                        cl_device_info const param,
                        std::string const param_name);

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
