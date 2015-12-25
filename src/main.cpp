#include<iostream>
#include<cmath>

#define __USE_BSD	//to get usleep
#include<unistd.h>

/*struct timespec abstime;
clock_gettime(CLOCK_REALTIME, &abstime);
abstime.tv_sec += cooldown;
pthread_cond_timedwait(&notifier, &mutex, &abstime);*/

#include"raycg.hpp"
#include"data.hpp"

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

/*void donut(float r)
{
    vec3 a,b,c,d;
    vec3 up = (vec3){ 0.0f, 1.0f, 0.0f };
    vec3 dir = (vec3){ 0.0f, 0.0f, 1.0f };
    vec3 vel;



    for(int i=0; i<100; i++)
    {
        ra = i*2*PIE / 100.0f;
        cra = cos(ra);
        sra = sin(ra);

        pos.x = cra*radu.x + sra*radv.x;
        pos.y = cra*radu.y + sra*radv.y;
        pos.z = cra*radu.z + sra*radv.z;

        for(int j=0; j<100; j++)
        {
            pa = j*2*PIE / 100;
            cpa = cos(pa);
            spa = sin(pa);

            normal.x = cpa*pos.x + spa*dir.x;
            normal.y = cpa*pos.y + spa*dir.y;
            normal.z = cpa*pos.z + spa*dir.z;

            x = 16*pos.x + 8*normal.x;
            y = 16*pos.y + 8*normal.y;
            z = 16*pos.z + 8*normal.z;

            a = dotProd(&toEyes, &normal);
            a = a < 0 ? 0 : a;

            sx = SX/2+(int)floor(x*1.5);
            sy = SY/2+(int)floor(y*1.5/2.0f);

            if(depth[sx][sy] > z)
            {
                depth[sx][sy] = z;
                field[sx][sy] = lts[(int)floor(a*11.1f)];
            }
        }
    }
}*/

void renderFloor(void)
{
    for(int i=-10; i<10; i++)
    {
        for(int j=-10+(i+110)%2; j<10; j+=2)
        {
            pushMatrix();
            translatef((float)i*10.0f, 0.0f, (float)j*10.0f);
            begin(POLYGON);
                vertexf(0.0f, 0.0f, 0.0f);
                vertexf(0.0f, 0.0f, 10.0f);
                vertexf(10.0f, 0.0f, 10.0f);
                vertexf(10.0f, 0.0f, 0.0f);
            end();
            popMatrix();
        }
    }
}

int main(void)
{
    //setup(800, 600, "./ray_marcher.cl");
    //setup(4096, 4096);
    setup(14000, 14000, "./ray_frag.cl");
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

    setPerspective(PIE/2.0f, camPos);
    //rotatef(-PIE/5.0f, 0.0f, 1.0f, 0.0f);

    printf("Start...\n");
    printf("Queueing models\n");
    fflush(stdout);

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

    pushMatrix();
    rotatef(-PIE/3.0f, 0.0f, 1.0f, 0.0f);

    vec3 a = (vec3){ -20.0f, 0.0f, 0.0f };
    vec3 b = (vec3){ 20.0f, 0.0f, 0.0f };
    vec3 c = (vec3){ 0.0f, 0.0f, -10.0f*(float)sqrt(12.0) };
    vec3 d = (vec3){ 0.0f, 0.0f, 0.0f };
    vec3 x = (vec3){ 0.0f, 0.0f, 0.0f };

    add3(&x, &a);
    add3(&x, &b);
    add3(&x, &c);
    multiply3(&x, 1.0f/3.0f);
    d.x = x.x;
    d.z = x.z;

    //set center to x|y=20;
    translatef(-x.x, 20.0f, -x.z);

    subtract3(&x, &a);
    float h = length3(&x);
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
    popMatrix();

    printf("Rendering...\n");
    fflush(stdout);

    printScreen("./output.png");
    teardown();

    printf("Closing.\n");
    fflush(stdout);
}
