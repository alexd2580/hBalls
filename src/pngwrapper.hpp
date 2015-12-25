#ifndef __PNGWRAP__H_
#define __PNGWRAP__H_

#include<png.h>
#include<cstdint>
#include<string>

void saveBWToFile(uint8_t* pixels, unsigned int w, unsigned int h, std::string& fpath);

#endif
