#include <stdio.h>
#include <stdlib.h>

#include<cmath>
#include"pngwrapper.hpp"

using namespace std;

/* A colored pixel. */
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} pixel_t;

/* A picture. */
typedef struct  {
    pixel_t* pixels;
    size_t width;
    size_t height;
} bitmap_t;

/**
 * Given "bitmap", this returns the pixel of bitmap at the point ("x", "y").
 */
static inline pixel_t* pixel_at(bitmap_t& bitmap, unsigned int x, unsigned int y)
{
    return bitmap.pixels + bitmap.width*y + x;
}

/**
 * Write "bitmap" to a PNG file specified by "path"; returns 0 on success, non-zero on error.
 */
static int save_png_to_file (bitmap_t& bitmap, string& path)
{
    FILE* fp;
    png_structp png_ptr = nullptr;
    png_infop info_ptr = nullptr;
    png_byte** row_pointers = nullptr;

    /* "status" contains the return value of this function. At first
       it is set to a value which means 'failure'. When the routine
       has finished its work, it is set to a value which means
       'success'. */
    int status = -1;

    /* The following number is set by trial and error only. I cannot
       see where it it is documented in the libpng manual.
    */
    int pixel_size = 3;
    int depth = 8;

    fp = fopen(path.c_str(), "wb");
    if(!fp)
    {
        goto fopen_failed;
    }

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if(png_ptr == nullptr)
    {
        goto png_create_write_struct_failed;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if(info_ptr == nullptr)
    {
        goto png_create_info_struct_failed;
    }

    /* Set up error handling. */

    if(setjmp(png_jmpbuf(png_ptr)))
    {
        goto png_failure;
    }

    /* Set image attributes. */

    png_set_IHDR( png_ptr,
                  info_ptr,
                  bitmap.width,
                  bitmap.height,
                  depth,
                  PNG_COLOR_TYPE_RGB,
                  PNG_INTERLACE_NONE,
                  PNG_COMPRESSION_TYPE_DEFAULT,
                  (png_uint_32)PNG_FILTER_TYPE_DEFAULT);

    /* Initialize rows of PNG. */

    row_pointers = (png_byte**)png_malloc(png_ptr, bitmap.height * sizeof(png_byte*));
    for(unsigned int y = 0; y < bitmap.height; y++)
    {
        png_byte* row = (png_byte*)png_malloc(png_ptr, sizeof(uint8_t) * bitmap.width * pixel_size);
        row_pointers[y] = row;
        for(unsigned int x = 0; x < bitmap.width; x++)
        {
            pixel_t* pixel = pixel_at(bitmap, x, y);
            *row++ = pixel->red;
            *row++ = pixel->green;
            *row++ = pixel->blue;
        }
    }

    /* Write the image data to "fp". */

    png_init_io(png_ptr, fp);
    png_set_rows(png_ptr, info_ptr, row_pointers);
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);

    /* The routine has successfully written the file, so we set
       "status" to a value which indicates success. */
    status = 0;

    for(unsigned int y = 0; y < bitmap.height; y++)
    {
        png_free (png_ptr, row_pointers[y]);
    }
    png_free (png_ptr, row_pointers);

 png_failure:
 png_create_info_struct_failed:
    png_destroy_write_struct(&png_ptr, &info_ptr);
 png_create_write_struct_failed:
    fclose(fp);
 fopen_failed:
    return status;
}

void saveBWToFile(uint8_t* pixels, unsigned int w, unsigned int h, string& fpath)
{
    bitmap_t bwmap;

    bwmap.width = w;
    bwmap.height = h;

    bwmap.pixels = (pixel_t*)malloc(sizeof(pixel_t) * bwmap.width * bwmap.height);

    for(unsigned int y=0, t=0; y<bwmap.height; y++)
    {
        for(unsigned int x=0; x<bwmap.width; x++, t++)
        {
            pixel_t* pixel = pixel_at(bwmap, x, y);
            pixel->red = pixels[t];
            pixel->green = pixels[t];
            pixel->blue = pixels[t];
        }
    }

    save_png_to_file(bwmap, fpath);
}
