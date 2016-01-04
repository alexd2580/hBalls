#ifndef SCENE_HELPER____
#define SCENE_HELPER____

#include"scene.hpp"

void box(Scene& scene, float x, float y, float z, Material const& mat);
void room(Scene& scene, float x, float y, float z, Material const& mat);

namespace Table
{
  void render(Scene& scene);
}

#endif
