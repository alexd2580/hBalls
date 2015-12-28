#include"cl_ocgl.hpp"

#include<iostream>
#include<cstdlib>
#include<fstream>

using namespace std;

cl_int OpenCL::error;

typedef struct event_queue_ event_queue;
struct event_queue_
{
    cl_event current;
    event_queue* next;
};

void OpenCL::checkError(void)
{
  if(error!=CL_SUCCESS)
  {
    cout << "Failure: " << error << endl;
    exit(1);
  }
}

void OpenCL::checkError(string msg)
{
  if(error!=CL_SUCCESS)
  {
    cout << "Failure (" << msg << "): " << error << endl;
    exit(1);
  }
}

OpenCL::~OpenCL(void)
{
    clReleaseContext(context);
    free(device_ids);
    free(platform_ids);
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
  error = clGetPlatformInfo(
    platform_ids[platform], param, 0, nullptr, &size);
  checkError();
  char* text = (char*)malloc(sizeof(char) * size);
  error = clGetPlatformInfo(
    platform_ids[platform], param, size, text, nullptr);
  checkError();
  cout << param_name << ": " << text << endl;
  free(text);
}

void OpenCL::_list_devices(unsigned int platform, cl_device_type type, std::string type_name)
{
  cout << "Listing devices of type " << type_name << " on platform " << platform << endl;

  error = clGetDeviceIDs(platform_ids[platform], type, 0, nullptr, &device_count);
  checkError("clGetDeviceIDs count");
  device_ids = (cl_device_id*)malloc(sizeof(cl_device_id)*device_count);
  if(device_ids == nullptr)
  {
      cout << "Memory allocation failed." << endl;
      exit(-1);
  }

  error = clGetDeviceIDs(platform_ids[platform], type, device_count, device_ids, nullptr);
  checkError("clGetDeviceIDs");

  for(unsigned int i=0; i<device_count; i++)
  {
      cout << "Device: " << i << endl;
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

template<typename T> void OpenCL::_print_device_info(unsigned device, cl_device_info param, std::string param_name)
{
  T res;
  error = clGetDeviceInfo(device_ids[device], param, sizeof(T), &res, nullptr);
  checkError("clGetDeviceInfo device info");

  cout << param_name << "=" << res << endl;
}

OpenCL::OpenCL(void)
{
    //platformCount, platformIDs
    error = clGetPlatformIDs(0, nullptr, &platform_count);
    checkError("clGetPlatformIDs count");
    platform_ids = (cl_platform_id*)malloc(sizeof(cl_platform_id)*platform_count);
    if(platform_ids == nullptr)
    {
        cout << "Memory allocation failed." << endl;
        exit(-1);
    }

    error = clGetPlatformIDs(platform_count, platform_ids, nullptr);
    checkError("clGetPlatformIDs");

    for(unsigned int i=0; i<platform_count; i++)
    {
        cout << "Platform: " << i << endl;

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
    checkError("clCreateContext");

    cout << "OpenCL started" << endl;
}

cl_mem OpenCL::allocate_space(size_t byte_size, void* bytes)
{
    cl_mem remoteBuffer = clCreateBuffer(context,
        CL_MEM_READ_WRITE | (bytes != nullptr ? CL_MEM_COPY_HOST_PTR : 0),
        byte_size, bytes, &error);
    checkError();
    return remoteBuffer;
}

cl_kernel OpenCL::load_program(string& fpath, string& mainFunction)
{
    string kernel_string;
    if(!load_file(fpath, kernel_string))
    {
      cout << "Could not read file " << fpath << endl;
      exit(1); // TODO
    }
    char const* kernel_string_ptr = kernel_string.c_str();
    cl_program program = clCreateProgramWithSource(context, 1,
        &kernel_string_ptr, nullptr, &error);
    checkError();

    //build
    error = clBuildProgram(program, device_count, device_ids, nullptr, nullptr, nullptr);
    //checkError(error);

    if(error == CL_BUILD_PROGRAM_FAILURE)
    {
        // Determine the size of the log
        size_t log_size;
        clGetProgramBuildInfo(program, device_ids[0], CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
        // Allocate memory for the log
        char* log = (char*)malloc(log_size*sizeof(char));
        // Get the log
        clGetProgramBuildInfo(program, device_ids[0], CL_PROGRAM_BUILD_LOG, log_size, log, nullptr);
        // Print the log
        cout << "Error while building program " << fpath << endl << log << endl;
        exit(1);
    }

    //create kernel
    cl_kernel kernel = clCreateKernel(program, mainFunction.c_str(), &error);
    checkError();

    return kernel;
}

void OpenCL::waitForEvent(cl_event& e)
{
    error = clWaitForEvents(1, &e);
    checkError();
}

cl_int OpenCL::getEventStatus(cl_event& e)
{
    cl_int stat;
    error = clGetEventInfo(e, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(cl_int), &stat, NULL);
    checkError();
    return stat;
}

void OpenCL::setArgument(cl_kernel& kernel, int nr, int arg_size, void* arg)
{
    error = clSetKernelArg(kernel, (cl_uint)nr, (size_t)arg_size, arg);
    checkError();
}

cl_command_queue OpenCL::createCommandQueue(void)
{
    cl_command_queue queue = clCreateCommandQueue(context, device_ids[0], 0, &error);
    checkError();
    return queue;
}

cl_event OpenCL::enqueue_kernel(size_t width, cl_kernel& kernel, cl_command_queue& queue)
{
    const size_t workSize[] = { width, 0, 0 };
    cl_event event;
    error = clEnqueueNDRangeKernel(queue, kernel,
        1, // One dimension, because workSize has dimension one
        nullptr,
        workSize,
        nullptr,
        0, nullptr, &event);
    checkError();
    return event;
}

void OpenCL::writeBufferBlocking(cl_command_queue& queue, cl_mem buffer, size_t bytes, void* data)
{
    error = clEnqueueWriteBuffer(queue, buffer, CL_TRUE,
        0, bytes, data, 0, nullptr, nullptr);
    checkError();
}

void OpenCL::readBufferBlocking(cl_command_queue& queue, cl_mem remoteBuf, size_t bytes, void* localBuf)
{
  error = clEnqueueReadBuffer(queue, remoteBuf, CL_TRUE, 0,
    bytes, localBuf, 0, nullptr, nullptr);
  checkError();
}
