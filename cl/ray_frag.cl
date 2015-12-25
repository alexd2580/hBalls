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

#define POLYGON 1
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

float3 getPos(global float* vertex)
{
    return (float3){ vertex[0], vertex[1], vertex[2] };
}

float getRadius(global float* vertex)
{
    return vertex[3];
}

char isLit(global float* light, float3 pos)
{
    return 0;
}

constant int vertex_size = 3*sizeof(float);


/**
 * Checks with a fairly simple algorithm if the specified object is
 * intersected by the given ray. Sound, but NOT complete!
 * If result is 0 -> check is required using traceObject
 */
/*char doesNotIntersect
(
    private float3 eyePos,
    private float3 eyeDir,
    global int* object_i
)
{
    global float* object_f;
    
    float3 pos, v1, v2;
    v1 = eyeDir;
    if(fabs(eyeDir.x) < 0.1f)
        v1.x = 1.0f;
    else
        v1.x = 0.0f;
    v2 = normalize(cross(v1, eyeDir));
    v1 = normalize(cross(v2, eyeDir));
    
    float dirPart, p1, p2;
    
    if(object_i[0] == POLYGON)
    {
        int n = object_i[1];
        object_f = (float*)(object_i+2);
        float min_1, max_1, min_2, max_2;
        min_1 = min_2 = 1.0f/0.0f;
        max_1 = max_2 = -1.0f/0.0f;
        
        char nonInFront = 1;
        for(int i=0; i<n; i++)
        {
            pos = getPos(object_f + i*4);
            pos -= eyePos;
            dirPart = dot(eyeDir, pos);
            if(dirPart > 0.0f)
                nonInFront = 0;
            pos -= dirPart*eyeDir;
            
            p1 = dot(pos, v1);
            p2 = dor(pos, v2);
            
            if(p1 < min_1) min_1 = p1;
            else if(p1 > max_1) max_1 = p1;
            
            if(p2 < min_2) min_2 = p2;
            else if(p2 > max_2) max_2 = p2;
        }
        
        if(nonInFront)
            return 1;
        if(min_1 >= 0 || min_2 >= 0 || max_1 <= 0 || max_2 <= 0)
            return 1;
        return 0;
    }
    else if(object_i[0] == SPHERE)
    {
        object_f = (float*)(object_i+1);
        float3 pos = getPos(object_f);
        float radius = getColor(obejct_f);
    }
    return 0;
}*/

void tracePolygon //this doesn't work... now it does! a LOT faster
(
    private float3 eyePos,
    private float3 eyeDir, //normalized
    global uchar* polygon,
    private float* depth,
    private float* res
)
{
    uchar n = *polygon;
    polygon++;
    
    float3 a = getPos((global float*)polygon);
    float3 b;
    float3 toA = a - eyePos;
    float3 toB;
    float3 crossV;    
    
    for(uchar i=1; i<n; i++)
    {
        b = getPos((global float*)(polygon + i*vertex_size));
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
    
    float3 toC = getPos((global float*)(polygon + vertex_size)) - a; //a to v1
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
}

void traceSphere
(
    private float3 eyePos,
    private float3 eyeDir, //normalized
    global uchar* sphere,
    private float* depth,
    private float* res
)
{
    float3 center = getPos((global float*)sphere);
    
    float3 cToP = center-eyePos; //camera to point
    float dX = dot(eyeDir, cToP);
    if(dX < 0)
        return;
        
    float dH = length(cToP);
    float radius = getRadius((global float*)sphere);
    
    float off = dH*dH - dX*dX;
    off = radius*radius - off;
    if(off < 0)
        return;
    
    dX -= sqrt(off);
    if(dX > *depth)
        return;
    
    cToP = eyePos + dX*eyeDir;
    res[0] = cToP.x;
    res[1] = cToP.y;
    res[2] = cToP.z;
    cToP = normalize(cToP-center);
    res[3] = cToP.x;
    res[4] = cToP.y;
    res[5] = cToP.z;
    *depth = dX;
    return;
}

global uchar* runTraceObjects
(
    private float3 eyePos,
    private float3 eyeDir, //normalized
    global uchar* objects,
    global int* offsets,
    private float* depth,
    private float* res
)
{
    private int offset;
    global uchar* closest = 0;
    global uchar* object;
    private uchar type;
    private float dth;
    private float frag = 1.0f;
        
    for(int i=0; i<1000; i++)
    {
        offset = offsets[i];
        if(offset == -1)
            return closest; //-1 indicates last object
        
        object = objects + offset;
        type = object[0];
        
        dth = *depth;
        
        switch(type)
        {
        case POLYGON:
            tracePolygon(eyePos, eyeDir, object+3, depth, res);
            break;
        case SPHERE:
            traceSphere(eyePos, eyeDir, object+3, depth, res);
            break;
        default:
            break;
        }
        
        if(*depth < dth)
        {    
            dth = *depth;
            closest = object;
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
    private float* res
)
{
    global uchar* closest;

    closest = runTraceObjects(eyePos, eyeDir, objects, 
                offsets, depth, res);
    
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
            eyePos = (float3){ res[0], res[1], res[2] };
            eyeDir = reflect(eyeDir, (float3){ res[3], res[4], res[5] });
            closest = runTraceObjects(eyePos, eyeDir, objects, 
                        offsets, &dth, res);
            break;
        case MIRROR:
            eyePos = (float3){ res[0], res[1], res[2] };
            eyeDir = reflect(eyeDir, (float3){ res[3], res[4], res[5] });
            closest = runTraceObjects(eyePos, eyeDir, objects, 
                        offsets, &dth, res);
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
    private int size_x = ((int*)(fov+14))[0];
    private int size_y = ((int*)(fov+14))[1];
    //don't. think. about. it.
    private int pos_x = id % size_x;
    private int pos_y = id / size_x;
    
    //x on screen == x in coordsystem
    private float rel_x = (2.0f * (float)pos_x / (float)size_x) - 1.0f;
    //y on screen goes down, y in coordsys goes up -> invert
    private float rel_y = (2.0f * (float)-pos_y / (float)size_y) + 1.0f;
    
    float max_u = tan(fovy/2.0f);
    float max_r = max_u*aspect;
    
    eyeDir += (rel_y*max_u*eyeUp - rel_x*max_r*eyeLeft);
    eyeDir = normalize(eyeDir);

    private float frag;
    private float depth = depthBuf[id];
    private float res[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    //------------------------------------------------------------------------//
      frag = traceObjects(eyePos, eyeDir, objects, offsets, &depth, res);
    //------------------------------------------------------------------------//
    
    float3 toSun = (float3){ 0.0f, -1.0f, 0.0f };
    if(depth >= depth + 10.0f)
    {
        float amb = 0.2f;
        float diff = 0.8f * clamp(dot(eyeDir, toSun), 0.0f, 1.0f);
        frag *= amb+diff;
    }
    else
    {
        float3 pos = (float3){ res[0], res[1], res[2] };
        float3 normal = (float3){ res[3], res[4], res[5] };
        float depthL = 1.0f/0.0f;
        
        traceObjects(
            pos + 0.0001f*normal,
            toSun,
            objects,
            offsets,
            &depthL,
            res
            );
                
        float amb = 0.2f;
        float diff = 0.8f;
        if(depthL >= depthL + 10.0f) //no object blocking
            diff = 0.8f * clamp(dot(normal, toSun), 0.0f, 1.0f);
        frag *= amb+diff;
    }
    
    frameBuf[id] = (uchar)floor(255.1f*frag);
    depthBuf[id] = depth;
    return;
}

