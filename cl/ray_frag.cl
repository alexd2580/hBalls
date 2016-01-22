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
void traceTriangle(float3 eye_pos,
                   float3 eye_dir, // normalized
                   global float* triangle_,
                   global float** closest,
                   float* min_depth,
                   float3 res[])
{
  global float* triangle = triangle_ + HEADER_SIZE;
  float3 a = (float3){triangle[0], triangle[1], triangle[2]};
  float3 b = (float3){triangle[3], triangle[4], triangle[5]};
  float3 c = (float3){triangle[6], triangle[7], triangle[8]};

  float3 atob = b - a;
  float3 atoc = c - a;

  // Begin calculating determinant - also used to calculate u parameter
  float3 P = cross(eye_dir, atoc);
  // if determinant is near zero, ray lies in plane of triangle
  float det = dot(atob, P);

  if(det < 0.00001f)
    return;

  float inv_det = 1.0f / det;
  // calculate distance from V1 to ray origin
  float3 T = eye_pos - a;

  // Calculate u parameter and test bound
  float u = dot(T, P) * inv_det;
  // The intersection lies outside of the triangle
  if(u < 0.0f || u > 1.0f)
    return;

  // Prepare to test v parameter
  float3 Q = cross(T, atob);

  // Calculate v parameter and test bound
  float v = dot(eye_dir, Q) * inv_det;
  // The intersection lies outside of the triangle
  if(v < 0.0f || u + v > 1.0f)
    return;

  float dist = dot(atoc, Q) * inv_det;
  if(dist > 0.00001f && dist < *min_depth)
  {
    res[0] = eye_pos + dist * eye_dir;
    res[1] = normalize(cross(atob, atoc));
    *min_depth = dist;
    *closest = triangle_;
    return;
  }
  return;
}

/**
 * Given eye_pos, eye_dir, and sphere_ computes the intersection point
 * of the ray with the sphere (if there is an intersection)
 * and updates min_depth and closest, if the distance from eye_pos
 * to the hit point is smaller than *min_depth.
 */
void traceSphere(float3 eye_pos,
                 float3 eye_dir, // normalized
                 global float* sphere_,
                 global float** closest,
                 float* min_depth,
                 float3 res[])
{
  global float* sphere = sphere_ + HEADER_SIZE;
  float3 center = (float3){sphere[0], sphere[1], sphere[2]};
  float radius = sphere[3];

  float3 eyeToSphere = center - eye_pos;
  /* View axis distance */
  float dX1 = dot(eye_dir, eyeToSphere);
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
  if(dX1 > *min_depth)
    return;

  *min_depth = dX1;
  *closest = sphere_;
  res[0] = eye_pos + dX1 * eye_dir;
  res[1] = normalize(res[0] - center);

  return;
}

/**
 * Lookup loop. Currently running in O(n) on a list of primitives.
 * TODO Replace by lookup structure.
 */
void runTraceObjects(float3 eye_pos,
                     float3 eye_dir, // normalized
                     global float* objects,
                     uint count,
                     global float** closest,
                     float* min_depth,
                     float3 res[])
{
  global float* object = objects;
  uchar type = *((global uchar*)object);

  for(int index = 0; index < count; index++)
  {
    type = *(global uchar*)object;
    switch(type)
    {
    case TRIANGLE:
      traceTriangle(eye_pos, eye_dir, object, closest, min_depth, res);
      object += HEADER_SIZE + TRIANGLE_SIZE;
      break;
    case SPHERE:
      traceSphere(eye_pos, eye_dir, object, closest, min_depth, res);
      object += HEADER_SIZE + SPHERE_SIZE;
      break;
    default:
      break;
    }
  }
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
                  global float* octree, // EMPTY!! NOT USED!!
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
  float3 frag = (float3){0.0f, 0.0f, 0.0f};
  float3 res[2];

  //--------------------------------------------------------------------------//

  float3 pos;
  float3 normal;

  uchar material;
  float roughness;
  float luminescence;
  float3 color;

  float3 brdf = (float3){1.0f, 1.0f, 1.0f};

  const uint max_bounces = 5;
  global float* closest = 0;
  float min_depth;
  for(uint itr = 0; itr < max_bounces; itr++)
  {
    min_depth = 1.0f / 0.0f;
    runTraceObjects(
        eye_pos, eye_dir, objects, surf_count, &closest, &min_depth, res);
    runTraceObjects(eye_pos,
                    eye_dir,
                    objects + lamp_off,
                    lamp_count,
                    &closest,
                    &min_depth,
                    res);
    if(closest == 0)
      break; // nothing hit
    material = ((global uchar*)closest)[1];
    roughness = closest[1];
    luminescence = closest[2];
    color = (float3){closest[3], closest[4], closest[5]};
    pos = res[0];
    normal = res[1];
    frag += color * luminescence * brdf;

    ulong guess = xorshift1024star(prng) % max_bounces;
    if(guess <= itr)
      break;

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
      /*case GLASS:
          break;
      default:
          break;*/
    }
  }

  //--------------------------------------------------------------------------//

  float4 total = frame_f[id] + (float4){frag.x, frag.y, frag.z, 0.0};
  frame_f[id] = total;

  uchar frag_r = (uchar)clamp(255.1f * total.x / *samples, 0.0f, 255.0f);
  uchar frag_g = (uchar)clamp(255.1f * total.y / *samples, 0.0f, 255.0f);
  uchar frag_b = (uchar)clamp(255.1f * total.z / *samples, 0.0f, 255.0f);
  uint frag_i = frag_r << 24 | frag_g << 16 | frag_b << 8 | 255;

  frame_c[id] = frag_i;
}
