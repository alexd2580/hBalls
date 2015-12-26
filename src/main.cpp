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
    begin(POLYGON);
        //BACK
        vertexf(s, s, -s);
        vertexf(s, -s, -s);
        vertexf(-s, -s, -s);
        vertexf(-s, s, -s);
    end();
    begin(POLYGON);
        //FRONT
        vertexf(s, s, s);
        vertexf(-s, s, s);
        vertexf(-s, -s, s);
        vertexf(s, -s, s);
    end();
    begin(POLYGON);
        //TOP
        vertexf(s, s, s);
        vertexf(s, s, -s);
        vertexf(-s, s, -s);
        vertexf(-s, s, s);
    end();
    begin(POLYGON);
        //BOTTOM
        vertexf(s, -s, s);
        vertexf(-s, -s, s);
        vertexf(-s, -s, -s);
        vertexf(s, -s, -s);
    end();
    begin(POLYGON);
        //LEFT
        vertexf(-s, s, s);
        vertexf(-s, s, -s);
        vertexf(-s, -s, -s);
        vertexf(-s, -s, s);
    end();
    begin(POLYGON);
        //RIGHT
        vertexf(s, s, s);
        vertexf(s, -s, s);
        vertexf(s, -s, -s);
        vertexf(s, s, -s);
    end();
}

void renderFloor(void)
{
    for(int i=-10; i<10; i++)
    {
        for(int j=-10+(i+110)%2; j<10; j+=2)
        {
            push_matrix();
            translatef((float)i*10.0f, 0.0f, (float)j*10.0f);
            begin(POLYGON);
                vertexf(0.0f, 0.0f, 0.0f);
                vertexf(0.0f, 0.0f, 10.0f);
                vertexf(10.0f, 0.0f, 10.0f);
                vertexf(10.0f, 0.0f, 0.0f);
            end();
            pop_matrix();
        }
    }
}

int main(void)
{
    string kernel("./cl/ray_frag.cl");
    string image("png/out.png");

    //setup(800, 600, "./ray_marcher.cl");
    //setup(4096, 4096);
    OpenCL ocl;
    setup(1000, 1000, kernel, ocl);
    printInfo();

    /*
    fov should be an array of floats:
    fovy, aspect,
    posx, posy, posz,
    dirx, diry, dirz,
    upx, upy, upz,
    */
    /*float camPos[] = {
        0.0f, 75.0f, 200.0f,
        0.0f, -0.25f, -1.0f,
        0.0f, 1.0f, 0.0f
        };*/
    float camPos[] = {
        75.0f, 75.0f, 75.0f,
        -1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f
        };

    setPerspective(F_PI/2.0f, camPos);
    //rotatef(-PIE/5.0f, 0.0f, 1.0f, 0.0f);

    cerr << "Start..." << endl;
    cerr << "Queueing models" << endl;

    colori(200);
    materiali(MATT);
    renderFloor();

    colori(200);
    materiali(METALLIC);
    begin(POLYGON);
        vertexf(-100.0f, 0.0f, -100.0f);
        vertexf(100.0f, 0.0f, -100.0f);
        vertexf(100.0f, 100.0f, -100.0f);
        vertexf(-100.0f, 100.0f, -100.0f);
    end();

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
    end();

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
        vertexf(20.0f,0.0f,0.0f);
    end();

    begin(SPHERE);
        vertexv(b);
        vertexf(20.0f,0.0f,0.0f);
    end();

    begin(SPHERE);
        vertexv(c);
        vertexf(20.0f,0.0f,0);
    end();

    begin(SPHERE);
        vertexv(d);
        vertexf(20.0f,0.0f,0);
    end();
    pop_matrix();

    cerr << "Rendering..." << endl;

    printScreen(image, ocl);

    cerr << "Closing." << endl;
}
