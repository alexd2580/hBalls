#include<iostream>
#include<cmath>
#include<glm/glm.hpp>
#include<string>

#define __USE_BSD	//to get usleep
#include<unistd.h>

/*struct timespec abstime;
clock_gettime(CLOCK_REALTIME, &abstime);
abstime.tv_sec += cooldown;
pthread_cond_timedwait(&notifier, &mutex, &abstime);*/

#include"scene.hpp"
#include"cl_helper.hpp"
#include"sdl.hpp"

using namespace std;

#define F_PI ((float)M_PI)

void prepareView(void)
{
  glm::vec3 camPos(50.0f, 50.0f, 50.0f);
  glm::vec3 camDir(-1.0f, -1.0f, -1.0f);
  glm::vec3 camUp(0.0f, 1.0f, 0.0f);

  float fov = F_PI/2.0f;

  camDir = glm::normalize(camDir);
  camUp = glm::normalize(camUp);
  glm::vec3 camLeft = glm::cross(camUp, camDir);
  camUp = glm::cross(camDir, camLeft);

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
  data[1] = float(Scene::size_w) / float(Scene::size_h);
  data[2] = camPos.x;
  data[3] = camPos.y;
  data[4] = camPos.z;
  data[5] = camDir.x;
  data[6] = camDir.y;
  data[7] = camDir.z;
  data[8] = camUp.x;
  data[9] = camUp.y;
  data[10] = camUp.z;
  data[11] = camLeft.x;
  data[12] = camLeft.y;
  data[13] = camLeft.z;

  int* data_i = (int*)(data+14);
  data_i[0] = Scene::size_w;
  data_i[1] = Scene::size_h;

  OpenCL::writeBufferBlocking(CLHelper::queue, CLHelper::fov_mem, 16*FLOAT_SIZE, data);
}

void scene(void)
{
  cout << "[Main] Queueing models" << endl;

  /*Scene::quad(
    DIFFUSE,
    glm::vec3(1.0f, 1.0f, 1.0f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(-20.0, -20.0, 0.0),
    glm::vec3(20.0, -20.0, 0.0),
    glm::vec3(20.0, 20.0, 0.0),
    glm::vec3(-20.0, 20.0, 0.0)
  );*/

  Scene::push_matrix();

  glm::vec3 a(-20.0, 0.0, 0.0);
  glm::vec3 b(20.0, 0.0, 0.0);
  glm::vec3 c(0.0, 0.0, -10.0*sqrt(12.0));
  glm::vec3 d(0.0, 0.0, 0.0);

  glm::vec3 x = c / 3.0f;
  d.z = x.z;

  //set center to x|y=20;
  Scene::translatef(-x.x, 20.0f, -x.z);

  x -= a;
  float h = glm::length(x);
  h = (float)sqrt((2.0f*20.0f)*(2.0f*20.0f) - h*h);
  d.y = h;

  Scene::sphere(
    DIFFUSE,
    glm::vec3(0.0f, 0.0f, 1.0f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    a,
    20.0f
  );

  Scene::sphere(
    DIFFUSE,
    glm::vec3(0.0f, 1.0f, 0.0f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    b,
    20.0f
  );

  Scene::sphere(
    DIFFUSE,
    glm::vec3(1.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    c,
    20.0f
  );

  Scene::sphere(
    DIFFUSE,
    glm::vec3(1.0f, 1.0f, 1.0f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    d,
    20.0f
  );

  Scene::pop_matrix();

  Scene::quad(
    DIFFUSE,
    glm::vec3(1.0f, 1.0f, 1.0f),
    glm::vec3(0.5f, 0.5f, 0.5f),
    glm::vec3(-10000.0, -100.0, -10000.0),
    glm::vec3(-10000.0, -100.0, 10000.0),
    glm::vec3(10000.0, -100.0, 10000.0),
    glm::vec3(10000.0, -100.0, -10000.0)
  );
}


int main(void)
{
    cout << "[Main] Launched." << endl;

    string kernel("./cl/ray_frag.cl");
    string cleaner("./cl/clearBuffers.cl");
    string image("png/out.png");

    unsigned int size_w = 1000;
    unsigned int size_h = 1000;

    uint32_t* frame_buffer = (uint32_t*)alloca(size_w*size_h*sizeof(uint32_t));

    if(SDL::init(size_w, size_h) != 0)
    {
      cerr << "SDL initialization failed." << endl;
      return 1;
    }

    float samples = 0.0f;

    try
    {
      OpenCL ocl;
      ocl.init();
      CLHelper::init(size_w, size_h, kernel, cleaner, ocl);
      Scene::init(size_w, size_h);
      Scene::printInfo();

      prepareView();
      scene();
      CLHelper::pushScene();

      while(!SDL::die)
      {
        SDL::handleEvents();
        CLHelper::clearBuffers(ocl);
        samples++;
        OpenCL::writeBufferBlocking(CLHelper::queue, CLHelper::samples_mem,
          FLOAT_SIZE, &samples);
        CLHelper::render();
        OpenCL::readBufferBlocking(CLHelper::queue, CLHelper::frame_c_mem,
          size_w*size_h*sizeof(uint32_t), frame_buffer);
        //for(int i=0; i<size_w*size_h; i++)
          //cout << frame_buffer[i] << " ";
        //SDL::die = true;
        SDL::drawFrame(frame_buffer);
        //SDL::wait(100);
      }
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
        float camPos[] = {
            75.0f, 75.0f, 75.0f,
            -1.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f
            };

        setPerspective(F_PI/2.0f, camPos);
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
