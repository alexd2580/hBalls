#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>
#include <string>

#define __USE_BSD // to get usleep
#include <unistd.h>

#include "cl.hpp"
#include "scene_helper.hpp"
#include "sdl.hpp"

using namespace std;
using namespace OpenCL;

void push_camera(cl::CommandQueue const& queue,
                 Camera const& camera,
                 int const size_w,
                 int const size_h,
                 unsigned int const object_count,
                 RemoteBuffer const& data_mem)
{
  /**
   * Main kernel function.
   * general_data is an array of 17 4-byte units:
   * fovy, aspect :: Float
   * posx, posy, posz :: Float
   * dirx, diry, dirz :: Float
   * upx, upy, upz :: Float
   * leftx, lefty, leftz :: Float
   * size_w, size_h :: Int
   * num_primitives :: Int
   */
  float data[17];
  data[0] = camera.fov;
  data[1] = float(size_w) / float(size_h);
  data[2] = camera.pos.x;
  data[3] = camera.pos.y;
  data[4] = camera.pos.z;
  data[5] = camera.dir.x;
  data[6] = camera.dir.y;
  data[7] = camera.dir.z;
  data[8] = camera.up.x;
  data[9] = camera.up.y;
  data[10] = camera.up.z;
  data[11] = camera.left.x;
  data[12] = camera.left.y;
  data[13] = camera.left.z;

  int* data_i = (int*)(data + 14);
  data_i[0] = size_w;
  data_i[1] = size_h;
  data_i[2] = object_count;

  writeBufferBlocking(queue, data_mem, data);
}

void push_data(cl::CommandQueue const& queue,
               Scene const& scene,
               RemoteBuffer const& surfs_mem,
               RemoteBuffer const& lamps_mem)
{
  size_t size = scene.m_surf_buffer.index * sizeof(float);
  (void)size;
  // TODO fix writeBuffer, so that is doesn't copy EVERYTHING
  writeBufferBlocking(queue, surfs_mem, scene.m_surf_buffer.buffer);
  writeBufferBlocking(queue, lamps_mem, scene.m_lamp_buffer.buffer);
}

void create_scene(Scene& scene)
{
  cout << "[Main] Queueing models." << endl;

  Material const red(SurfaceType::diffuse, 1.0f, 0.0f, {1.0f, 0.0f, 0.0f});
  Material const green(SurfaceType::diffuse, 1.0f, 0.0f, {0.0f, 1.0f, 0.0f});
  Material const blue(SurfaceType::diffuse, 1.0f, 0.0f, {0.0f, 0.0f, 1.0f});

  Material const mirror(SurfaceType::mirror, 0.0f, 0.0f, glm::vec3(1.0f));
  Material const metal(SurfaceType::metallic, 0.2f, 0.0f, glm::vec3(1.0f));
  Material const lamp(SurfaceType::diffuse, 0.0f, 30.0f, glm::vec3(1.0f));

  Material const white(SurfaceType::diffuse, 1.0f, 0.0f, glm::vec3(1.0f));

  scene.push_matrix();
  scene.translate(1.5f, 1.5f, -1.5f);
  room(scene, 3.f, 3.f, 3.f, white);
  scene.pop_matrix();

  scene.push_matrix();
  scene.translate(2.8f, 2.8f, -1.5f);
  box(scene, 0.4f, 0.4f, 0.4f, lamp);
  scene.pop_matrix();

  scene.push_matrix();
  scene.translate(0.2f, 0.0f, -3.f + 0.85f);
  Table::render(scene);
  scene.pop_matrix();

  cout << "[Main] Done." << endl;
}

int main(void)
{
  cout << "[Main] Entry." << endl;

  /** SDL **/
  unsigned int const size_w = 640;
  unsigned int const size_h = 480;
  uint32_t* frame_buffer =
      (uint32_t*)alloca(size_w * size_h * sizeof(uint32_t));

  if(SDL::init(size_w, size_h) != 0)
  {
    cerr << "[Main] SDL initialization failed." << endl;
    return 1;
  }

  try
  {
    /** OpenCL **/
    Environment env;

    /** Kernel **/
    string tracer("./cl/ray_frag2.cl");
    string tracer_main("trace");
    OpenCL::Kernel path_tracer(tracer, tracer_main);

    /** Scene **/
    size_t const lamp_count = 30;
    size_t const surf_count = 150;
    Scene scene(surf_count, lamp_count);
    create_scene(scene);

    /**
     * PRNG
     * adapted from svenstaro's trac0r
     * 01.01.2016 => github.com/svenstaro/trac0r
     */
    unsigned long prng[17];
    for(int i = 0; i < 16; i++)
      prng[i] = (unsigned long)rand() << 32 | (unsigned long)rand();
    prng[16] = 0;

    /** Buffers **/
    RemoteBuffer /*float */ data_mem = env.allocate(17 * sizeof(float));
    RemoteBuffer /*float */ surf_mem =
        env.allocate(surf_count * triangle_size * sizeof(float));
    RemoteBuffer /*float */ lamp_mem =
        env.allocate(lamp_count * triangle_size * sizeof(float));
    RemoteBuffer /*char4 */ frame_c_mem =
        env.allocate(size_h * size_w * sizeof(uint32_t));
    RemoteBuffer /*float4*/ frame_f_mem =
        env.allocate(size_h * size_w * 4 * sizeof(float));
    RemoteBuffer /*float */ samples_mem = env.allocate(sizeof(float));
    RemoteBuffer /*PRNG  */ prng_mem =
        env.allocate(17 * sizeof(unsigned long), prng);

    /** Prepare Kernel **/
    path_tracer.make(env);
    path_tracer.set_argument(0, data_mem);
    path_tracer.set_argument(1, surf_mem);
    path_tracer.set_argument(2, lamp_mem);
    path_tracer.set_argument(3, frame_c_mem);
    path_tracer.set_argument(4, frame_f_mem);
    path_tracer.set_argument(5, samples_mem);
    path_tracer.set_argument(6, prng_mem);

    /** Camera **/
    Camera c;
    c.pos = glm::vec3(-0.3f, 1.2f, 0.0f);
    c.dir = glm::vec3(0.5f, -0.2, -1);
    c.up = glm::vec3(0.0f, 1.0f, 0.0f);

    c.fov = glm::quarter_pi<float>();

    c.dir = glm::normalize(c.dir);
    c.up = glm::normalize(c.up);
    c.left = glm::cross(c.up, c.dir);
    c.up = glm::cross(c.dir, c.left);

    /** CommandQueue **/
    cl::CommandQueue queue = env.create_queue();

    /** Push data to remote buffers **/
    push_camera(queue, c, size_w, size_h, scene.objects_count(), data_mem);
    push_data(queue, scene, objects_mem);

    float samples = 0.0f;
    while(!SDL::die)
    {
      SDL::handleEvents();
      samples++;
      writeBufferBlocking(queue, samples_mem, &samples);
      /** RUN KERNEL **/
      path_tracer.enqueue(size_w, size_h, queue);
      queue.finish();
      /** Read result from char-framebuffer **/
      readBufferBlocking(queue, frame_c_mem, frame_buffer);
      /** Draw it **/
      SDL::drawFrame(frame_buffer);
      cout << "[Main] Samples: " << (int)samples << endl;
      cout.flush();
    }
  }
  catch(OpenCLException& e)
  {
    e.print();
  }

  SDL::close();
  cout << "[Main] Exit." << endl;
  return 0;
}

/*
renderWalls();
colori(200);
materiali(MIRROR);


set_mode(SPHERE);
spheref(0.0f,0.0f,0.0f,50.0f);*/

/*materiali(MATT);
renderFloor();
materiali(MIRROR);

begin(POLYGON);
    vertexf(100.0f, 0.0f, 100.0f);
    vertexf(-100.0f, 0.0f, 100.0f);
    vertexf(-100.0f, 100.0f, 100.0f);
    vertexf(100.0f, 100.0f, 100.0f);
end();

begin(POLYGON);
    vertexf(-100.0f, 0.0f, 100.0f);
    vertexf(-100.0f, 0.0f, -100.0f);
    vertexf(-100.0f, 100.0f, -100.0f);
    vertexf(-100.0f, 100.0f, 100.0f);
end();

begin(POLYGON);
    vertexf(100.0f, 0.0f, -100.0f);
    vertexf(100.0f, 0.0f, 100.0f);
    vertexf(100.0f, 100.0f, 100.0f);
    vertexf(100.0f, 100.0f, -100.0f);
end();

begin(POLYGON);
    vertexf(-100.0f, 100.0f, 100.0f);
    vertexf(-100.0f, 100.0f, -100.0f);
    vertexf(100.0f, 100.0f, -100.0f);
    vertexf(100.0f, 100.0f, 100.0f);
end();*/

/*
    colori(200);
    materiali(MIRROR);

    push_matrix();
    rotatef(-F_PI/3.0f, 0.0f, 1.0f, 0.0f);

    glm::vec3 a(-20.0, 0.0, 0.0);
    glm::vec3 b(20.0, 0.0, 0.0);
    glm::vec3 c(0.0, 0.0, -10.0*sqrt(12.0));
    glm::vec3 d(0.0, 0.0, 0.0);
    glm::vec3 x = a + b + c;

    x /= 3.0;
    d.x = x.x;
    d.z = x.z;

    //set center to x|y=20;
    translatef(-x.x, 20.0f, -x.z);

    x -= a;
    float h = glm::length(x);
    h = (float)sqrt((2.0f*20.0f)*(2.0f*20.0f) - h*h);
    d.y = h;

    begin(SPHERE);
        vertexv(a);
        floatf(20.0f);
    end();

    begin(SPHERE);
        vertexv(b);
        floatf(20.0f);
    end();

    begin(SPHERE);
        vertexv(c);
        floatf(20.0f);
    end();

    begin(SPHERE);
        vertexv(d);
        floatf(20.0f);
    end();
    pop_matrix();*/

/*
    void qube(float size)
    {
        float s = size/2.0f;
        set_mode(QUAD);
            //BACK
        vertexf(s, s, -s);
        vertexf(s, -s, -s);
        vertexf(-s, -s, -s);
        vertexf(-s, s, -s);
        //FRONT
        vertexf(s, s, s);
        vertexf(-s, s, s);
        vertexf(-s, -s, s);
        vertexf(s, -s, s);
        //TOP
        vertexf(s, s, s);
        vertexf(s, s, -s);
        vertexf(-s, s, -s);
        vertexf(-s, s, s);
        //BOTTOM
        vertexf(s, -s, s);
        vertexf(-s, -s, s);
        vertexf(-s, -s, -s);
        vertexf(s, -s, -s);
        //LEFT
        vertexf(-s, s, s);
        vertexf(-s, s, -s);
        vertexf(-s, -s, -s);
        vertexf(-s, -s, s);
        //RIGHT
        vertexf(s, s, s);
        vertexf(s, -s, s);
        vertexf(s, -s, -s);
        vertexf(s, s, -s);
    }

    void renderFloor(void)
    {
        for(int i=-10; i<10; i++)
        {
            for(int j=-10+(i+110)%2; j<10; j+=2)
            {
                push_matrix();
                translatef(float(i)*10.0f, 0.0f, float(j)*10.0f);
                set_mode(QUAD);
                vertexf(0.0f, 0.0f, 0.0f);
                vertexf(0.0f, 0.0f, 10.0f);
                vertexf(10.0f, 0.0f, 10.0f);
                vertexf(10.0f, 0.0f, 0.0f);
                pop_matrix();
            }
        }
    }

    void renderWall(void)
    {
      set_mode(QUAD);
      vertexf(-100.0f, 200.0f, 0.0f);
      vertexf(-100.0f, 0.0f, 0.0f);
      vertexf(100.0f, 0.0f, 0.0f);
      vertexf(100.0f, 200.0f, 0.0f);
    }

    void renderWalls(void)
    {
      push_matrix();
      translatef(0.0, 0.0, -95.0);
      rotatef(0.0, 0.0, 1.0, 0.0);
      renderWall();
      pop_matrix();
      push_matrix();
      translatef(-95.0, 0.0, 0.0);
      rotatef(F_PI/2.0f, 0.0f, 1.0f, 0.0f);
      renderWall();
      pop_matrix();
      push_matrix();
      translatef(0.0, 0.0, 95.0);
      rotatef(F_PI, 0.0f, 1.0f, 0.0f);
      renderWall();
      pop_matrix();
      push_matrix();
      translatef(95.0, 0.0, 0.0);
      rotatef(-F_PI/2.0f, 0.0f, 1.0f, 0.0f);
      renderWall();
      pop_matrix();
    }


    int main2(void)
    {
        string kernel("./cl/ray_frag.cl");
        string image("png/out.png");

        //setup(800, 600, "./ray_marcher.cl");
        //setup(4096, 4096);
        OpenCL ocl;
        setup(1000, 1000, kernel, ocl);
        printInfo();
        cout << "Defining scene" << endl;
*/
/*
fov should be an array of floats:
fovy, aspect,
posx, posy, posz,
dirx, diry, dirz,
upx, upy, upz,
*/
/*
        float cam_pos[] = {
            75.0f, 75.0f, 75.0f,
            -1.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f
            };

        setPerspective(F_PI/2.0f, cam_pos);
        //rotatef(-PIE/5.0f, 0.0f, 1.0f, 0.0f);

        cout << "Start..." << endl;
        cout << "Queueing models" << endl;

        colori(200);
        materiali(MATT);
        renderFloor();

        colori(200);
        materiali(METALLIC);
        set_mode(QUAD);
        vertexf(-100.0f, 0.0f, -100.0f);
        vertexf(100.0f, 0.0f, -100.0f);
        vertexf(100.0f, 100.0f, -100.0f);
        vertexf(-100.0f, 100.0f, -100.0f);

        vertexf(100.0f, 0.0f, 100.0f);
        vertexf(-100.0f, 0.0f, 100.0f);
        vertexf(-100.0f, 100.0f, 100.0f);
        vertexf(100.0f, 100.0f, 100.0f);

        vertexf(-100.0f, 0.0f, 100.0f);
        vertexf(-100.0f, 0.0f, -100.0f);
        vertexf(-100.0f, 100.0f, -100.0f);
        vertexf(-100.0f, 100.0f, 100.0f);

        vertexf(100.0f, 0.0f, -100.0f);
        vertexf(100.0f, 0.0f, 100.0f);
        vertexf(100.0f, 100.0f, 100.0f);
        vertexf(100.0f, 100.0f, -100.0f);

        vertexf(-100.0f, 100.0f, 100.0f);
        vertexf(-100.0f, 100.0f, -100.0f);
        vertexf(100.0f, 100.0f, -100.0f);
        vertexf(100.0f, 100.0f, 100.0f);

        colori(200);
        materiali(MIRROR);

        push_matrix();
        rotatef(-F_PI/3.0f, 0.0f, 1.0f, 0.0f);

        glm::vec3 a(-20.0, 0.0, 0.0);
        glm::vec3 b(20.0, 0.0, 0.0);
        glm::vec3 c(0.0, 0.0, -10.0*sqrt(12.0));
        glm::vec3 d(0.0, 0.0, 0.0);
        glm::vec3 x = a + b + c;

        x /= 3.0;
        d.x = x.x;
        d.z = x.z;

        //set center to x|y=20;
        translatef(-x.x, 20.0f, -x.z);

        x -= a;
        float h = glm::length(x);
        h = (float)sqrt((2.0f*20.0f)*(2.0f*20.0f) - h*h);
        d.y = h;

        set_mode(SPHERE);
        spherev(a, 20);
        spherev(b, 20);
        spherev(c, 20);
        spherev(d, 20);

        pop_matrix();

        cout << "Rendering" << endl;

        printScreen(image, ocl);

        cout << "Closing" << endl;

        return 0;
    }
*/
