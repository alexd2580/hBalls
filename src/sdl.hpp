#ifndef GRAPHICS_H_
#define GRAPHICS_H_

#include<cstdint>

namespace SDL
{
  int init(unsigned int w, unsigned int h);
  void close(void);

  void drawFrame(uint32_t* pixels);
  void handleEvents(void);
  void wait(uint32_t);

  extern bool die;
}

#endif /* GRAPHICS_H_ */
