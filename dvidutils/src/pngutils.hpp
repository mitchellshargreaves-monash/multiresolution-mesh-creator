
typedef unsigned char uint8;

uint8* read_8bit_png_file(const char *file_name, int &w, int &h);
void write_8bit_png_file(const char* file_name, unsigned char *raster, int width, int  height);
