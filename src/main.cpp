#include <iostream>
#include <cmath>
#include <glm/glm.hpp>
#include <string>

#define __USE_BSD // to get usleep
#include <unistd.h>

/*struct timespec abstime;
clock_gettime(CLOCK_REALTIME, &abstime);
abstime.tv_sec += cooldown;
pthread_cond_timedwait(&notifier, &mutex, &abstime);*/

#include "scene_helper.hpp"
#include "cl_helper.hpp"
#include "sdl.hpp"

using namespace std;
using namespace OpenCL;

#define F_PI ((float)M_PI)

void prepare_view(int const size_w, int const size_h)
{
  glm::vec3 cam_pos(-0.3f, 1.2f, 0.0f);
  glm::vec3 cam_dir(0.5f, -0.2, -1);
  glm::vec3 cam_up(0.0f, 1.0f, 0.0f);

  float fov = F_PI / 4.f;

  cam_dir = glm::normalize(cam_dir);
  cam_up = glm::normalize(cam_up);
  glm::vec3 cam_left = glm::cross(cam_up, cam_dir);
  cam_up = glm::cross(cam_dir, cam_left);

  /*
  fov should be an array of floats:
  fovy, aspect,
  posx, posy, posz,
  dirx, diry, dirz,
  upx, upy, upz,
  -- add leftx, lefty, leftz,
  size_x, size_y <-- IT'S TWO INTS!!
  */
  float data[16];
  data[0] = fov;
  data[1] = float(size_w) / float(size_h);
  data[2] = cam_pos.x;
  data[3] = cam_pos.y;
  data[4] = cam_pos.z;
  data[5] = cam_dir.x;
  data[6] = cam_dir.y;
  data[7] = cam_dir.z;
  data[8] = cam_up.x;
  data[9] = cam_up.y;
  data[10] = cam_up.z;
  data[11] = cam_left.x;
  data[12] = cam_left.y;
  data[13] = cam_left.z;

  int* data_i = (int*)(data + 14);
  data_i[0] = size_w;
  data_i[1] = size_h;

  CLHelper::write_buffer(CLHelper::fov_mem, data);
}

void create_scene(Scene& scene)
{
  cout << "[Main] Queueing models" << endl;

  Material const red(
      DIFFUSE, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f));
  Material const green(
      DIFFUSE, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f));
  Material const blue(
      DIFFUSE, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f));

  Material const mirror(MIRROR, 0.0f, glm::vec3(1.0f), glm::vec3(0.0f));
  Material const metal(METALLIC, 0.0f, glm::vec3(1.0f), glm::vec3(0.0f));
  Material const lamp(DIFFUSE, 0.0f, glm::vec3(1.0f), glm::vec3(5.0f));

  Material const white(DIFFUSE, 0.0f, glm::vec3(1.0f), glm::vec3(0.0f));

  scene.push_matrix();
  scene.translate(1.5f, 1.5f, -1.5f);
  room(scene, 3.f, 3.f, 3.f, white);
  scene.pop_matrix();

  scene.push_matrix();
  scene.translate(2.8f, 2.8f, -1.5f);
  box(scene, 10.4f, 0.4f, 0.4f, lamp);
  scene.pop_matrix();

  scene.push_matrix();
  scene.translate(0.2f, 0.0f, -3.f + 0.85f);
  Table::render(scene);
  scene.pop_matrix();
}

int main(void)
{
  cout << "[Main] Launched." << endl;

  string tracer("./cl/ray_frag.cl");
  string tracer_main("trace");
  OpenCL::Kernel path_tracer(tracer, tracer_main);

  unsigned int const size_w = 1;
  unsigned int const size_h = 1;
  uint32_t* frame_buffer =
      (uint32_t*)alloca(size_w * size_h * sizeof(uint32_t));

  if(SDL::init(size_w, size_h) != 0)
  {
    cerr << "[Main] SDL initialization failed." << endl;
    return 1;
  }

  float samples = 0.0f;

  try
  {
    size_t const primitive_size = 500 * 16;
    size_t const octree_size = 3000; // TODO find better approximations!

    CLHelper::init(size_w, size_h, path_tracer, primitive_size, octree_size);

    AABB aabb(glm::vec3(-10.0f), glm::vec3(10.0f));

    Scene scene(size_w, size_h, primitive_size, aabb);
    scene.printInfo();

    prepare_view(size_w, size_h);
    create_scene(scene);
    CLHelper::push_scene(scene);

    while(!SDL::die)
    {
      SDL::handleEvents();
      samples++;
      CLHelper::write_buffer(CLHelper::samples_mem, &samples);
      CLHelper::render(path_tracer);
      CLHelper::read_buffer(CLHelper::frame_c_mem, frame_buffer);
      SDL::drawFrame(frame_buffer);
      cout << "[Main] Samples: " << (int)samples << endl;
      cout.flush();
    }

    CLHelper::close();
  }
  catch(OpenCLException& e)
  {
    e.print();
  }

  SDL::close();
  cout << "[Main] Closed." << endl;
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
