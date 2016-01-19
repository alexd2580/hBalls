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

#define HEADER_SIZE 7 // floats

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

bool intersects_cube(float3 eye_pos,
                     float3 eye_dir, // normalized
                     float3 lower,
                     float3 private upper)
{
  float latest_entry = -1.0f / 0.0f;
  float earliest_exit = 1.0f / 0.0f;

  float lower_intersect, upper_intersect;
  if(eye_dir.x == 0.0f)
  {
    if(eye_pos.x < lower.x || eye_pos.x > upper.x)
      return false;
  }
  else
  {
    lower_intersect = lower.x - eye_pos.x / eye_dir.x;
    upper_intersect = upper.x - eye_pos.x / eye_dir.x;
    latest_entry = max(latest_entry, min(lower_intersect, upper_intersect));
    earliest_exit = min(earliest_exit, max(lower_intersect, upper_intersect));
  }
  if(eye_dir.y == 0.0f)
  {
    if(eye_pos.y < lower.y || eye_pos.y > upper.y)
      return false;
  }
  else
  {
    lower_intersect = lower.y - eye_pos.y / eye_dir.y;
    upper_intersect = upper.y - eye_pos.y / eye_dir.y;
    latest_entry = max(latest_entry, min(lower_intersect, upper_intersect));
    earliest_exit = min(earliest_exit, max(lower_intersect, upper_intersect));
  }
  if(eye_dir.z == 0.0f)
  {
    if(eye_pos.z < lower.z || eye_pos.z > upper.z)
      return false;
  }
  else
  {
    lower_intersect = lower.z - eye_pos.z / eye_dir.z;
    upper_intersect = upper.z - eye_pos.z / eye_dir.z;
    latest_entry = max(latest_entry, min(lower_intersect, upper_intersect));
    earliest_exit = min(earliest_exit, max(lower_intersect, upper_intersect));
  }

  return latest_entry < earliest_exit && earliest_exit > 0.0f;
}

global float* runTraceObjects(float3 eye_pos,
                              float3 eye_dir, // normalized
                              global float* objects,
                              global float* octree,
                              float3 res[])
{
  global float* object;
  global float* closest = 0;
  uchar type = *((global uchar*)objects);
  float min_depth = 1.0f / 0.0f;

  global float* octrees_todo[100]; // TODO analyze approximation
  octrees_todo[0] = octree;
  octrees_todo[1] = 0;
  int index = 0;
  int next_free = 1;

  float3 lower;
  float3 upper;
  int size;

  global float* cur_octree;
  global int* cur_octree_i;

  while(index < next_free)
  {
    cur_octree = octrees_todo[index];
    index++;

    lower = (float3){cur_octree[0], cur_octree[1], cur_octree[2]};
    upper = (float3){cur_octree[3], cur_octree[4], cur_octree[5]};
    if(intersects_cube(eye_pos, eye_dir, lower, upper))
    {
      printf("intersect\n");
    }
    else
    {
      printf("miss\n");
      continue;
    }

    cur_octree_i = (global int*)(cur_octree + 6);
    size = *cur_octree_i;
    printf("%d\n", size);
    cur_octree_i++;
    for(int i = 0; i < size; i++)
    {
      object = objects + *cur_octree_i;
      cur_octree_i++;
      type = *(global uchar*)object;
      switch(type)
      {
      case TRIANGLE:
        traceTriangle(eye_pos, eye_dir, object, &closest, &min_depth, res);
        break;
      case SPHERE:
        traceSphere(eye_pos, eye_dir, object, &closest, &min_depth, res);
        break;
      default:
        break;
      }
    }

    int offset;
    for(int i = 0; i < 8; i++)
    {
      offset = *cur_octree_i;
      cur_octree_i++;
      if(offset != -1)
      {
        octrees_todo[next_free] = cur_octree + offset;
        next_free++;
      }
    }
  }
  return closest;
}

/*
fov should be an array of floats:
fovy, aspect,
posx, posy, posz,
dirx, diry, dirz,
upx, upy, upz,
leftx, lefty, leftz,
size_x, size_y <--- TWO INTS!
*/

kernel void trace(global float* fov,
                  global float* objects,
                  global float* octree,
                  global uint* frame_c,
                  global float4* frame_f,
                  global float* samples,
                  global PRNG* prng)
{
  float fovy = fov[0];
  float aspect = fov[1];

  float3 eye_pos = (float3){fov[2], fov[3], fov[4]};
  float3 eye_dir = (float3){fov[5], fov[6], fov[7]};
  float3 eye_up = (float3){fov[8], fov[9], fov[10]};
  float3 eye_left = (float3){fov[11], fov[12], fov[13]};
  int size_w = ((global int*)(fov + 14))[0];
  int size_h = ((global int*)(fov + 14))[1];

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

  //------------------------------------------------------------------------//
  float3 pos;
  float3 normal;

  uchar material;
  float3 passive;
  float3 active;

  float3 brdf = (float3){1.0f, 1.0f, 1.0f};

  global float* closest = 0;
  for(uint itr = 0; itr < 5 && dot(brdf, brdf) > 0.9f; itr++)
  {
    closest = runTraceObjects(eye_pos, eye_dir, objects, octree, res);
    if(closest == 0)
      break; // nothing hit
    material = ((global uchar*)closest)[1];
    passive = (float3){closest[1], closest[2], closest[3]};
    active = (float3){closest[4], closest[5], closest[6]};
    pos = res[0];
    normal = res[1];
    frag += active * brdf;

    switch(material)
    {
    case DIFFUSE:
      eye_pos = pos;
      eye_dir = oriented_uniform_sample_hemisphere(prng, normal);
      brdf *= 2.0f * passive * dot(normal, eye_dir);
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

  //------------------------------------------------------------------------//

  float4 total = frame_f[id] + (float4){frag.x, frag.y, frag.z, 0.0};
  frame_f[id] = total;

  uchar frag_r = (uchar)clamp(255.1f * total.x / *samples, 0.0f, 255.0f);
  uchar frag_g = (uchar)clamp(255.1f * total.y / *samples, 0.0f, 255.0f);
  uchar frag_b = (uchar)clamp(255.1f * total.z / *samples, 0.0f, 255.0f);
  uint frag_i = frag_r << 24 | frag_g << 16 | frag_b << 8 | 255;

  frame_c[id] = frag_i;
}
