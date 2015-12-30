/*{-# RayTracer by AlexD2580 -- where no Harukas were harmed! #-}*/

/**** PUBLISHED UNDER THE DWTFYWWI-LICENSE ****/

/**
uint8 type;
uint8 material;
uin16 __padding__
float3 passive;
float3 active;

data...

sphere -> 11*4 byte
triangle -> 16*4 byte
**/

#define END 0
#define TRIANGLE 1
#define SPHERE 2

//Surface types
#define NONE 0
#define DIFFUSE 1
#define METALLIC 2
#define MIRROR 3
#define GLASS 4

float3 reflect(float3 source, float3 normal)
{
    float3 axis = normalize(normal);
    float3 ax_dir = axis * dot(source, axis);
    return(source - 2*ax_dir);
}

void traceTriangle
(
    private float3 eyePos,
    private float3 eyeDir, //normalized
    global float* triangle,
    private float* depth,
    private float3 res[]
)
{
    float3 a = (float3){triangle[0], triangle[1], triangle[2]};
    float3 b = (float3){triangle[3], triangle[4], triangle[5]};
    float3 c = (float3){triangle[6], triangle[7], triangle[8]};
    float3 toA = a - eyePos;
    float3 toB = b - eyePos;
    float3 toC = c - eyePos;

    float3 crossV;
    crossV = cross(toB, toA);
    if(dot(eyeDir, crossV) < 0.0f)
        return;
    crossV = cross(toC, toB);
    if(dot(eyeDir, crossV) < 0.0f)
        return;
    crossV = cross(toA, toC);
    if(dot(eyeDir, crossV) < 0.0f)
        return;

    float3 aToB = b - a;
    float3 aToC = c - a;
    float3 normal = normalize(cross(aToB, aToC));

    float ortho_dist = dot(eyePos-a, normal);
    float ortho_per_step = -dot(eyeDir, normal);

    float dist = ortho_dist / ortho_per_step;

    if(dist > *depth || dist <= 0.0f)
      return; //there was a closer point before, drop;

    float3 planePos = eyePos + dist * eyeDir;

    //--------------------

    res[0] = planePos;
    res[1] = normal;
    *depth = dist;
    return;
}

void traceSphere
(
    private float3 eyePos,
    private float3 eyeDir, //normalized
    global float* sphere,
    private float* depth,
    private float3 res[]
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

global float* runTraceObjects
(
    private float3 eyePos,
    private float3 eyeDir, //normalized
    global float* objects,
    private float* depth,
    private float3 res[]
)
{
    global float* closest = 0;
    private uchar type = ((global uchar*)objects)[0];
    private float dth;

    while(type != END) // type 0 is END
    {
        dth = *depth;

        switch(type)
        {
        case TRIANGLE: // +7 skip header
            traceTriangle(eyePos, eyeDir, objects+7, depth, res);
            break;
        case SPHERE:
            traceSphere(eyePos, eyeDir, objects+7, depth, res);
            break;
        default:
            break;
        }

        if(*depth < dth)
        {
            dth = *depth;
            closest = objects;
        }

        // go to next
        switch(type)
        {
        case TRIANGLE:
          objects += 7 + 9;
          break;
        case SPHERE:
          objects += 7 + 4;
          break;
        }

        type = ((global uchar*)objects)[0];
    }
    return closest;
}

float3 traceObjects
(
    private float3 eyePos,
    private float3 eyeDir, //normalized
    global float* objects,
    private float* depth,
    private float3 res[]
)
{
    global float* closest =
      runTraceObjects(eyePos, eyeDir, objects, depth, res);

    private float3 pos;
    private float3 normal;

    private uchar material;
    private float3 passive;
    private float3 active;

    private float3 brdf = (float3){ 1.0f, 1.0f, 1.0f };
    private float3 frag = (float3){ 0.0f, 0.0f, 0.0f };

    for(private int itr=0; closest != 0 && itr < 100; itr++)
    {
        material = ((global uchar*)closest)[1];
        passive = (float3){ closest[1], closest[2], closest[3] };
        active = (float3){ closest[4], closest[5], closest[6] };
        pos = res[0];
        normal = res[1];

        switch(material)
        {
        case DIFFUSE:
        default:
            eyePos = pos;
            eyeDir = reflect(eyeDir, normal); // TODO random reflect
            break;
        /*case METALLIC:
            eyePos = pos;
            eyeDir = reflect(eyeDir, normal);
            break;
        case MIRROR:
            eyePos = pos;
            eyeDir = reflect(eyeDir, normal);
            break;
        case GLASS:
            break;
        default:
            break;*/
        }

        frag += active * brdf;
        brdf *= 2.0f * passive * dot(normal, eyeDir);

        private float dth = 1.0f/0.0f; //reset
        closest = runTraceObjects(eyePos, eyeDir, objects, &dth, res);
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
    global float* objects,
    global uint* frameBuf,
    global float* depthBuf
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

    private float3 frag;
    private float depth = depthBuf[id];
    private float3 res[2];

    //------------------------------------------------------------------------//
      frag = traceObjects(eyePos, eyeDir, objects, &depth, res);
    //------------------------------------------------------------------------//

    uchar frag_r = (uchar)clamp(255.1f*frag.x, 0.0f, 255.0f);
    uchar frag_g = (uchar)clamp(255.1f*frag.y, 0.0f, 255.0f);
    uchar frag_b = (uchar)clamp(255.1f*frag.z, 0.0f, 255.0f);
    uint frag_i = frag_r << 24 | frag_g << 16 | frag_b << 8 | 255;

    frameBuf[id] = frag_i;
    depthBuf[id] = depth;
    return;
}
