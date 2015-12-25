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


constant int vertex_size = 3*sizeof(float);
constant float min_dist = 0.1f;
constant float max_dist = 500.0f;
constant int max_steps = 100;

constant int max_iterations = 100;
constant float scale = 20.0f;
constant float fixedRadius2 = 10.0f;
constant float minRadius2 = 5.0f;
constant float foldingLimit = 20.0f;


float3 sphereFold(float3 z, float* dz)
{
	float r2 = dot(z,z);
	if(r2<minRadius2)
	{ 
		float temp = fixedRadius2/minRadius2;
		z *= temp;
		*dz *= temp;
	} 
	else if(r2<fixedRadius2)
	{ 
		float temp = fixedRadius2/r2;
		z *= temp;
		*dz *= temp;
	}
	return z;
}

float3 boxFold(float3 z)
{
	return(clamp(z, -foldingLimit, foldingLimit) * 2.0f - z);
}

float estimateMandelbox(float3 z)
{
	float3 offset = z;
	float dr = 1.0f;
	for(int n=0; n<max_iterations; n++)
	{
		z = boxFold(z);         // Reflect
		z = sphereFold(z, &dr);    // Sphere Inversion
 		
        z = scale*z + offset;  // Scale & Translate
        dr = dr*fabs(scale)+1.0f;
	}
	float r = length(z);
	return(r/fabs(dr));
}

float estimatePolygon(private float3 eyePos, global uchar* object)
{
    uchar n = *object;
    object++;
    
    float3 a = getPos((global float*)(object));
    float3 b = getPos((global float*)(object+vertex_size));
    float3 c = getPos((global float*)(object+2*vertex_size));
    
    b -= a;
    c -= a;
    
    float3 normal = cross(b, c);
    normal = normalize(normal);
    float ed = fabs(dot(normal, eyePos-a));
    if(ed < 0.0f)
        return max_dist;    
    
    float3 ep = eyePos - ed*normal;

    for(uchar i=1; i<n; i++)
    {
        b = getPos((global float*)(object+i*vertex_size));
        c = cross(normal, b-a);
        if(dot(ep-a, c) < 0)
            return(min(length(eyePos-a), length(eyePos-b)));            
        a = b;
    }

    b = getPos((global float*)object);
    c = cross(normal, b-a);
    if(dot(ep-a, c) < 0)
        return(min(length(eyePos-a), length(eyePos-b)));
    
    return ed;
}

float estimateSphere(private float3 eyePos, global uchar* object)
{
    return(length(eyePos-getPos((global float*)object)) - getRadius((global float*)object));
}

/**
 * Returns estimated min-dist one can move forward
 */
private float estimateDist
(
    private float3 eyePos,
    global uchar* objects,
    global int* offsets
)
{
    private int offset;
    global uchar* object;
    private uchar type;
    private float dist = max_dist;
        
    for(int i=0; i<1000; i++)
    {
        offset = offsets[i];
        if(offset == -1)
            return dist; //-1 indicates last object
        
        object = objects + offset;
        type = object[0];
        
        switch(type)
        {
        case POLYGON:
            dist = min(dist, estimatePolygon(eyePos, object+3));
            break;
        case SPHERE:
            dist = min(dist, estimateSphere(eyePos, object+3));
            break;
        default:
            break;
        }
    }
    return dist;
}

/**
 * Returns the number of steps for the given ray, until min_dist is reached
 */
int march
(
    private float3 eyePos,
    private float3 eyeDir, //normalized
    global uchar* objects,
    global int* offsets
)
{
    private float s = max_dist;
    private int itr;
      
    for(itr = 0; s > min_dist && itr < max_steps; itr++)
    {
        s = estimateDist(eyePos, objects, offsets);
        //s = estimateMandelbox(eyePos);
        eyePos += s*eyeDir;
        if(s >= max_dist) 
            itr = max_steps;
    }
    
    return itr;
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
    
    /* DONE COMPUTING EYEDIR */

    private int steps = 0;
    private float depth = depthBuf[id];
    private float res[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    //------------------------------------------------------------------------//
      steps = march(eyePos, eyeDir, objects, offsets);
    //------------------------------------------------------------------------//
    
    frameBuf[id] = (uchar)floor((255.1f*(float)steps)/(float)max_steps);
    depthBuf[id] = 0.0f; //depth; TODO
    return;
}

/*void tracePolygon //this doesn't work... now it does! a LOT faster
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
}*/

/*

float3 toSun = (float3){ 0.0f, 1.0f, 0.0f };
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
    
    */ 
