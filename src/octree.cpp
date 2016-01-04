#include <glm/glm.hpp>
#include <vector>
#include <iostream>

#include "octree.hpp"

using namespace std;

ostream& operator<<(ostream& out, glm::vec3 const& val)
{
  return out << "(" << val.x << "," << val.y << "," << val.z << ")";
}

AABB::AABB(glm::vec3 const& lower_, glm::vec3 const& upper_)
    : lower(lower_), upper(upper_)
{
}

__attribute__((pure)) bool AABB::is_subspace_of(AABB const& space) const
{
  return lower.x >= space.lower.x && lower.y >= space.lower.y &&
         lower.z >= space.lower.z && upper.x <= space.upper.x &&
         upper.y <= space.upper.y && upper.z <= space.upper.z;
}

__attribute__((pure)) bool AABB::is_superspace_of(AABB const& space) const
{
  return lower.x <= space.lower.x && lower.y <= space.lower.y &&
         lower.z <= space.lower.z && upper.x >= space.upper.x &&
         upper.y >= space.upper.y && upper.z >= space.upper.z;
}

ostream& operator<<(ostream& out, AABB const& val)
{
  return out << val.lower << " => " << val.upper;
}

void push_vector(float* data, glm::vec3 const& vec)
{
  data[0] = vec.x;
  data[1] = vec.y;
  data[2] = vec.z;
}

Octree::Octree(AABB const& aabb_) : aabb(aabb_)
{
  for (int i = 0; i < 8; i++)
    sub[i] = nullptr;
}

Octree::Octree(glm::vec3 const& lower, glm::vec3 const& upper)
    : aabb(lower, upper)
{
  for (int i = 0; i < 8; i++)
    sub[i] = nullptr;
}

Octree::~Octree(void)
{
  for (int i = 0; i < 8; i++)
    if (sub[i] != nullptr)
      delete sub[i];
}

Octree* Octree::insert(unsigned int val, AABB const& space)
{
  float size = aabb.upper.x - aabb.lower.x; // qubic dx=dy=dz

  /** Check if it's a subspace [of one of the eight subspaces] **/
  glm::vec3 lower;
  glm::vec3 upper;

  if (space.is_subspace_of(aabb))
  {
    for (int i = 0; i < 8; i++)
    {
      if (sub[i] != nullptr)
      {
        if (space.is_subspace_of(sub[i]->aabb))
        {
          /* it's guaranteed that the given space is a subspace of
            sub[i], therefore it doesn't have to be extended. */
          (void)sub[i]->insert(val, space);
          return this;
        }
      }
      else
      {
        lower = aabb.lower;
        int id = i;
        if (id / 4 == 1)
          lower.x += size / 2.0f;
        if ((id % 4) / 2 == 1)
          lower.y += size / 2.0f;
        if (id % 2 == 1)
          lower.z += size / 2.0f;

        upper = lower + (size / 2.0f) * glm::vec3(1.0f);
        AABB subspace(lower, upper);

        if (space.is_subspace_of(subspace))
        {
          /* it's guaranteed that the given space is a subspace of
            sub[i], therefore it doesn't have to be extended. */
          sub[i] = new Octree(subspace);
          (void)sub[i]->insert(val, space);
          return this;
        }
      }
    }

    primitives.push_back(val);
    return this;
  }

  glm::vec3 offset(aabb.lower - space.lower);
  unsigned int id = 0;
  glm::vec3 new_lower(aabb.lower);

  if (offset.x > 0)
  {
    id += 4;
    new_lower.x -= size;
  }
  if (offset.y > 0)
  {
    id += 2;
    new_lower.y -= size;
  }
  if (offset.z > 0)
  {
    id += 1;
    new_lower.z -= size;
  }
  glm::vec3 new_upper(new_lower);
  new_upper.x += 2 * size;
  new_upper.y += 2 * size;
  new_upper.z += 2 * size;

  Octree* ext = new Octree(new_lower, new_upper);
  ext->sub[id] = this;
  return ext->insert(val, space);
}

void Octree::print_info(void) const
{
  cout << "AABB: " << aabb << " : ";
  for (auto i = primitives.begin(); i != primitives.end(); i++)
    cout << *i << " ";
  cout << endl;
  for (int i = 0; i < 8; i++)
  {
    if (sub[i] != nullptr)
    {
      cout << "Subtree: ";
      sub[i]->print_info();
    }
  }
}

/**
Format:
float3 lower
float3 upper
unsigned int size
(size * unsigned int) ids
(8 * int) offsets <-
float-size units difference to beginning of thus subtree
-1 if there is no subtree there.
=> the subtrees

**/

/**
* Returns the space that storing this octree
* required in float-sized units
* @param buffer_f - A pointer to the start of the available space.
*/
unsigned int Octree::print_to_array(float* buffer_f) const
{
  unsigned int offset = 0;
  int* buffer_i = (int*)buffer_f; // sozeof(int) == sizeof(float)

  push_vector(buffer_f, aabb.lower);
  push_vector(buffer_f + 3, aabb.upper);
  offset += 6;

  buffer_i[offset] = (int)primitives.size();
  offset++;
  for (auto i = primitives.begin(); i != primitives.end(); i++)
  {
    buffer_i[offset] = *i;
    offset++;
  }

  unsigned int subsize = 0;

  for (int i = 0; i < 8; i++)
  {
    if (sub[i] == nullptr)
      buffer_i[offset + i] = -1;
    else
    {
      buffer_i[offset + i] = (int)subsize + offset + 8;
      subsize += sub[i]->print_to_array(buffer_f + offset + 8 + subsize);
    }
  }

  return offset + 8 + subsize;
}

Octree* Octree::reconstruct(float* data_f)
{
  glm::vec3 lower(data_f[0], data_f[1], data_f[2]);
  glm::vec3 upper(data_f[3], data_f[4], data_f[5]);
  Octree* oct = new Octree(lower, upper);
  int* data_i = (int*)data_f + 6;
  int size = *data_i;
  data_i++;
  for (int i = 0; i < size; i++)
    oct->primitives.push_back(data_i[i]);
  data_i += size;

  for (int i = 0; i < 8; i++)
  {
    int off = data_i[i];
    if (off == -1)
      oct->sub[i] = nullptr;
    else
      oct->sub[i] = Octree::reconstruct(data_f + off);
  }
  return oct;
}

/*int main(void)
{
  Octree* o = new Octree(glm::vec3(0.0f), glm::vec3(1.0f));
  o = o->insert(1, AABB(glm::vec3(0.1f), glm::vec3(0.9f)));
  o = o->insert(2, AABB(glm::vec3(0.1f), glm::vec3(1.0f)));
  o = o->insert(3, AABB(glm::vec3(-0.1f), glm::vec3(0.9f)));
  o = o->insert(4, AABB(glm::vec3(10000.0f, 0.0f,0.0f), glm::vec3(10001.0f,
0.0f,0.0f)));
  o->print_info();
  float asd[10000];
  cout << o->print_to_array(asd) << endl;
  delete o;
  o = nullptr;
  o = Octree::reconstruct(asd);
  o->print_info();
  delete o;
  return 0;
} */
