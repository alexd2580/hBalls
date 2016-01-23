#include "cl.hpp"

#include <iostream>
#include <cstdlib>
#include <fstream>

using namespace std;

bool load_file(string const& name, string& content)
{
  // We create the file object, saying I want to read it
  fstream file(name.c_str(), fstream::in);

  // We verify if the file was successfully opened
  if(file.is_open())
  {
    // We use the standard getline function to read the file into
    // a std::string, stoping only at "\0"
    getline(file, content, '\0');

    // We return the success of the operation
    return !file.bad();
  }

  // The file was not successfully opened, so returning false
  return false;
}

namespace OpenCL
{

cl_int error;

__attribute__((const)) char const* translate_cl_error(cl_int err)
{
#define ERRORCASE(e)                                                           \
  case e:                                                                      \
    return #e;
  switch(err)
  {
    ERRORCASE(CL_SUCCESS)
    ERRORCASE(CL_DEVICE_NOT_FOUND)
    ERRORCASE(CL_DEVICE_NOT_AVAILABLE)
    ERRORCASE(CL_COMPILER_NOT_AVAILABLE)
    ERRORCASE(CL_MEM_OBJECT_ALLOCATION_FAILURE)
    ERRORCASE(CL_OUT_OF_RESOURCES)
    ERRORCASE(CL_OUT_OF_HOST_MEMORY)
    ERRORCASE(CL_PROFILING_INFO_NOT_AVAILABLE)
    ERRORCASE(CL_MEM_COPY_OVERLAP)
    ERRORCASE(CL_IMAGE_FORMAT_MISMATCH)
    ERRORCASE(CL_IMAGE_FORMAT_NOT_SUPPORTED)
    ERRORCASE(CL_BUILD_PROGRAM_FAILURE)
    ERRORCASE(CL_MAP_FAILURE)
    ERRORCASE(CL_MISALIGNED_SUB_BUFFER_OFFSET)
    ERRORCASE(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST)
    ERRORCASE(CL_COMPILE_PROGRAM_FAILURE)
    ERRORCASE(CL_LINKER_NOT_AVAILABLE)
    ERRORCASE(CL_LINK_PROGRAM_FAILURE)
    ERRORCASE(CL_DEVICE_PARTITION_FAILED)
    ERRORCASE(CL_KERNEL_ARG_INFO_NOT_AVAILABLE)

    ERRORCASE(CL_INVALID_VALUE)
    ERRORCASE(CL_INVALID_DEVICE_TYPE)
    ERRORCASE(CL_INVALID_PLATFORM)
    ERRORCASE(CL_INVALID_DEVICE)
    ERRORCASE(CL_INVALID_CONTEXT)
    ERRORCASE(CL_INVALID_QUEUE_PROPERTIES)
    ERRORCASE(CL_INVALID_COMMAND_QUEUE)
    ERRORCASE(CL_INVALID_HOST_PTR)
    ERRORCASE(CL_INVALID_MEM_OBJECT)
    ERRORCASE(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR)
    ERRORCASE(CL_INVALID_IMAGE_SIZE)
    ERRORCASE(CL_INVALID_SAMPLER)
    ERRORCASE(CL_INVALID_BINARY)
    ERRORCASE(CL_INVALID_BUILD_OPTIONS)
    ERRORCASE(CL_INVALID_PROGRAM)
    ERRORCASE(CL_INVALID_PROGRAM_EXECUTABLE)
    ERRORCASE(CL_INVALID_KERNEL_NAME)
    ERRORCASE(CL_INVALID_KERNEL_DEFINITION)
    ERRORCASE(CL_INVALID_KERNEL)
    ERRORCASE(CL_INVALID_ARG_INDEX)
    ERRORCASE(CL_INVALID_ARG_VALUE)
    ERRORCASE(CL_INVALID_ARG_SIZE)
    ERRORCASE(CL_INVALID_KERNEL_ARGS)
    ERRORCASE(CL_INVALID_WORK_DIMENSION)
    ERRORCASE(CL_INVALID_WORK_GROUP_SIZE)
    ERRORCASE(CL_INVALID_WORK_ITEM_SIZE)
    ERRORCASE(CL_INVALID_GLOBAL_OFFSET)
    ERRORCASE(CL_INVALID_EVENT_WAIT_LIST)
    ERRORCASE(CL_INVALID_EVENT)
    ERRORCASE(CL_INVALID_OPERATION)
    ERRORCASE(CL_INVALID_GL_OBJECT)
    ERRORCASE(CL_INVALID_BUFFER_SIZE)
    ERRORCASE(CL_INVALID_MIP_LEVEL)
    ERRORCASE(CL_INVALID_GLOBAL_WORK_SIZE)
    ERRORCASE(CL_INVALID_PROPERTY)
    ERRORCASE(CL_INVALID_IMAGE_DESCRIPTOR)
    ERRORCASE(CL_INVALID_COMPILER_OPTIONS)
    ERRORCASE(CL_INVALID_LINKER_OPTIONS)
    ERRORCASE(CL_INVALID_DEVICE_PARTITION_COUNT)
  default:
    return "undefined";
  }
}

OpenCLException::OpenCLException(cl_int err, string& msg)
    : error(err), message(msg)
{
}

void OpenCLException::print(void)
{
  cerr << "[OpenCL] Error " << translate_cl_error(error) << " := " << error
       << ": " << message << endl;
}

/******************************************************************************/
/******************************************************************************/

Environment::Environment(unsigned int platform_num, cl_device_type dev_type)
{
  cout << "[OpenCL] Initializing." << endl;

  vector<cl::Platform> platforms;
  error = cl::Platform::get(&platforms);
  if(error != CL_SUCCESS)
  {
    string msg("Could not get platforms.");
    throw OpenCLException(error, msg);
  }

  for(auto i = platforms.begin(); i != platforms.end(); i++)
  {
    cout << "[OpenCL] Platform:" << endl;
    print_platform_info(*i, CL_PLATFORM_VERSION);
    print_platform_info(*i, CL_PLATFORM_NAME);
    print_platform_info(*i, CL_PLATFORM_VENDOR);
    print_platform_info(*i, CL_PLATFORM_EXTENSIONS);
  }

  // SELECT CPU VS GPU HERE!!!!
  m_platform = platforms[platform_num];
  list_devices(m_devices, m_platform, dev_type);

  m_context = cl::Context(m_devices, nullptr, nullptr, nullptr, &error);
  if(error != CL_SUCCESS)
  {
    string msg("Could not create context.");
    throw OpenCLException(error, msg);
  }

  cout << "[OpenCL] Done." << endl;
}

RemoteBuffer Environment::allocate(size_t byte_size) const
{
  cl::Buffer remote_buffer(
      m_context, CL_MEM_READ_WRITE | 0, byte_size, nullptr, &error);
  if(error != CL_SUCCESS)
  {
    string msg("Could not create remote buffer of size " +
               std::to_string(byte_size) + ".");
    throw OpenCLException(error, msg);
  }

  RemoteBuffer rb;
  rb.size = byte_size;
  rb.buffer = remote_buffer;
  return rb;
}

RemoteBuffer Environment::allocate(size_t byte_size, void* bytes) const
{
  cl::Buffer remote_buffer(m_context,
                           CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                           byte_size,
                           bytes,
                           &error);
  if(error != CL_SUCCESS)
  {
    string msg("Could not create remote buffer of size " +
               std::to_string(byte_size) + ".");
    throw OpenCLException(error, msg);
  }

  RemoteBuffer rb;
  rb.size = byte_size;
  rb.buffer = remote_buffer;
  return rb;
}

cl::CommandQueue Environment::create_queue(void) const
{
  cl::CommandQueue queue(m_context, m_devices[0], 0, &error);
  if(error != CL_SUCCESS)
  {
    string msg("Could not create command queue.");
    throw OpenCLException(error, msg);
  }
  return queue;
}

void writeBufferBlocking(cl::CommandQueue const& queue,
                         RemoteBuffer const& remote,
                         void const* data)
{
  queue.finish();
  error = queue.enqueueWriteBuffer(
      remote.buffer, CL_TRUE, 0, remote.size, data, nullptr, nullptr);
  if(error != CL_SUCCESS)
  {
    string msg("Could not write to buffer.");
    throw OpenCLException(error, msg);
  }
}

void readBufferBlocking(cl::CommandQueue const& queue,
                        RemoteBuffer const& remote,
                        void* data)
{
  queue.finish();
  error = queue.enqueueReadBuffer(
      remote.buffer, CL_TRUE, 0, remote.size, data, nullptr, nullptr);
  if(error != CL_SUCCESS)
  {
    string msg("Could not read from buffer.");
    throw OpenCLException(error, msg);
  }
}

/******************************************************************************/
/******************************************************************************/

Kernel::Kernel(string const& fpath, string const& mname)
    : file_path(fpath), main_function(mname)
{
  m_kernel = nullptr;
  m_program = nullptr;
}

void Kernel::load(Environment const& context)
{
  string kernel_string;
  if(!load_file(file_path, kernel_string))
  {
    string msg("Could not read file " + file_path + ".");
    throw OpenCLException(0, msg);
  }

  char const* kernel_string_ptr = kernel_string.c_str();
  vector<pair<const char*, size_t>> sources;
  sources.push_back(
      pair<const char*, size_t>(kernel_string_ptr, kernel_string.size()));
  m_program = cl::Program(context.m_context, sources, &error);
  if(error != CL_SUCCESS)
  {
    string msg("Could not create program.");
    throw OpenCLException(error, msg);
  }
}

void Kernel::build(Environment const& context)
{
  error = m_program.build(context.m_devices);
  switch(error)
  {
  case CL_SUCCESS:
    break;
  case CL_BUILD_PROGRAM_FAILURE:
  {
    string log;
    m_program.getBuildInfo(context.m_devices[0], CL_PROGRAM_BUILD_LOG, &log);
    string msg("Could not build program " + file_path + ":\n" + log);
    throw OpenCLException(error, msg);
  }
  default:
  {
    string msg("Could not build program " + file_path + ".");
    throw OpenCLException(error, msg);
  }
  }
}

void Kernel::create_kernel(void)
{
  m_kernel = cl::Kernel(m_program, main_function.c_str(), &error);
  if(error != CL_SUCCESS)
  {
    string msg("Could not create kernel.");
    throw OpenCLException(error, msg);
  }

  /*cl_ulong local_mem;
  error = m_kernel.getWorkGroupInfo(CL_KERNEL_LOCAL_MEM_SIZE, &local_mem);
  if(error != CL_SUCCESS)
  {
    string msg("Could not get kernel local mem size.");
    throw OpenCLException(error, msg);
  }

  cout << "[" << file_path << "] Local mem size: " << (long)local_mem << endl;*/

  // pocl ** * ******* ***** ** **** (private mem unimplemented, would fail)
  /*cl_ulong private_mem;
  error = clGetKernelWorkGroupInfo(m_kernel,
                                   context.device_ids[0],
                                   CL_KERNEL_PRIVATE_MEM_SIZE,
                                   sizeof(cl_ulong),
                                   &private_mem,
                                   nullptr);
  if (error != CL_SUCCESS)
  {
    string msg("Could not create kernel private mem size.");
    throw OpenCLException(error, msg);
  }

  cout << "[" << file_path << "] Private memory size: " << private_mem <<
  endl;*/
}

void Kernel::make(Environment const& c)
{
  load(c);
  build(c);
  create_kernel();
}

void Kernel::set_argument(unsigned int nr, RemoteBuffer const& buf)
{
  error = m_kernel.setArg(nr, buf.buffer);
  if(error != CL_SUCCESS)
  {
    string msg("Could not set kernel argument " + std::to_string(nr) + ".");
    throw OpenCLException(error, msg);
  }
}

cl::Event Kernel::enqueue(size_t const width,
                          cl::CommandQueue const& queue) const
{
  cl::Event event;
  error = queue.enqueueNDRangeKernel(m_kernel,
                                     cl::NullRange,
                                     cl::NDRange(width, 0, 0),
                                     cl::NullRange, // cl::NDRange(width, 0, 0),
                                     nullptr,
                                     &event);

  if(error != CL_SUCCESS)
  {
    string msg("Could not enqueue kernel.");
    throw OpenCLException(error, msg);
  }
  return event;
}

cl::Event Kernel::enqueue(size_t const width,
                          size_t const height,
                          cl::CommandQueue const& queue) const
{
  cl::Event event;
  error = queue.enqueueNDRangeKernel(m_kernel,
                                     cl::NDRange(0, 0),
                                     cl::NDRange(width, height),
                                     cl::NullRange, // cl::NDRange(width, 1),
                                     nullptr,
                                     &event);
  if(error != CL_SUCCESS)
  {
    string msg("Could not enqueue kernel.");
    throw OpenCLException(error, msg);
  }
  return event;
}

/******************************************************************************/
/******************************************************************************/

void _print_platform_info(cl::Platform const& platform,
                          cl_platform_info const param,
                          string const param_name)
{
  string val;
  error = platform.getInfo(param, &val);
  if(error != CL_SUCCESS)
  {
    string msg("Could not get parameter " + param_name + ".");
    throw OpenCLException(error, msg);
  }

  cout << "[OpenCL] " << param_name << "=\t" << val << endl;
}

void _list_devices(vector<cl::Device>& devices,
                   cl::Platform const& platform,
                   cl_device_type const type,
                   std::string const type_name)
{
  cout << "[OpenCL] Listing devices of type " << type_name << "." << endl;

  error = platform.getDevices(type, &devices);
  if(error != CL_SUCCESS)
  {
    string msg("Could not get devices of type (" + type_name + ").");
    throw OpenCLException(error, msg);
  }

  for(auto i = devices.begin(); i != devices.end(); i++)
  {
    cout << "[OpenCL] Device:" << endl;
    print_device_info(*i, CL_DEVICE_NAME, string);
    print_device_info(*i, CL_DEVICE_VENDOR, string);
    print_device_info(*i, CL_DEVICE_VERSION, string);
    print_device_info(*i, CL_DRIVER_VERSION, string);
    print_device_info(*i, CL_DEVICE_OPENCL_C_VERSION, string);
    print_device_info(*i, CL_DEVICE_AVAILABLE, cl_bool);
    print_device_info(*i, CL_DEVICE_COMPILER_AVAILABLE, cl_bool);
    print_device_info(*i, CL_DEVICE_ERROR_CORRECTION_SUPPORT, cl_bool);
    print_device_info(*i, CL_DEVICE_GLOBAL_MEM_SIZE, cl_ulong);
    print_device_info(*i, CL_DEVICE_MAX_CLOCK_FREQUENCY, cl_uint);
    print_device_info(*i, CL_DEVICE_MAX_COMPUTE_UNITS, cl_uint);
    print_device_info(*i, CL_DEVICE_MAX_CONSTANT_ARGS, cl_uint);
    print_device_info(*i, CL_DEVICE_MAX_WORK_GROUP_SIZE, size_t);
    print_device_info(*i, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, cl_uint);
  }
  cout << endl;
}

template <typename T>
void _print_device_info(cl::Device const& device,
                        cl_device_info const param,
                        std::string const param_name)
{
  T res;
  error = device.getInfo<T>(param, &res);
  if(error != CL_SUCCESS)
  {
    string msg("Could not get parameter " + param_name + ".");
    throw OpenCLException(error, msg);
  }

  cout << "[OpenCL] " << param_name << " =\t" << res << endl;
}

void waitForEvent(cl_event& e)
{
  error = clWaitForEvents(1, &e);
  if(error != CL_SUCCESS)
  {
    string msg("Waiting for event failed.");
    throw OpenCLException(error, msg);
  }
}

cl_int getEventStatus(cl_event& e)
{
  cl_int stat;
  error = clGetEventInfo(
      e, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(cl_int), &stat, NULL);
  if(error != CL_SUCCESS)
  {
    string msg("Could not get event status.");
    throw OpenCLException(error, msg);
  }
  return stat;
}
}
