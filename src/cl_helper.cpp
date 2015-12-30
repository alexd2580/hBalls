
#include<iostream>
#include<vector>
#include"cl_helper.hpp"
#include"scene.hpp"

using namespace std;

namespace CLHelper
{
  /**
   * Buffers on GPU
   */
  cl_mem /*float*/ fov_mem;
  cl_mem /*float*/ objects_mem;
  cl_mem /*char4*/ frameBuf_mem;
  cl_mem /*float*/ depthBuf_mem;

  cl_command_queue queue;
  cl_kernel raytracer;
  cl_kernel bufferCleaner;

  unsigned int size_w;
  unsigned int size_h;

  void clearBuffers(OpenCL& cl)
  {
    cl.enqueue_kernel(size_w*size_h, bufferCleaner, queue);
  }

  void init(unsigned int w, unsigned int h, string& tracepath, string& cleanpath, OpenCL& cl)
  {
    cout << "[CLHelper] Initializing." << endl;

    size_w = w;
    size_h = h;

    frameBuf_mem = cl.allocate_space(size_h*size_w*sizeof(uint32_t), nullptr);
    fov_mem = cl.allocate_space(16*FLOAT_SIZE, nullptr);
    objects_mem = cl.allocate_space(MAX_PRIMITIVES*14*FLOAT_SIZE, nullptr);
    depthBuf_mem = cl.allocate_space(size_w*size_h*FLOAT_SIZE, nullptr);

    queue = cl.createCommandQueue();

    string clearBufferMain("clearBuffers");
    bufferCleaner = cl.load_program(cleanpath, clearBufferMain);
    string tracerMain("trace");
    raytracer = cl.load_program(tracepath, tracerMain);

    OpenCL::setArgument(bufferCleaner, 0, MEM_SIZE, &frameBuf_mem);
    OpenCL::setArgument(bufferCleaner, 1, MEM_SIZE, &depthBuf_mem);

    OpenCL::setArgument(raytracer, 1, MEM_SIZE, &fov_mem);
    OpenCL::setArgument(raytracer, 2, MEM_SIZE, &objects_mem);
    OpenCL::setArgument(raytracer, 3, MEM_SIZE, &frameBuf_mem);
    OpenCL::setArgument(raytracer, 4, MEM_SIZE, &depthBuf_mem);

    clearBuffers(cl);

    cout << "[CLHelper] Done." << endl;
  }

  void close(void)
  {
    cout << "[CLHelper] Closing." << endl;
    clReleaseKernel(bufferCleaner);
    clReleaseKernel(raytracer);

    clReleaseCommandQueue(queue);
    cout << "[CLHelper] Closed." << endl;
  }

  void pushScene(void)
  {
    cout << "[CLHelper] Pushing scene definition." << endl;

    size_t size = Scene::objects_buffer_i - Scene::objects_buffer_start;
    size = (size+1) * FLOAT_SIZE;
    *((uint8_t*)Scene::objects_buffer_i) = END;

    OpenCL::writeBufferBlocking(queue, objects_mem, size, Scene::objects_buffer_start);
  }

  void render(OpenCL& cl)
  {
      cout << "[CLHelper] Rendering." << endl;

      vector<cl_event> event_list;
      cl_mem id_mem;
      int idi;

      for(unsigned int i=0; i<size_h; i++)
      {
          idi = i*size_w;
          id_mem = cl.allocate_space(INT_SIZE, &idi);
          OpenCL::setArgument(raytracer, 0, MEM_SIZE, &id_mem);
          event_list.push_back(OpenCL::enqueue_kernel(size_w, raytracer, queue));
      }

      cl_int status;
      unsigned int queue_size = size_h;
      unsigned int total = size_h;
      unsigned int finished = 0;
      unsigned int running = 0;
      //unsigned int queued = 0;

      while(finished != total)
      {
          running = 0;
          //queued = 0;

          for(unsigned int i=0; i<queue_size; i++)
          {
              status = OpenCL::getEventStatus(event_list[i]);
              switch(status)
              {
              //case CL_QUEUED:
              //case CL_SUBMITTED:
                  break;
              case CL_RUNNING:
                  running++;
                  break;
              case CL_COMPLETE:
                  finished++;
                  queue_size--;
                  event_list[i] = event_list[queue_size];
                  i--;
                  break;
              default:
                  break;
              }
          }

          /*
          cout << "                              \r"
               << "Queued:   " << queued << " / " << size_h << endl;
          cout << "                              \r"
               << "Finished: " << finished << " / " << size_h << " ...";
          cout << "\x1b[1A\r";
          cout.flush();
          usleep(10000);*/
      }
      //cout << endl << endl;

      clFinish(queue);
  }
}
