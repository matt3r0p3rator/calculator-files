#include "image.h"

int jpeg_readDataHeader(dataheader* header, FILE* fp);
int read_jpeg(unsigned short int* gray_image, dataheader* dheader);
