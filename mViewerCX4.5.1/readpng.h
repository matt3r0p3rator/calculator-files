#include "image.h"

#define PNG_BYTES_TO_CHECK 4

int png_readDataHeader(dataheader* header, FILE* fp);
int read_png(unsigned short int* gray_image, dataheader* dheader);
