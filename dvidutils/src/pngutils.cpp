#include <cstdlib>
#include <vector>

#include <png.h>
#include "pngutils.hpp"

// Reads an 8 bit png file.
uint8* read_8bit_png_file(const char *file_name, int &w, int &h)
{
    FILE *fp = fopen(file_name, "rb");
    if (!fp) {
        return NULL;
    }
    const int number=8;  // read this many bytes
    png_byte header[number];
    fread(header, 1, number, fp);
    bool is_png = !png_sig_cmp(header, 0, number);
    if (!is_png) {
        return NULL;  // not a .png file
    }
    
    // create the structures
    png_structp png_ptr = png_create_read_struct
    (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL); // use default error handlers
    if (!png_ptr)
        return NULL;
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        return NULL;
    }
    
    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, number);
    
    png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_SWAP_ENDIAN, NULL);
    fclose(fp);
    
    // now get some info about the
    int bit_depth, color_type;
    png_uint_32 width, height;
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);
    printf("width %d, height %d, bit depth %d, color_type %d\n", width, height, bit_depth, color_type);
    
    // get the pointers to each row.
    uint8* rslt = (uint8 *)malloc(height*width*sizeof(uint8));
    if (bit_depth == 8){
        png_byte **row_ptrs = (png_byte**)png_get_rows(png_ptr, info_ptr);
        int n=0;
        for(int y=0; y<height; y++)
            for(int x=0; x<width; x++)
                rslt[n++] = row_ptrs[y][x];
    }
    else {
        printf("Trying 8 bit read; Cannot read .png file '%s' with bit depth %d\n", file_name, bit_depth);
        free(rslt);
        rslt = NULL;
    }
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);  // we've made a copy, so free original
    w = width;   // returned values.  Just to make types 'int' for convenience
    h = height;
    return rslt;
}

// Writes an in-memory raster as an 8 bit .png file
void write_8bit_png_file(const char* file_name, unsigned char *raster, int width, int  height)
{
    /* create file */
    FILE *fp = fopen(file_name, "w");
    if (fp == NULL) {
        printf("Open for write of 8-bit '%s' failed\n", file_name);
        exit(-1);
    }
    
    /* initialize stuff */
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop  info_ptr = png_create_info_struct(png_ptr);
    png_init_io(png_ptr, fp);
    
    png_byte bit_depth = 8;
    png_byte color_type = PNG_COLOR_TYPE_GRAY;
    png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, color_type, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_write_info(png_ptr, info_ptr);
    
    // create the row pointers
    std::vector<png_byte *> row_pointers(height);
    for(int i=0; i<height; i++)
        row_pointers[i] = (png_byte *)(raster+i*width);
    png_write_image(png_ptr, &row_pointers[0]);
    
    png_write_end(png_ptr, NULL);
    fclose(fp);
}
