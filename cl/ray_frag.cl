/*
RayTracer by AlexD2580 -- where no Harukas were harmed!
PUBLISHED UNDER THE HAPPY BUNNY LICENSE

material        :: Uint8
__padding__     :: Uint24
roughness       :: Float
-- 0 -> mirror-like; 1 -> diffuse;
luminescence    :: Float
-- values can (and for lamps they should) be greater than 1
color           :: Float3
payload         :: (Float3,Float3,Float3)
normal          :: Float3

Intersection:
uchar material;
float3 color;
float luminescence;
float roughness;
*/

#define PRIM_SIZE 18 // floats

// Surface types
#define NONE 0
#define DIFFUSE 1
#define METALLIC 2
#define MIRROR 3
#define GLASS 4

/***  UTIL  ***/

/**
 * Represents a ray in 3D space.
 */
typedef struct Ray
{
  /* Origin */
  float3 pos;
  /**
   * Direction. Make sure that this vector is normalized.
   */
  float3 dir;
} Ray;

/**
 * Aggregation of pointers to the pixel aux buffer.
 */
typedef struct Intersection
{
  /* Pointer to the global object definition */
  global float* global* object;
  /* Position */
  global float3* pos;
  /* Distance to bounce */
  float* dist;
} Intersection;

/**
 * Returns a vector orthogonal to a given vector in 3D space.
 * This function was copied (01.01.2016) from github.com/svenstaro/trac0r
 * @param v The vector to find an orthogonal vector for
 * @return A vector orthogonal to the given vector
 */
inline float3 ortho(const float3 v)
{
  // Awesome branchless function for finding an orthogonal vector in 3D space by
  // http://lolengine.net/blog/2013/09/21/picking-orthogonal-vector-combing-coconuts
  //
  // Their "boring" branching is commented here for completeness:
  // return glm::abs(v.x) > glm::abs(v.z) ? glm::vec3(-v.y, v.x, 0.0) :
  // glm::vec3(0.0, -v.z, v.y);

  float m;
  float k = fract(fabs(v.x) + 0.5f, &m);
  return (float3){-v.y, v.x - k * v.z, k * v.y};
}

float3 reflect(float3 source, float3 normal)
{
  float3 axis = normalize(normal);
  float3 ax_dir = axis * dot(source, axis);
  return (source - 2 * ax_dir);
}

/**** UTILS ****/
/*** RANDOM ***/

/**
 * The following section:
 *  PRNG
 *  xorshift1024star
 *  rand_range
 *  uniform_sample_sphere
 *  oriented_uniform_sample_hemisphere
 *  sample_hemisphere
 * was copied and adapted from svenstaro's trac0r:
 * 01.01.2016 => github.com/svenstaro/trac0r
 */

/**
 * Pseudo Random Number Generator
 */
typedef struct PRNG
{
  ulong m_seed[16];
  ulong m_p;
} PRNG;

// From http://xorshift.di.unimi.it/xorshift1024star.c
inline ulong xorshift1024star(global PRNG* prng)
{
  ulong s0 = prng->m_seed[prng->m_p];
  ulong s1 = prng->m_seed[prng->m_p = (prng->m_p + 1) & 15];
  s1 ^= s1 << 31; // a
  s1 ^= s1 >> 11; // b
  s0 ^= s0 >> 30; // c
  return (prng->m_seed[prng->m_p] = s0 ^ s1) * 1181783497276652981L *
         get_global_id(0) * get_global_id(1);
}

inline float rand_range(global PRNG* prng, const float min, const float max)
{
  return min +
         ((float)xorshift1024star(prng)) / (float)(ULONG_MAX / (max - min));
}

/**
 * @brief Selects a random point on a sphere with uniform distribution.
 *
 * Marsaglia, G. "Choosing a Point from the Surface of a Sphere." Ann. Math.
 * Stat. 43, 645-646, 1972.
 * Muller, M. E. "A Note on a Method for Generating Points Uniformly on
 * N-Dimensional Spheres."
 * Comm. Assoc. Comput. Mach. 2, 19-20, Apr. 1959.
 *
 * @return A random point on the surface of a sphere
 */
inline float3 uniform_sample_sphere(global PRNG* prng)
{
  float3 rand_vec;
  rand_vec.x = rand_range(prng, -1.f, 1.f);
  rand_vec.y = rand_range(prng, -1.f, 1.f);
  rand_vec.z = rand_range(prng, -1.f, 1.f);
  return normalize(rand_vec);
}

/**
 * @brief Given a direction vector, returns a random uniform point on the
 * hemisphere around dir.
 *
 * @param dir A vector that represents the hemisphere's center
 * @return A random point the on the hemisphere
 */
inline float3 oriented_uniform_sample_hemisphere(global PRNG* prng, float3 dir)
{
  float3 v = uniform_sample_sphere(prng);
  return v * sign(dot(v, dir));
}

/**
 * @brief Selects a random point on a sphere with uniform or cosine-weighted
 * distribution.
 *
 * @param dir A vector around which the hemisphere will be centered
 * @param power 0.f means uniform distribution while 1.f means cosine-weighted
 * @param angle When a full hemisphere is desired, use pi/2. 0 equals perfect
 * reflection. The value
 * should therefore be between 0 and pi/2. This angle is equal to half the cone
 * width.
 * @return A random point on the surface of a sphere
 */
inline float3
sample_hemisphere(global PRNG* prng, float3 dir, float power, float angle)
{
  // Code by Mikael Hvidtfeldt Christensen
  // from
  // http://blog.hvidtfeldts.net/index.php/2015/01/path-tracing-3d-fractals/
  // Thanks!

  float3 o1 = normalize(ortho(dir));
  float3 o2 = normalize(cross(dir, o1));
  float rx = rand_range(prng, 0.0f, 1.0f);
  float ry = rand_range(prng, cos(angle), 1.0f);
  rx *= 3.1415f * 2.0f;
  ry = pow(ry, 1.0f / (power + 1.0f));
  float oneminus = sqrt(1.0f - ry * ry);
  return cos(rx) * oneminus * o1 + sin(rx) * oneminus * o2 + ry * dir;
}

/*** RANDOM ***/
/**** MAIN  ****/

/**
 * https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
 */
void test_triangle(const Ray ray,
                   global float* triangle,
                   Intersection const isec)
{
  float3 a = (float3){triangle[6], triangle[7], triangle[8]};
  float3 b = (float3){triangle[9], triangle[10], triangle[11]};
  float3 c = (float3){triangle[12], triangle[13], triangle[14]};

  float3 atob = b - a;
  float3 atoc = c - a;

  // Begin calculating determinant - also used to calculate u parameter
  float3 P = cross(ray.dir, atoc);
  // if determinant is near zero, ray lies in plane of triangle
  float det = dot(atob, P);

  if(det < 0.00001f)
    return;

  float inv_det = 1.0f / det;
  // calculate distance from V1 to ray origin
  float3 T = ray.pos - a;

  // Calculate u parameter and test bound
  float u = dot(T, P) * inv_det;
  // The intersection lies outside of the triangle
  if(u < 0.0f || u > 1.0f)
    return;

  // Prepare to test v parameter
  float3 Q = cross(T, atob);

  // Calculate v parameter and test bound
  float v = dot(ray.dir, Q) * inv_det;
  // The intersection lies outside of the triangle
  if(v < 0.0f || u + v > 1.0f)
    return;

  float dist = dot(atoc, Q) * inv_det;
  if(dist > 0.00001f && dist < *isec.dist)
  {
    *isec.pos = ray.pos + dist * ray.dir;
    *isec.dist = dist;
    *isec.object = triangle;
  }
}

/**
 * Lookup loop. Currently running in O(n) on a list of primitives (triangles).
 * TODO Replace by lookup structure.
 */
void run_trace(const Ray ray,
               global float* objects,
               uint count,
               Intersection const isec)
{
  global float* object = objects;
  for(int index = 0; index < count; index++)
  {
    test_triangle(ray, object, isec);
    object += PRIM_SIZE;
  }
}

void gen_random_point(global PRNG* prng,
                      global float* obj,
                      uint count,
                      Intersection const isec)
{
    uint rand = xorshift1024star(prng) % count;
    obj += rand * PRIM_SIZE;

    *isec.object = obj;
    *isec.dist = 0.0f;

    float3 a = (float3){obj[6], obj[7], obj[8]};
    float3 to_b = (float3){obj[9], obj[10], obj[11]} - a;
    float3 to_c = (float3){obj[12], obj[13], obj[14]} - a;
    float r1 = rand_range(prng, 0.0f, 1.0f);
    float r2 = rand_range(prng, 0.0f, 0.5f);
    if(r1 + r2 > 1.0f)
    {
      r1 = 1.0f - r1;
      r2 = 1.0f - r2;
    }
    *isec.pos = a + r1 * to_b + r2 * to_c;
}

/**
 * Main kernel function.
 * general_data is an array of 20 4-byte units:
 * fovy, aspect :: Float
 * posx, posy, posz :: Float
 * dirx, diry, dirz :: Float
 * upx, upy, upz :: Float
 * leftx, lefty, leftz :: Float
 * size_w, size_h :: UInt
 * num_surfs :: UInt
 * num_lamps :: UInt
 * off_lamps :: UInt
 * max_bounces :: UInt
 */
kernel void trace(global void* general_data,
                  global float* objects,
                  global float* aux_buffer,
                  global uint* frame_c,
                  global float4* frame_f,
                  global float* samples,
                  global PRNG* prng)
{
    global float* data_f = (global float*)general_data;
    global int* data_i = (global int*)general_data;

    const float fovy = data_f[0];
    const float aspect = data_f[1];

    float3 eye_pos = (float3){data_f[2], data_f[3], data_f[4]};
    float3 eye_dir = (float3){data_f[5], data_f[6], data_f[7]};
    const float3 eye_up = (float3){data_f[8], data_f[9], data_f[10]};
    const float3 eye_left = (float3){data_f[11], data_f[12], data_f[13]};
    const int size_w = data_i[14];
    const int size_h = data_i[15];

    const uint surf_count = data_i[16];
    const uint lamp_count = data_i[17];
    const uint lamp_off = data_i[18];
    const uint max_bounces = data_i[19];

    /** Pixel coordinates **/
    const int pos_x = get_global_id(0);
    const int pos_y = get_global_id(1);
    const int id = pos_y * size_w + pos_x;
    /** Relative coordinate system [-1..1]x[-1..1] **/
    float rel_x = (2.0f * (float)pos_x / (float)size_w) - 1.0f;
    // y on screen goes down, y in coordsys goes up -> invert
    float rel_y = -((2.0f * (float)pos_y / (float)size_h) - 1.0f);

    float max_u = tan(fovy / 2.0f);
    float max_r = max_u * aspect;

    eye_dir += (rel_y * max_u * eye_up - rel_x * max_r * eye_left);
    eye_dir = normalize(eye_dir); // TODO FIX THIS!!
    eye_dir = sample_hemisphere(prng, eye_dir, 0.0f, 0.001f);

    /**
    * Size of a float/an aux_buffer frame in float-units (4byte-segments)
    * global float** object
    * float3 pos, normal
    * float dist
    */
    const uint float_ptr_size = sizeof(global float**) / 4;
    const uint unit_size = float_ptr_size + 4;

    /**
    * First eye-bounce point is NOT the eye itself
    * First lamp-bounce point is the random lamp-point
    */
    const global float* base_ptr = aux_buffer + 2 * max_bounces * id * unit_size;
    global float* global* object_ptrs = (global float* global*)base_ptr;
    global float3* positions =
      (global float3*)(base_ptr + 2 * max_bounces * float_ptr_size);
    float3 normal;

    Intersection intersection;
    float dist = 0.0f;
    intersection.dist = &dist;

    //--------------------------------------------------------------------------//

    Ray ray;
    global float* object;
    uint material;
    float roughness;

    /* Compute eye bounces */
    ray.pos = eye_pos;
    ray.dir = eye_dir;

    int eye_bounces = 0;




    run_trace(ray, objects, surf_count, intersection);
    object = *intersection.object;

    float3 frag = (float3){0.0f, 0.0f, 0.0f};
    if(object != 0)
    frag = (float3){object[3], object[4], object[5]};

    float4 total = frame_f[id] + (float4){frag.x, frag.y, frag.z, 0.0};
    frame_f[id] = total;

    uchar frag_r = (uchar)clamp(255.1f * total.x / *samples, 0.0f, 255.0f);
    uchar frag_g = (uchar)clamp(255.1f * total.y / *samples, 0.0f, 255.0f);
    uchar frag_b = (uchar)clamp(255.1f * total.z / *samples, 0.0f, 255.0f);
    uint frag_i = frag_r << 24 | frag_g << 16 | frag_b << 8 | 255;
    frame_c[id] = frag_i;







  /*while(eye_bounces < max_bounces)
  {
    intersection.object = object_ptrs + eye_bounces;
    intersection.pos = positions + eye_bounces;

    *intersection.object = 0;
    *intersection.dist = 1.0f / 0.0f;

    run_trace(ray, objects, surf_count, intersection);
    run_trace(ray, objects + lamp_off, lamp_count, intersection);

    object = *intersection.object;
    if(object == 0)
      break; // nothing hit
    eye_bounces++;

    material = ((global uchar*)object)[0];
    roughness = object[1];
    ray.pos = *intersection.pos;
    normal = (float3){object[15], object[16], object[17]};

    switch(material)
    {
    case DIFFUSE:
      ray.dir = oriented_uniform_sample_hemisphere(prng, normal);
      break;
    case MIRROR:
      ray.dir = reflect(ray.dir, normal);
      break;
    case METALLIC:
      ray.dir = reflect(ray.dir, normal);
      ray.dir = sample_hemisphere(prng, ray.dir, 1.0f, 1.0f);
      break;
    }
  }*/

  /*intersection.object = object_ptrs + max_bounces;
  intersection.pos = positions + max_bounces;

  gen_random_point(prng, objects + lamp_off, lamp_count, intersection);
  object = intersection.object;
*/
  /* Compute light bounces *//*
  ray.pos = *intersection.pos;
  normal = (float3){object[15], object[16], object[17]};
  ray.dir = oriented_uniform_sample_hemisphere(prng, normal);

  int lamp_bounces = 1;
  while(lamp_bounces < max_bounces)
  {
    intersection.object = object_ptrs + max_bounces + eye_bounces;
    intersection.pos = positions + max_bounces + eye_bounces;

    *intersection.object = 0;
    *intersection.dist = 1.0f / 0.0f;

    run_trace(ray, objects, surf_count, intersection);
    run_trace(ray, objects + lamp_off, lamp_count, intersection);

    object = *intersection.object;
    if(object == 0)
      break; // nothing hit
    lamp_bounces++;

    material = ((global uchar*)object)[0];
    roughness = object[1];
    ray.pos = *intersection.pos;
    normal = (float3){object[15], object[16], object[17]};

    switch(material)
    {
    case DIFFUSE:
      ray.dir = oriented_uniform_sample_hemisphere(prng, normal);
      break;
    case MIRROR:
      ray.dir = reflect(ray.dir, normal);
      break;
    case METALLIC:
      ray.dir = reflect(ray.dir, normal);
      ray.dir = sample_hemisphere(prng, ray.dir, 1.f, 1.f);
      break;
    }
  }*/

  //--------------------------------------------------------------------------//

  //float3 frag = (float3){0.0f, 0.0f, 0.0f};


  /*if(eye_bounces >= 1)
  {
    object = object_ptrs[eye_bounces - 1];
    float3 color = (float3){object[3], object[4], object[5]};
    float lumin = object[2];
    frag = color * lumin;
  }*/

  //float4 total = frame_f[id] + (float4){frag.x, frag.y, frag.z, 0.0};
  //frame_f[id] = total;

  /*uchar frag_r = (uchar)clamp(255.1f * total.x / *samples, 0.0f, 255.0f);
  uchar frag_g = (uchar)clamp(255.1f * total.y / *samples, 0.0f, 255.0f);
  uchar frag_b = (uchar)clamp(255.1f * total.z / *samples, 0.0f, 255.0f);
  uint frag_i = frag_r << 24 | frag_g << 16 | frag_b << 8 | 255;
  frame_c[id] = frag_i;*/
}


  /*
    float3 d;
    for(int i = eye_bounces - 2; i >= 0; i--)
    {
      object = surf_objects[i];
      color = (float3){object[3], object[4], object[5]};
      lumin = object[2];
      d = surf_positions[i] - surf_positions[i + 1];
      frag *= dot(d, surf_normals[i + 1]) * 2.0f;
      frag += color * lumin;
    }
  }*/

  /*float3 pos;
  float3 normal;

  uchar material;
  float roughness;
  float luminescence;
  float3 color;

  float3 brdf = (float3){1.0f, 1.0f, 1.0f};

  global float* closest = 0;
  float min_depth;

  aterial = ((global uchar*)closest)[1];
  roughness = closest[1];
  luminescence = closest[2];
  color = (float3){closest[3], closest[4], closest[5]};
  pos = res[0];
  normal = res[1];
  frag += color * luminescence * brdf;

  switch(material)
  {
  case DIFFUSE:
    eye_pos = pos;
    eye_dir = oriented_uniform_sample_hemisphere(prng, normal);
    brdf *= 2.0f * color * dot(normal, eye_dir);
    break;
  case MIRROR:
    eye_pos = pos;
    eye_dir = reflect(eye_dir, normal);
    break;
  case METALLIC:
    eye_pos = pos;
    eye_dir = reflect(eye_dir, normal);
    eye_dir = sample_hemisphere(prng, eye_dir, 1.f, 1.f);
    break;

  case GLASS:
    break;
  default:
    break;
  }*/
