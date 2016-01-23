/*{-# RayTracer by AlexD2580 -- where no Harukas were harmed! #-}*/

/**** PUBLISHED UNDER THE HAPPY BUNNY LICENSE ****/

/**
## HEADER ##
type            :: Uint8
material        :: Uint8
__padding__     :: Uint16
roughness       :: Float
-- 0 -> mirror-like; 1 -> diffuse;
luminescence    :: Float
-- values can (and for lamps they should) be greater than 1
color           :: Float3

## BODY ##
payload     :: Data

sizeof(Sphere) = (6+4) * 4 byte
sizeof(Triengle) = (6+9) * 4 byte
**/

#define HEADER_SIZE 6 // floats
#define TRIANGLE_SIZE 9
#define SPHERE_SIZE 4

#define TRIANGLE 1
#define SPHERE 2

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

typedef struct Intersection
{
  /** These parameters are set by the trace functions **/
  /* Pointer to the global object definition */
  global float* object;
  /* Position */
  float3 pos;
  /* Normal */
  float3 normal;
  /* Distance */
  float dist;
} Intersection;

typedef struct IntersectionTest
{
  /** These parameters are set by the trace functions **/
  /* Pointer to the global object definition */
  global float* object[100];
  /* Position */
  float3 pos[100];
  /* Normal */
  float3 normal[100];
  /* Distance */
  float dist[100];
} IntersectionTest;

/*
  uchar material;
  float3 color;
  float luminescence;
  float roughness;
} Intersection;*/

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
                   global float* triangle_,
                   Intersection* closest)
{
  global float* triangle = triangle_ + HEADER_SIZE;
  float3 a = (float3){triangle[0], triangle[1], triangle[2]};
  float3 b = (float3){triangle[3], triangle[4], triangle[5]};
  float3 c = (float3){triangle[6], triangle[7], triangle[8]};

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
  if(dist > 0.00001f && dist < closest->dist)
  {
    closest->pos = ray.pos + dist * ray.dir;
    closest->normal = normalize(cross(atob, atoc));
    closest->dist = dist;
    closest->object = triangle_;
  }
}

/**
 * Given eye_pos, eye_dir, and sphere_ computes the intersection point
 * of the ray with the sphere (if there is an intersection)
 * and updates min_depth and closest, if the distance from eye_pos
 * to the hit point is smaller than *min_depth.
 */
void test_sphere(const Ray ray, global float* sphere_, Intersection* closest)
{
  global float* sphere = sphere_ + HEADER_SIZE;
  float3 center = (float3){sphere[0], sphere[1], sphere[2]};
  float radius = sphere[3];

  float3 eyeToSphere = center - ray.pos;
  /* View axis distance */
  float dX1 = dot(ray.dir, eyeToSphere);
  if(dX1 <= 0)
    return;

  /* SqrDistance to center of the sphere */
  float dH2 = dot(eyeToSphere, eyeToSphere);

  /* SqrOffset from view axis */
  float dY2 = dH2 - dX1 * dX1;

  /* Offset is greater than radius */
  float oX2 = radius * radius - dY2;
  if(oX2 < 0)
    return;

  /* Now dX1 is the distance to the intersection */
  dX1 -= sqrt(oX2);
  /* Intersection is not visible */
  if(dX1 > closest->dist)
    return;

  closest->object = sphere_;
  closest->dist = dX1;
  closest->pos = ray.pos + dX1 * ray.dir;
  float inv_rad = 1.0f / radius;
  closest->normal = (closest->pos - center) * inv_rad;
}

/**
 * Lookup loop. Currently running in O(n) on a list of primitives.
 * TODO Replace by lookup structure.
 */
void run_trace(const Ray ray,
               global float* objects,
               uint count,
               Intersection* closest)
{
  global float* object = objects;
  uchar type = *((global uchar*)object);
  for(int index = 0; index < count; index++)
  {
    type = *(global uchar*)object;
    switch(type)
    {
    case TRIANGLE:
      test_triangle(ray, object, closest);
      object += HEADER_SIZE + TRIANGLE_SIZE;
      break;
    case SPHERE:
      test_sphere(ray, object, closest);
      object += HEADER_SIZE + TRIANGLE_SIZE; // Max of both
      break;
    default:
      break;
    }
  }
}

/*inline void read_header(Intersection* const i)
{
  global float* obj = i->object;
  i->material = ((global uchar*)obj)[1];
  i->roughness = obj[1];
  i->luminescence = obj[2];
  i->color = (float3){obj[3], obj[4], obj[5]};
}*/

Intersection gen_random_point(global PRNG* prng, global float* obj, uint count)
{
  uint rand = xorshift1024star(prng) % count;
  obj += rand * (HEADER_SIZE + TRIANGLE_SIZE);

  uchar type = ((global uchar*)obj)[0];
  Intersection i;
  i.object = obj;
  // read_header(&i);

  switch(type)
  {
  case SPHERE:
  {
    i.normal = uniform_sample_sphere(prng);
    float radius = obj[9];
    i.pos = (float3){obj[6], obj[7], obj[8]} + i.normal * radius;
  }
  break;
  case TRIANGLE:
  {
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
    i.pos = a + r1 * to_b + r2 * to_c;
    i.normal = normalize(cross(to_b, to_c));
  }
  break;
  default:
    break;
  }
  return i;
}

/**
 * Main kernel function.
 * general_data is an array of 19 4-byte units:
 * fovy, aspect :: Float
 * posx, posy, posz :: Float
 * dirx, diry, dirz :: Float
 * upx, upy, upz :: Float
 * leftx, lefty, leftz :: Float
 * size_w, size_h :: UInt
 * num_surfs :: UInt
 * num_lamps :: UInt
 * off_lamps :: UInt
 */
kernel void trace(global void* general_data,
                  global float* objects,
                  global float* UNUSED,
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

  uint surf_count = data_i[16];
  uint lamp_count = data_i[17];
  uint lamp_off = data_i[18];

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

#define MAX_BOUNCES 1
  /* First bounce point is NOT the eye itself */
  Intersection eye_intersections[1];

  IntersectionTest asd;
  asd.object[0] = 0;

  /* First bounce point is the random lamp-point */
  Intersection lamp_intersections[MAX_BOUNCES];
  lamp_intersections[0] =
      gen_random_point(prng, objects + lamp_off, lamp_count);

  //--------------------------------------------------------------------------//

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
    break;*/
  /*case GLASS:
        break;
    default:
        break;*/ /*
}

*/

  Ray ray;
  Intersection* intersection;
  global float* object;
  uint material;
  float roughness;

  /* Compute eye bounces */
  ray.pos = eye_pos;
  ray.dir = eye_dir;

  uint eye_bounces = 0;
  while(eye_bounces < MAX_BOUNCES)
  {
    intersection = eye_intersections + eye_bounces;
    intersection->object = 0;
    intersection->dist = 1.0f / 0.0f;
    run_trace(ray, objects, surf_count, intersection);
    run_trace(ray, objects + lamp_off, lamp_count, intersection);
    object = intersection->object;
    if(object == 0)
      break; // nothing hit
    eye_bounces++;

    material = ((global uchar*)object)[1];
    roughness = object[1];
    ray.pos = intersection->pos;

    switch(material)
    {
    case DIFFUSE:
      ray.dir = oriented_uniform_sample_hemisphere(prng, intersection->normal);
      break;
    case MIRROR:
      ray.dir = reflect(ray.dir, intersection->normal);
      break;
    case METALLIC:
      ray.dir = reflect(ray.dir, intersection->normal);
      ray.dir = sample_hemisphere(prng, ray.dir, 1.0f, 1.0f);
      break;
    default:
      break;
      /*case GLASS:
          break;
      default:
          break;*/
    }
  }

  /* Compute light bounces */
  ray.pos = lamp_intersections[0].pos;
  ray.dir =
      oriented_uniform_sample_hemisphere(prng, lamp_intersections[0].normal);

  uint lamp_bounces = 1;
  while(lamp_bounces < MAX_BOUNCES)
  {
    intersection = lamp_intersections + lamp_bounces;
    intersection->object = 0;
    intersection->dist = 1.0f / 0.0f;
    run_trace(ray, objects, surf_count, intersection);
    run_trace(ray, objects + lamp_off, lamp_count, intersection);
    object = intersection->object;
    if(object == 0)
      break; // nothing hit
    lamp_bounces++;

    material = ((global uchar*)object)[1];
    roughness = object[1];
    ray.pos = intersection->pos;

    switch(material)
    {
    case DIFFUSE:
      ray.dir = oriented_uniform_sample_hemisphere(prng, intersection->normal);
      break;
    case MIRROR:
      ray.dir = reflect(ray.dir, intersection->normal);
      break;
    case METALLIC:
      ray.dir = reflect(ray.dir, intersection->normal);
      ray.dir = sample_hemisphere(prng, ray.dir, 1.f, 1.f);
      break;
      /*case GLASS:
          break;
      default:
          break;*/
    }
  }

  //--------------------------------------------------------------------------//

  float3 frag = (float3){1.0f, 0.0f, 0.0f};
  float4 total = frame_f[id] + (float4){frag.x, frag.y, frag.z, 0.0};
  frame_f[id] = total;

  uchar frag_r = (uchar)clamp(255.1f * total.x / *samples, 0.0f, 255.0f);
  uchar frag_g = (uchar)clamp(255.1f * total.y / *samples, 0.0f, 255.0f);
  uchar frag_b = (uchar)clamp(255.1f * total.z / *samples, 0.0f, 255.0f);
  uint frag_i = frag_r << 24 | frag_g << 16 | frag_b << 8 | 255;
  frame_c[id] = frag_i;
}
