/*{-# RayTracer by AlexD2580 -- where no Harukas were harmed! #-}*/

/*
                PUBLISHED UNDER THE DWTFYWWI-LICENSE
*/


/*
Object:
type :: int
material :: int
color :: float
object :: ?


Concave PolygonN:
n :: int
vertices :: vertex[n]

Vertex :
pos :: float3

Sphere:
data...

Quad:
*/

#define TRIANGLE 1
#define SPHERE 2

//Surface types
#define NONE 0
#define MATT 1 //daemon
#define METALLIC 2
#define MIRROR 3
#define GLASS 4

float3 reflect(float3 source, float3 normal)
{
    float3 axis = normalize(normal);
    float3 ax_dir = axis * dot(source, axis);
    return(source - 2*ax_dir);
}

char isLit(global float* light, float3 pos)
{
    return 0;
}

/*void traceTriangle //this doesn't work... now it does! a LOT faster
(
    private float3 eyePos,
    private float3 eyeDir, //normalized
    global float* triangle,
    private float* depth,
    private float* res
)
{
    uchar n = 3;

    float3 a = (float3){triangle[0], triangle[1], triangle[2]};
    float3 b;
    float3 toA = a - eyePos;
    float3 toB;
    float3 crossV;
    global float* tmp;

    for(uchar i=1; i<n; i++)
    {
        tmp = triangle + i*vertex_size;
        b = (float3){tmp[0], tmp[1], tmp[2]};
        toB = b - eyePos;
        crossV = cross(toB, toA);
        if(dot(eyeDir, crossV) <= 0)
            return; //point is outside, drop;
        toA = toB;
    }

    toA = a - eyePos;
    crossV = cross(toA, toB);
    if(dot(eyeDir, crossV) <= 0)
        return; //point is outside, drop;

    tmp = triangle + vertex_size;
    float3 toC = (float3){tmp[0], tmp[1], tmp[2]} - a; //a to v1
    toB = b - a; //a to vn

    float3 normal = normalize(cross(toC, toB));

    float d = -dot(a, normal);
    float t = -( dot(eyePos, normal) + d ) / dot(eyeDir, normal);

    if(t > *depth || t <= 0)
        return; //there was a closer point before, drop;

    float3 planePos = eyePos + t * eyeDir;

    //--------------------

    res[0] = planePos.x;
    res[1] = planePos.y;
    res[2] = planePos.z;
    res[3] = normal.x;
    res[4] = normal.y;
    res[5] = normal.z;

    *depth = t;
    return;
}*/

void traceSphere
(
    private float3 eyePos,
    private float3 eyeDir, //normalized
    global float* sphere,
    private float* depth,
    private float3* res
)
{
    float3 center = (float3){sphere[0], sphere[1], sphere[2]};
    float radius = sphere[3];

    float3 eyeToSphere = center-eyePos;
    /* View axis distance */
    float dX1 = dot(eyeDir, eyeToSphere);
    if(dX1 <= 0)
      return;

    /* SqrDistance to center of the sphere */
    float dH2 = dot(eyeToSphere, eyeToSphere);

    /* SqrOffset from view axis */
    float dY2 = dH2 - dX1*dX1;

    /* Offset is greater than radius */
    float oX2 = radius*radius - dY2;
    if(oX2 < 0)
        return;

    /* Now dX1 is the distance to the intersection */
    dX1 -= sqrt(oX2);
    /* Intersection is not visible */
    if(dX1 > *depth)
        return;

    res[0] = eyePos + dX1*eyeDir;
    res[1] = normalize(res[0] - center);
    *depth = dX1;
    return;
}

global uchar* runTraceObjects
(
    private float3 eyePos,
    private float3 eyeDir, //normalized
    global float* objects,
    global int* offsets,
    private float* depth,
    private float3* res
)
{
    private int offset;
    global uchar* closest = 0;
    global float* object;
    private uchar type;
    private float dth;
    private float frag = 1.0f;

    for(int i=0; i<1000; i++)
    {
        offset = offsets[i];
        if(offset == -1)
            return closest; //-1 indicates last object

        object = objects + offset;
        type = ((global uchar*)object)[0];

        dth = *depth;

        switch(type)
        {
        case TRIANGLE: // +1 skip header
            //traceTriangle(eyePos, eyeDir, object+1, depth, res);
            break;
        case SPHERE:
            traceSphere(eyePos, eyeDir, object+1, depth, res);
            break;
        default:
            break;
        }

        if(*depth < dth)
        {
            dth = *depth;
            closest = (global uchar*)object;
        }
    }
    return closest;
}

/**
 * Returns the frag-value for the given ray
 */
float traceObjects
(
    private float3 eyePos,
    private float3 eyeDir, //normalized
    global uchar* objects,
    global int* offsets,
    private float* depth,
    private float3* res
)
{
    global float* objects_f = (global float*)objects;

    global uchar* closest =
      runTraceObjects(eyePos, eyeDir, objects_f, offsets, depth, res);

    private int itr = 0;
    private float frag = 1.0f;
    private float dth;
    private float3 pos;
    private float3 normal;

    private uchar type;
    private uchar material;
    private float color;

    while(closest != 0 && itr < 100 && frag > 0.0039f)
    {
        type = closest[0];
        material = closest[1];
        color = (float)closest[2]/255.0f;

        dth = 1.0f/0.0f; //reset

        switch(material)
        {
        case MATT:
            frag *= color;
            closest = 0;
            break;
        case METALLIC:
            frag *= color;
            eyePos = res[0];
            eyeDir = reflect(eyeDir, res[1]);
            closest =
              runTraceObjects(eyePos, eyeDir, objects_f, offsets, &dth, res);
            break;
        case MIRROR:
            eyePos = res[0];
            eyeDir = reflect(eyeDir, res[1]);
            closest =
              runTraceObjects(eyePos, eyeDir, objects_f, offsets, &dth, res);
            break;
        case GLASS:
            break;
        default:
            break;
        }

        itr++;
    }

    return frag;
}

/*
fov should be an array of floats:
fovy, aspect,
posx, posy, posz,
dirx, diry, dirz,
upx, upy, upz,
leftx, lefty, leftz,
size_x, size_y <-- OMG IT'S TWO INTS!!
*/

kernel void trace //main
(
    global int* startID,
    global float* fov,
    global uchar* objects,
    global int* offsets,
    global uchar* frameBuf,
    global float* depthBuf,
    global char* lights
)
{
    const int id = *startID + get_global_id(0);

    private float fovy = fov[0];
    private float aspect = fov[1];

    private float3 eyePos = (float3){ fov[2], fov[3], fov[4] };
    private float3 eyeDir = (float3){ fov[5], fov[6], fov[7] };
    private float3 eyeUp = (float3){ fov[8], fov[9], fov[10] };
    private float3 eyeLeft = (float3){ fov[11], fov[12], fov[13] };
    private int size_x = ((global int*)(fov+14))[0];
    private int size_y = ((global int*)(fov+14))[1];
    //don't. think. about. it.
    private int pos_x = id % size_x;
    private int pos_y = id / size_x;

    //x on screen == x in coordsystem
    private float rel_x = (2.0f * (float)pos_x / (float)size_x) - 1.0f;
    //y on screen goes down, y in coordsys goes up -> invert
    private float rel_y = (2.0f * (float)-pos_y / (float)size_y) + 1.0f;

    private float max_u = tan(fovy/2.0f);
    private float max_r = max_u*aspect;

    eyeDir += (rel_y*max_u*eyeUp - rel_x*max_r*eyeLeft);
    eyeDir = normalize(eyeDir); // TODO FIX THIS!!

    private float frag;
    private float depth = depthBuf[id];
    private float3 res[2];

    //------------------------------------------------------------------------//
      frag = traceObjects(eyePos, eyeDir, objects, offsets, &depth, res);
    //------------------------------------------------------------------------//

    float3 toSun = (float3){ 0.0f, 1.0f, 0.0f };
    if(depth >= depth + 10.0f) // depth = inf -> no collision
    {
        float amb = 0.2f;
        float diff = 0.8f * clamp(dot(eyeDir, toSun), 0.0f, 1.0f);
        frag *= amb+diff;
        frag = 0.0;
    }
    else // depth < inf
    {
        float3 pos = res[0];
        float3 normal = res[1];

        float amb = 0.2f;
        float diff = 0.0f;

        float cos_t = dot(normal, toSun);
        if(cos_t > -0.001)
        {
          float depthL = 1.0f/0.0f;
          /*traceObjects( //TODO REENABLE
            pos + 0.0001f*normal,
            toSun,
            objects,
            offsets,
            &depthL,
            res
            );*/
          if(depthL >= depthL + 10.0f) //no object blocking
          {
            diff = 0.8f * clamp(cos_t, 0.0f, 1.0f);
            printf("LIT\n");
          }
        }
        printf("HIT: %f\t%f\t%f\n", frag, cos_t, diff);

        frag *= amb+diff;
    }

    frameBuf[id] = (uchar)floor(255.1f*frag);
    printf("ANY: %d %d %d => %f\t%f %f %f\n", pos_x, pos_y, id, frag, eyeDir.x, eyeDir.y, eyeDir.z);
    depthBuf[id] = depth;
    return;
}
