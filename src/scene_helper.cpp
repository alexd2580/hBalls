
#include "scene_helper.hpp"

void box(Scene& scene, float x, float y, float z, Material const& mat)
{
  x /= 2.0f;
  y /= 2.0f;
  z /= 2.0f;

  /* left/right bottom/top back/front */

  glm::vec3 lbb(-x, -y, -z);
  glm::vec3 rbb(x, -y, -z);
  glm::vec3 rbf(x, -y, z);
  glm::vec3 lbf(-x, -y, z);

  glm::vec3 ltb(-x, y, -z);
  glm::vec3 rtb(x, y, -z);
  glm::vec3 rtf(x, y, z);
  glm::vec3 ltf(-x, y, z);

  /*front */ scene.quad(mat, lbf, rbf, rtf, ltf);
  /*left  */ scene.quad(mat, lbf, ltf, ltb, lbb);
  /*back  */ scene.quad(mat, lbb, ltb, rtb, rbb);
  /*right */ scene.quad(mat, rtf, rbf, rbb, rtb);
  /*top   */ scene.quad(mat, rtf, rtb, ltb, ltf);
  /*bottom*/ scene.quad(mat, lbb, rbb, rbf, lbf);
}

void room(Scene& scene, float x, float y, float z, Material const& mat)
{
  box(scene, -x, -y, -z, mat);
}

namespace Monitor
{
// w 58 h 35 d 7
// o 9 f20

// centered on bottom front left corner
float width = 0.58f;
float height = 0.35f;
float depth = 0.07f;
float offset = 0.09f;
float footsize = 0.20f;

Material
    black_metal(SurfaceType::metallic, 0.5f, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f));

void display(Scene& scene)
{
  scene.push_matrix();
  scene.translate(width / 2.0f, offset + height / 2.0f, -footsize / 2.0f);
  box(scene, width, height, depth, black_metal);
  scene.pop_matrix();
}

void leg(Scene& scene)
{
  (void)scene;
  // TODO
}

void render(Scene& scene)
{
  display(scene);
  leg(scene);
}
}

namespace Table
{
// center is front bottom left corner
Material
    light_green(SurfaceType::diffuse, 1.0f, 0.0f, glm::vec3(0.6f, 1.0f, 0.6f));

void tableTop(Scene& scene)
{
  scene.push_matrix();
  scene.translate(0.475f, 0.725f, -0.39f);
  box(scene, 0.95f, 0.05f, 0.78f, light_green);
  scene.pop_matrix();

  scene.push_matrix();
  scene.translate(0.35f, 0.75f, -0.45f);
  // Monitor::render();
  scene.pop_matrix();
}

void leg(Scene& scene)
{
  scene.push_matrix();
  scene.translate(0.025f, 0.35f, -0.39f);
  box(scene, 0.05f, 0.7f, 0.78f, light_green);
  scene.pop_matrix();
}

void body(Scene& scene)
{
  scene.push_matrix();
  scene.translate(0.95f, 0.0f, 0.0f);

  float t_h = 1.49f;
  float b_h = 0.05f;
  float s_h = 0.012f;
  float u_h = (t_h - 2 * b_h - 3 * s_h) / 4.0f;

  scene.push_matrix();
  scene.translate(0.2f, b_h / 2.0f, -0.39f);
  box(scene, 0.4f, b_h, 0.78f, light_green);
  scene.translate(0.0f, (b_h + s_h) / 2.0f + u_h, 0.0f);
  box(scene, 0.4f, s_h, 0.68f, light_green);
  scene.translate(0.0f, s_h + u_h, 0.0f);
  box(scene, 0.4f, s_h, 0.68f, light_green);
  scene.translate(0.0f, s_h + u_h, 0.0f);
  box(scene, 0.4f, s_h, 0.68f, light_green);
  scene.pop_matrix();
  scene.push_matrix();
  scene.translate(0.2f, t_h - b_h / 2.0f, -0.39f);
  box(scene, 0.4f, b_h, 0.78f, light_green);
  scene.pop_matrix();

  float t_d = 0.78f;
  float u_d = (t_d - 2 * b_h - s_h) / 2.0f;

  scene.push_matrix();
  scene.translate(0.2f, t_h / 2.0f, -b_h / 2.0f);
  box(scene, 0.4f, t_h - 2 * b_h, b_h, light_green);
  scene.translate(0.0f, 0.0f, -(b_h + s_h) / 2.0f - u_d);
  box(scene, 0.4f, t_h - 2 * b_h, s_h, light_green);
  scene.translate(0.0f, 0.0f, -(b_h + s_h) / 2.0f - u_d);
  box(scene, 0.4f, t_h - 2 * b_h, b_h, light_green);
  scene.pop_matrix();

  scene.pop_matrix();
}

void render(Scene& scene)
{
  leg(scene);
  tableTop(scene);
  body(scene);
}
}

/*void scene(void)
{
  cout << "[Main] Queueing models" << endl;

  scene.quad(
    DIFFUSE,
    glm::vec3(1.0f, 1.0f, 1.0f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(-20.0, -20.0, 0.0),
    glm::vec3(20.0, -20.0, 0.0),
    glm::vec3(20.0, 20.0, 0.0),
    glm::vec3(-20.0, 20.0, 0.0)
  );


  scene.Material red  (DIFFUSE, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f,
0.0f, 0.0f));
  scene.Material green(DIFFUSE, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f,
0.0f, 0.0f));
  scene.Material blue (DIFFUSE, glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f,
0.0f, 0.0f));

  scene.Material mirror(MIRROR, glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f,
0.0f, 0.0f));
  scene.Material metal(METALLIC, glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f,
0.0f, 0.0f));
  scene.Material lamp(DIFFUSE, glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(5.0f,
5.0f, 5.0f));

  scene.Material grey (DIFFUSE, glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f,
0.0f, 0.0f));

  glm::vec3 a(40.0, 0.0, 0.0);

  scene.push_matrix();

  scene.sphere(red, a, 20.0f);
  scene.rotate(2.0f*F_PI/3.0f, 0.0f, 1.0f, 0.0f);
  scene.sphere(green, a, 20.0f);
  scene.rotate(2.0f*F_PI/3.0f, 0.0f, 1.0f, 0.0f);
  scene.sphere(blue, a, 20.0f);
  scene.sphere(metal, glm::vec3(0.0f, 0.0f, 0.0f), 15.0f);
  scene.sphere(lamp, glm::vec3(0.0f, 7000.0f, 0.0f), 5000.0f);

  scene.pop_matrix();

  scene.sphere(mirror, glm::vec3(600.0f, 100.0f, -1000.0f), 1100.0f);


  scene.push_matrix();
  scene.translate(0.0f, 500.0f, 0.0f);
  room(1000.0f, 1000.0f, 1000.0f, grey);
  scene.pop_matrix();
  scene.push_matrix();
  scene.translate(0.0f, 975.0f, 0.0f);
  box(400.0f, 50.0f, 400.0f, lamp);
  scene.pop_matrix();
}*/
