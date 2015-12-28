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

#include"raycg.hpp"

using namespace std;

#define F_PI ((float)M_PI)

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

    /*
    fov should be an array of floats:
    fovy, aspect,
    posx, posy, posz,
    dirx, diry, dirz,
    upx, upy, upz,
    */

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

#include<ctime>

int main(void)
{
    setvbuf(stdout, nullptr, _IONBF, 0);

    string kernel("./cl/ray_frag.cl");
    string image("png/out.png");

    OpenCL ocl;
    setup(10, 10, kernel, ocl);
    printInfo();

    float camPos[] = {
        0.0f, 100.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
        };

    setPerspective(F_PI/2.0f, camPos);

    cout << "Queueing models" << endl;
    colori(200);
    materiali(MATT);

    /*set_mode(QUAD);
    vertexf(-50.0, 0.0, 0.0);
    vertexf(50.0, 0.0, 0.0);
    vertexf(50.0, 0.0, -100.0);
    vertexf(-50.0, 0.0, -100.0);*/

    set_mode(SPHERE);
    spheref(0.0f,0.0f,0.0f,50.0f);

    /*materiali(MATT);
    renderFloor();
    materiali(MIRROR);
    renderWalls();*/

/*

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

    cout << "Rendering" << endl;
    printScreen(image, ocl);
    return 0;
}
