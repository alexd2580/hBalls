#include"cl.hpp"

#include<iostream>
#include<cstdlib>
#include<fstream>

using namespace std;

char const* translate_cl_error(cl_int error)
{
  #define ERRORCASE(e) case e: return #e;
  switch(error)
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
  cerr << "[OpenCL] Error " << translate_cl_error(error)
    << " := " << error << ": " << message << endl;
}

cl_int OpenCL::error;

typedef struct event_queue_ event_queue;
struct event_queue_
{
    cl_event current;
    event_queue* next;
};

OpenCL::~OpenCL(void)
{
    cout << "[OpenCL] Closing." << endl;
    clReleaseContext(context);
    if(device_ids != nullptr) free(device_ids);
    if(platform_ids != nullptr) free(platform_ids);
    cout << "[OpenCL] Closed." << endl;
}

bool load_file(string& name, string& content)
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

void OpenCL::_print_platform_info
(
    unsigned int platform,
    cl_platform_info param,
    string param_name
)
{
  size_t size;
  error = clGetPlatformInfo(platform_ids[platform], param, 0, nullptr, &size);
  if(error != CL_SUCCESS)
  {
    string msg("Could not get parameter text size for " + param_name + ".");
    throw OpenCLException(error, msg);
  }

  char* text = (char*)alloca(sizeof(char) * size);
  error = clGetPlatformInfo(platform_ids[platform], param, size, text, nullptr);
  if(error != CL_SUCCESS)
  {
    string msg("Could not get parameter " + param_name + ".");
    throw OpenCLException(error, msg);
  }

  cout << "[OpenCL] " << param_name << ":\t" << text << endl;
}

void OpenCL::_list_devices(unsigned int platform, cl_device_type type, std::string type_name)
{
  cout << "[OpenCL] Listing devices of type " << type_name
    << " on platform " << platform << "." << endl;

  error = clGetDeviceIDs(platform_ids[platform], type, 0, nullptr, &device_count);
  if(error != CL_SUCCESS)
  {
    string msg("Could not get device count (" + type_name + ") for platform "
      + std::to_string(platform) + ".");
    throw OpenCLException(error, msg);
  }
  device_ids = (cl_device_id*)malloc(sizeof(cl_device_id)*device_count);
  if(device_ids == nullptr)
  {
    string msg("Memory allocation failed.");
    throw OpenCLException(0, msg);
  }

  error = clGetDeviceIDs(platform_ids[platform], type, device_count, device_ids, nullptr);
  if(error != CL_SUCCESS)
  {
    string msg("Could not get device ids (" + type_name
      + ") on platform " + std::to_string(platform) + ".");
    throw OpenCLException(error, msg);
  }

  for(unsigned int i=0; i<device_count; i++)
  {
      cout << "[OpenCL] Device " << i << ":" << endl;
      print_device_info(i, CL_DEVICE_AVAILABLE, cl_bool);
      print_device_info(i, CL_DEVICE_COMPILER_AVAILABLE, cl_bool);
      print_device_info(i, CL_DEVICE_ERROR_CORRECTION_SUPPORT, cl_bool);
      print_device_info(i, CL_DEVICE_GLOBAL_MEM_SIZE, cl_ulong);
      print_device_info(i, CL_DEVICE_MAX_CLOCK_FREQUENCY, cl_uint);
      print_device_info(i, CL_DEVICE_MAX_COMPUTE_UNITS, cl_uint);
      print_device_info(i, CL_DEVICE_MAX_CONSTANT_ARGS, cl_uint);
      print_device_info(i, CL_DEVICE_MAX_WORK_GROUP_SIZE, size_t);
      print_device_info(i, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, cl_uint);
  }
  cout << endl;
}

template<typename T> void OpenCL::_print_device_info
(
  unsigned device,
  cl_device_info param,
  std::string param_name
)
{
  T res;
  error = clGetDeviceInfo(device_ids[device], param, sizeof(T), &res, nullptr);
  if(error != CL_SUCCESS)
  {
    string msg("Could not get parameter " + param_name + " of device "
      + std::to_string(device) + ".");
    throw OpenCLException(error, msg);
  }

  cout << "[OpenCL] " << param_name << "=" << res << endl;
}

OpenCL::OpenCL(void)
{
    platform_ids = nullptr;
    device_ids = nullptr;
}

void OpenCL::init(void)
{
    cout << "[OpenCL] Initializing." << endl;
    //platformCount, platformIDs
    error = clGetPlatformIDs(0, nullptr, &platform_count);
    if(error != CL_SUCCESS)
    {
      string msg("Could not get platform count.");
      throw OpenCLException(error, msg);
    }

    platform_ids = (cl_platform_id*)malloc(sizeof(cl_platform_id)*platform_count);
    if(platform_ids == nullptr)
    {
      string msg("Memory allocation failed.");
      throw OpenCLException(error, msg);
    }

    error = clGetPlatformIDs(platform_count, platform_ids, nullptr);
    if(error != CL_SUCCESS)
    {
      string msg("Could not get platform ids.");
      throw OpenCLException(error, msg);
    }

    for(unsigned int i=0; i<platform_count; i++)
    {
        cout << "[OpenCL] Platform " << i << ":" << endl;
        print_platform_info(i, CL_PLATFORM_VERSION);
        print_platform_info(i, CL_PLATFORM_NAME);
        print_platform_info(i, CL_PLATFORM_VENDOR);
        print_platform_info(i, CL_PLATFORM_EXTENSIONS);
    }

    list_devices(0, CL_DEVICE_TYPE_GPU);

    //create context
    cl_context_properties contextProperties[] =
    { CL_CONTEXT_PLATFORM, (cl_context_properties)(platform_ids[0]), 0, 0 };

    context = clCreateContext(contextProperties, device_count, device_ids, nullptr, nullptr, &error);
    if(error != CL_SUCCESS)
    {
      string msg("Could not create context.");
      throw OpenCLException(error, msg);
    }

    cout << "[OpenCL] Started." << endl;
}

cl_mem OpenCL::allocate_space(size_t byte_size, void* bytes)
{
    cl_mem remoteBuffer = clCreateBuffer(context,
        CL_MEM_READ_WRITE | (bytes != nullptr ? CL_MEM_COPY_HOST_PTR : 0),
        byte_size, bytes, &error);
    if(error != CL_SUCCESS)
    {
      string msg("Could not create remote buffer of size " + std::to_string(byte_size) + ".");
      throw OpenCLException(error, msg);
    }
    return remoteBuffer;
}

cl_kernel OpenCL::load_program(string& fpath, string& mainFunction)
{
    string kernel_string;
    if(!load_file(fpath, kernel_string))
    {
      string msg("Could not read file " + fpath + ".");
      throw OpenCLException(0, msg);
    }

    char const* kernel_string_ptr = kernel_string.c_str();
    cl_program program = clCreateProgramWithSource(context, 1,
        &kernel_string_ptr, nullptr, &error);
    if(error != CL_SUCCESS)
    {
      string msg("Could not create program.");
      throw OpenCLException(error, msg);
    }

    //build
    error = clBuildProgram(program, device_count, device_ids, nullptr, nullptr, nullptr);
    //checkError(error);

    switch(error)
    {
    case CL_SUCCESS:
      break;
    case CL_BUILD_PROGRAM_FAILURE:
      {
        // Determine the size of the log
        size_t log_size;
        clGetProgramBuildInfo(program, device_ids[0], CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
        // Allocate memory for the log
        char* log = (char*)alloca(log_size*sizeof(char));
        // Get the log
        clGetProgramBuildInfo(program, device_ids[0], CL_PROGRAM_BUILD_LOG, log_size, log, nullptr);
        // Print the log

        string log_str(log);
        string msg("Could not build program " + fpath + ":\n" + log);
        throw OpenCLException(error, msg);
      }
    default:
      {
        string msg("Could not build program " + fpath + ".");
        throw OpenCLException(error, msg);
      }
    }

    //create kernel
    cl_kernel kernel = clCreateKernel(program, mainFunction.c_str(), &error);
    if(error != CL_SUCCESS)
    {
      string msg("Could not create kernel.");
      throw OpenCLException(error, msg);
    }

    return kernel;
}

void OpenCL::waitForEvent(cl_event& e)
{
    error = clWaitForEvents(1, &e);
    if(error != CL_SUCCESS)
    {
      string msg("Waiting for event failed.");
      throw OpenCLException(error, msg);
    }
}

cl_int OpenCL::getEventStatus(cl_event& e)
{
    cl_int stat;
    error = clGetEventInfo(e, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(cl_int), &stat, NULL);
    if(error != CL_SUCCESS)
    {
      string msg("Could not get event status.");
      throw OpenCLException(error, msg);
    }
    return stat;
}

void OpenCL::setArgument(cl_kernel& kernel, int nr, int arg_size, void* arg)
{
    error = clSetKernelArg(kernel, (cl_uint)nr, (size_t)arg_size, arg);
    if(error != CL_SUCCESS)
    {
      string msg("Could not set kernel argument.");
      throw OpenCLException(error, msg);
    }
}

cl_command_queue OpenCL::createCommandQueue(void)
{
    cl_command_queue queue = clCreateCommandQueue(context, device_ids[0], 0, &error);
    if(error != CL_SUCCESS)
    {
      string msg("COuld not create command queue.");
      throw OpenCLException(error, msg);
    }
    return queue;
}

cl_event OpenCL::enqueue_kernel(size_t width, cl_kernel& kernel, cl_command_queue& queue)
{
  const size_t workSize[] = { width, 0, 0 };
  cl_event event;
  error = clEnqueueNDRangeKernel(queue, kernel,
    1,
    nullptr,
    workSize,
    nullptr,
    0, nullptr, &event);
  if(error != CL_SUCCESS)
  {
    string msg("Could not enqueue kernel.");
    throw OpenCLException(error, msg);
  }
  return event;
}

cl_event OpenCL::enqueue_kernel(size_t width, size_t height, cl_kernel& kernel, cl_command_queue& queue)
{
  const size_t workSize[] = { width, height, 0 };
  cl_event event;
  error = clEnqueueNDRangeKernel(queue, kernel,
    2,
    nullptr,
    workSize,
    nullptr,
    0, nullptr, &event);
  if(error != CL_SUCCESS)
  {
    string msg("Could not enqueue kernel.");
    throw OpenCLException(error, msg);
  }
  return event;
}

void OpenCL::writeBufferBlocking(cl_command_queue& queue, cl_mem buffer, size_t bytes, void* data)
{
  error = clEnqueueWriteBuffer(queue, buffer, CL_TRUE,
    0, bytes, data, 0, nullptr, nullptr);
  if(error != CL_SUCCESS)
  {
    string msg("Could not write to buffer.");
    throw OpenCLException(error, msg);
  }
}

void OpenCL::readBufferBlocking(cl_command_queue& queue, cl_mem remoteBuf, size_t bytes, void* localBuf)
{
  error = clEnqueueReadBuffer(queue, remoteBuf, CL_TRUE, 0,
    bytes, localBuf, 0, nullptr, nullptr);
  if(error != CL_SUCCESS)
  {
    string msg("Could not read from buffer.");
    throw OpenCLException(error, msg);
  }
}
