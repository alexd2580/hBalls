#include<CL/cl.h>

#include<stdio.h> 
#include<stdlib.h>

//gcc -std=c99 openCLTest.c -o openCLTest -lOpenCL

typedef struct event_queue_ event_queue;
struct event_queue_
{
    cl_event this;
    event_queue* next;
};


cl_uint platformIdCount = 0;
cl_platform_id* platformIds;
cl_uint deviceIdCount = 0;
cl_device_id* deviceIds;

cl_context context;

cl_int error__;
#define checkError(err) error__ = (err); if(error__!=CL_SUCCESS) { printf("Failure: %d\n", error__); exit(1); }

void closeCL(void)
{
    clReleaseContext(context);
    
    free(deviceIds);
    free(platformIds);
}

char* readProgram(const char* fpath)
{
    FILE* file = fopen(fpath, "r");
    if(file == NULL)
    {
        printf("File cannot be opened!\n");
        return NULL;
    }
    
    fseek(file, 0L, SEEK_END);
    long size = ftell(file);
    fseek(file, 0L, SEEK_SET);
    
    char* program = (char*)malloc((size_t)((int)sizeof(char)*size));
    long offset = 0L;
    
    while(fgets(program+offset, 1000, file) != NULL)
        offset = ftell(file);
        
    return program;
}

/*int main(void)
{
    char* text = NULL;
    text = readProgram("./kernelTest.cl");
    printf("%s", text);
    return 0;
}*/

int setupCL(void)
{
    cl_int error;
    //platformCount, platformIDs
    error = clGetPlatformIDs(0, NULL, &platformIdCount);
    checkError(error);
    platformIds = (cl_platform_id*)malloc(sizeof(cl_platform_id)*platformIdCount);
    if(platformIds == NULL)
    {
        printf("Memory allocation failed.\n");
        exit(-1);
    }
    error = clGetPlatformIDs(platformIdCount, platformIds, NULL);
    checkError(error);
    
    for(size_t i=0; i<platformIdCount; i++)
    {
        printf("Platform: %d\n", (int)i);
        size_t size;
        char* text;
        
        error = clGetPlatformInfo(platformIds[i], CL_PLATFORM_PROFILE, 0, NULL, &size);
        checkError(error);
        text = (char*)malloc(sizeof(char) * size);
        error = clGetPlatformInfo(platformIds[i], CL_PLATFORM_PROFILE, size, text, NULL);
        checkError(error);
        printf("%s\n", text);
        free(text);
        
        error = clGetPlatformInfo(platformIds[i], CL_PLATFORM_VERSION, 0, NULL, &size);
        checkError(error);
        text = (char*)malloc(sizeof(char) * size);
        error = clGetPlatformInfo(platformIds[i], CL_PLATFORM_VERSION, size, text, NULL);
        checkError(error);
        printf("%s\n", text);
        free(text);
        
        error = clGetPlatformInfo(platformIds[i], CL_PLATFORM_NAME, 0, NULL, &size);
        checkError(error);
        text = (char*)malloc(sizeof(char) * size);
        error = clGetPlatformInfo(platformIds[i], CL_PLATFORM_NAME, size, text, NULL);
        checkError(error);
        printf("%s\n", text);
        free(text);
        
        error = clGetPlatformInfo(platformIds[i], CL_PLATFORM_VENDOR, 0, NULL, &size);
        checkError(error);
        text = (char*)malloc(sizeof(char) * size);
        error = clGetPlatformInfo(platformIds[i], CL_PLATFORM_VENDOR, size, text, NULL);
        checkError(error);
        printf("%s\n", text);
        free(text);
        
        error = clGetPlatformInfo(platformIds[i], CL_PLATFORM_EXTENSIONS, 0, NULL, &size);
        checkError(error);
        text = (char*)malloc(sizeof(char) * size);
        error = clGetPlatformInfo(platformIds[i], CL_PLATFORM_EXTENSIONS, size, text, NULL);
        checkError(error);
        printf("%s\n", text);
        free(text);
        putchar('\n');
    }
    
    //deviceCount, deviceIDs
    error = clGetDeviceIDs(platformIds[0], CL_DEVICE_TYPE_CPU, 0, NULL,
        &deviceIdCount);
    checkError(error);
    deviceIds = (cl_device_id*)malloc(sizeof(cl_device_id)*deviceIdCount);
    if(deviceIds == NULL)
    {
        printf("Memory allocation failed.\n");
        exit(-1);
    }    
    error = clGetDeviceIDs(platformIds[0], CL_DEVICE_TYPE_CPU, deviceIdCount,
        deviceIds, NULL);
    checkError(error);
    
    for(size_t i=0; i<deviceIdCount; i++)
    {
        printf("Device: %d\n", (int)i);
        //size_t size;
        //char* text;
        cl_device_type cdt;
        
        error = clGetDeviceInfo(deviceIds[i], CL_DEVICE_TYPE, sizeof(cl_device_type), &cdt, NULL);
        checkError(error);
        switch(cdt)
        {
        case CL_DEVICE_TYPE_CPU:
            printf("CPU\n");
            break;
        case CL_DEVICE_TYPE_GPU:
            printf("GPU\n");
            break;
        case CL_DEVICE_TYPE_ACCELERATOR:
            printf("ACCELERATOR\n");
            break;
        default:
            printf("WTF?!\n");
            break;
        }
        putchar('\n');    
    }
    
    //create context
    cl_context_properties contextProperties[] =
    {
        CL_CONTEXT_PLATFORM,
        (cl_context_properties)(platformIds[0]),
        0, 0
    };
    
    context = clCreateContext(
        contextProperties, deviceIdCount,
        deviceIds, NULL,
        NULL, &error);
    checkError(error);
    
    printf("OpenCL started\n");
    return 0;
}
    
cl_mem allocateSpace(int byte_size, void* bytes)
{
    cl_int error;
    cl_mem remoteBuffer = clCreateBuffer(context,
        CL_MEM_READ_WRITE | (bytes != NULL ? CL_MEM_COPY_HOST_PTR : 0),
        (size_t)byte_size, bytes, &error);
    checkError(error);
    return remoteBuffer;
}
    
cl_kernel loadProgram(const char* fpath, const char* mainFunction)
{
    cl_int error;
    const char* kernel_string = readProgram(fpath);
    cl_program program = clCreateProgramWithSource(context, 1, 
        (const char**) &kernel_string, NULL, &error);
    checkError(error);
    
    //build
    error = clBuildProgram (program, deviceIdCount,
        deviceIds, NULL, NULL, NULL);
    //checkError(error);

    if(error == CL_BUILD_PROGRAM_FAILURE)
    {
        // Determine the size of the log
        size_t log_size;
        clGetProgramBuildInfo(program, deviceIds[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        // Allocate memory for the log
        char *log = (char *)malloc(log_size);
        // Get the log
        clGetProgramBuildInfo(program, deviceIds[0], CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        // Print the log
        printf("Error while building program '%s':\n%s\n", fpath, log); 
        exit(-1);
    }
    
    //create kernel
    cl_kernel kernel = clCreateKernel(program, mainFunction, &error);
    checkError(error);
    
    return kernel;
}

void waitForEvent(cl_event e)
{
    cl_int error;
    error = clWaitForEvents(1, &e);
    checkError(error);
}

cl_int getEventStatus(cl_event e)
{
    cl_int stat;
    cl_int error;
    error = clGetEventInfo(e, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(cl_int), &stat, NULL);
    checkError(error);
    return(stat);
}

void setArgument(cl_kernel kernel, int nr, int arg_size, void* arg)
{
    cl_int error = clSetKernelArg(kernel, (cl_uint)nr, (size_t)arg_size, arg);
    checkError(error);
}
    
cl_command_queue createCommandQueue(void)
{
    cl_int error;
    cl_command_queue queue = clCreateCommandQueue(context, deviceIds[0],
        0, &error);
    checkError(error);
    return queue;
}

cl_event enqueueKernelInQueue(int width, cl_kernel kernel, cl_command_queue queue)
{
    cl_int error;
    const size_t workSize[] = { (size_t)width, 0, 0 };
    cl_event event;
    error = clEnqueueNDRangeKernel(queue, kernel,
        1, // One dimension, because workSize has dimension one
        NULL,
        workSize,
        NULL,
        0, NULL, &event);
    checkError(error);
    return event;   
}
    
void readBufferBlocking(cl_command_queue queue, cl_mem remoteBuf, int bytes, void* localBuf)
{
    cl_int error;
    error = clEnqueueReadBuffer(queue, remoteBuf, CL_TRUE, 0, 
        (size_t)bytes, localBuf, 0, NULL, NULL);
    checkError(error);
}


void writeBufferBlocking(cl_command_queue queue, cl_mem buffer, int bytes, void* data)
{
    cl_int error;
    error = clEnqueueWriteBuffer(queue, buffer, CL_TRUE,
        0, (size_t)bytes, data, 0, NULL, NULL);
    checkError(error);	
}


