#ifndef __CL_OCGL_H__
#define __CL_OCGL_H__

#include<CL/cl.h>

#include<stdio.h> 
#include<stdlib.h>

//gcc -std=c99 openCLTest.c -o openCLTest -lOpenCL

/*
by remote i mean the GPU/CPU-internal memory
and local is userspace
*/

/**
 * Inits platform ids
 * Inits device ids from platform 0
 */
int setupCL(void);

/**
 * Frees platform ids
 * Frees device ids
 */
void closeCL(void);

/**
 * Creates a remote buffer of byte_size bytes size
 * and fills it with bytes
 */
cl_mem allocateSpace(int byte_size, void* bytes);

/**
 * Loads a program and compiles it to a kernel
 * main function is identified by mainFunction 
 */
cl_kernel loadProgram(const char* fpath, const char* mainFunction);

/**
 * Blocks until the given event has been completed.
 */
void waitForEvent(cl_event e);

/**
 * Returns the statuc code of the event. non-blocking.
 */
cl_int getEventStatus(cl_event e);

/**
 * the argunemt ar nr.th element to the kernel
 */
void setArgument(cl_kernel kernel, int nr, int arg_size, void* arg);

/**
 * Starts a command queue.
 * 
 */
cl_command_queue createCommandQueue(void);

/**
 * Put kernel(action) into queue
 * Returns an event by which the computation can be identified
 */
cl_event enqueueKernelInQueue(int width, cl_kernel kernel, cl_command_queue queue);

/**
 * Input:
 * queue -> eventqueue
 * buffer -> remote buffer to write to
 * bytes -> number of bytes
 * data -> data*
 */
void writeBufferBlocking(cl_command_queue queue, cl_mem buffer, int bytes, void* data);

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
void readBufferBlocking(cl_command_queue queue, cl_mem remoteBuf, int bytes, void* localBuf);

#endif
