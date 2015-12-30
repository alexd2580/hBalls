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

/*** RANDOM ***/

typedef struct PRNG {
    ulong m_seed[16];
    ulong m_p;
} PRNG;

// From http://xorshift.di.unimi.it/xorshift1024star.c
inline ulong xorshift1024star(global PRNG *prng)
{
    ulong s0 = prng->m_seed[prng->m_p];
    ulong s1 = prng->m_seed[prng->m_p = (prng->m_p + 1) & 15];
    s1 ^= s1 << 31; // a
    s1 ^= s1 >> 11; // b
    s0 ^= s0 >> 30; // c
    return (prng->m_seed[prng->m_p] = s0 ^ s1) * 1181783497276652981L * get_global_id(0) * get_global_id(1);
}

inline float rand_range(global PRNG *prng, const float min, const float max)
{
    return min + ((float)xorshift1024star(prng)) / (float)(ULONG_MAX / (max - min));
}

/**
 * @brief Selects a random point on a sphere with uniform distribution.
 *
 * Marsaglia, G. "Choosing a Point from the Surface of a Sphere." Ann. Math. Stat. 43, 645-646, 1972.
 * Muller, M. E. "A Note on a Method for Generating Points Uniformly on N-Dimensional Spheres."
 * Comm. Assoc. Comput. Mach. 2, 19-20, Apr. 1959.
 *
 * @return A random point on the surface of a sphere
 */
inline float3 uniform_sample_sphere(global PRNG *prng)
{
    float3 rand_vec;
    rand_vec.x = rand_range(prng, -1.f, 1.f);
    rand_vec.y = rand_range(prng, -1.f, 1.f);
    rand_vec.z = rand_range(prng, -1.f, 1.f);
    return normalize(rand_vec);
}

/**
 * @brief Given a direction vector, returns a random uniform point on the hemisphere around dir.
 *
 * @param dir A vector that represents the hemisphere's center
 * @return A random point the on the hemisphere
 */
inline float3 oriented_uniform_sample_hemisphere(global PRNG *prng, float3 dir)
{
    float3 v = uniform_sample_sphere(prng);
    return v * sign(dot(v, dir));
}

/**
 * @brief Selects a random point on a sphere with uniform or cosine-weighted distribution.
 *
 * @param dir A vector around which the hemisphere will be centered
 * @param power 0.f means uniform distribution while 1.f means cosine-weighted
 * @param angle When a full hemisphere is desired, use pi/2. 0 equals perfect reflection. The value
 * should therefore be between 0 and pi/2. This angle is equal to half the cone width.
 * @return A random point on the surface of a sphere
 *//*
inline glm::vec3 sample_hemisphere(glm::vec3 dir, float power, float angle) {
    // Code by Mikael Hvidtfeldt Christensen
    // from http://blog.hvidtfeldts.net/index.php/2015/01/path-tracing-3d-fractals/
    // Thanks!

    glm::vec3 o1 = glm::normalize(ortho(dir));
    glm::vec3 o2 = glm::normalize(glm::cross(dir, o1));
    glm::vec2 r = glm::vec2{rand_range(0.f, 1.f), rand_range(glm::cos(angle), 1.f)};
    r.x = r.x * glm::two_pi<float>();
    r.y = glm::pow(r.y, 1.f / (power + 1.f));
    float oneminus = glm::sqrt(1.f - r.y * r.y);
    return glm::cos(r.x) * oneminus * o1 + glm::sin(r.x) * oneminus * o2 + r.y * dir;
}*/

/*** RANDOM ***/
/***  UTIL  ***/

/**
 * Returns a vector orthogonal to a given vector in 3D space.
 * @param v The vector to find an orthogonal vector for
 * @return A vector orthogonal to the given vector
 */
inline float3 ortho(const float3 v)
{
    // Awesome branchless function for finding an orthogonal vector in 3D space by
    // http://lolengine.net/blog/2013/09/21/picking-orthogonal-vector-combing-coconuts
    //
    // Their "boring" branching is commented here for completeness:
    // return glm::abs(v.x) > glm::abs(v.z) ? glm::vec3(-v.y, v.x, 0.0) : glm::vec3(0.0, -v.z, v.y);

    float k = fract(fabs(v.x) + 0.5f, (global float*)0);
    return (float3){ -v.y, v.x - k * v.z, k * v.y };
}

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
    private float* min_depth,
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
    if(dX1 > *min_depth)
        return;

    res[0] = eyePos + dX1*eyeDir;
    res[1] = normalize(res[0] - center);
    *min_depth = dX1;
    return;
}

global float* runTraceObjects
(
    private float3 eyePos,
    private float3 eyeDir, //normalized
    global float* objects,
    private float3 res[]
)
{
    global float* closest = 0;
    private uchar type = ((global uchar*)objects)[0];
    private float min_depth = 1.0f / 0.0f;
    private float depth = min_depth;

    while(type != END) // type 0 is END
    {
        switch(type)
        {
        case TRIANGLE: // +7 skip header
            traceTriangle(eyePos, eyeDir, objects+7, &depth, res);
            break;
        case SPHERE:
            traceSphere(eyePos, eyeDir, objects+7, &depth, res);
            break;
        default:
            break;
        }

        if(depth < min_depth)
        {
            min_depth = depth;
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
    private float3    eyePos,
    private float3    eyeDir, //normalized
    global  float*    objects,
    private float3    res[],
    global  PRNG*     prng
)
{
    global float* closest = runTraceObjects(eyePos, eyeDir, objects, res);

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
            eyeDir = oriented_uniform_sample_hemisphere(prng, normal);
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
        closest = runTraceObjects(eyePos, eyeDir, objects, res);
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
    global float*   fov,
    global float*   objects,
    global uint*    frame_c,
    global float4*  frame_f,
    global float*   samples,
    global PRNG*    prng
)
{
    private float fovy = fov[0];
    private float aspect = fov[1];

    private float3 eyePos = (float3){ fov[2], fov[3], fov[4] };
    private float3 eyeDir = (float3){ fov[5], fov[6], fov[7] };
    private float3 eyeUp = (float3){ fov[8], fov[9], fov[10] };
    private float3 eyeLeft = (float3){ fov[11], fov[12], fov[13] };
    private int size_w = ((global int*)(fov+14))[0];
    private int size_h = ((global int*)(fov+14))[1];

    const int pos_x = get_global_id(0);
    const int pos_y = get_global_id(1);
    const int id = pos_y * size_w + pos_x;

    //x on screen == x in coordsystem
    private float rel_x = (2.0f * (float)pos_x / (float)size_w) - 1.0f;
    //y on screen goes down, y in coordsys goes up -> invert
    private float rel_y = (2.0f * (float)-pos_y / (float)size_h) + 1.0f;

    private float max_u = tan(fovy/2.0f);
    private float max_r = max_u*aspect;

    eyeDir += (rel_y*max_u*eyeUp - rel_x*max_r*eyeLeft);
    eyeDir = normalize(eyeDir); // TODO FIX THIS!!

    private float3 frag;
    private float3 res[2];

    //------------------------------------------------------------------------//
      frag = traceObjects(eyePos, eyeDir, objects, res, prng);
    //------------------------------------------------------------------------//

    private float4 total = frame_f[id] + (float4){frag.x, frag.y, frag.z, 0.0};
    frame_f[id] = total;

    uchar frag_r = (uchar)clamp(255.1f*total.x / *samples, 0.0f, 255.0f);
    uchar frag_g = (uchar)clamp(255.1f*total.y / *samples, 0.0f, 255.0f);
    uchar frag_b = (uchar)clamp(255.1f*total.z / *samples, 0.0f, 255.0f);
    uint frag_i = frag_r << 24 | frag_g << 16 | frag_b << 8 | 255;

    frame_c[id] = frag_i;
    return;
}
