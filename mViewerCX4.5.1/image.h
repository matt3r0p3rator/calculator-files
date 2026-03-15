#ifndef image_h

#define image_h


#include <os.h>
#define MAX_FILEHEADER_SIZE 14
#define MAX_DATAHEADER_SIZE 124

typedef struct {
	int size;
	int type; //0=unknown 1=BMP
} fileheader;

typedef struct {
	int size;
	int width;
	int height;
	int planes;
	int bits; // 1 2 4 8 16 24 32
	int compression; //0=none
	int sourcedatasize;
	int rawlinesize;
	int indexedcolors; // 0 = no palette
	int redmask;
	int greenmask;
	int bluemask;
	int alphamask;
	int redbits;
	int greenbits;
	int bluebits;
	int alphabits;
	int invertedlines;
} dataheader;

void dispHeader(fileheader* dheader, dataheader* header);
int readFileHeader(fileheader* header, FILE* file);
int readDataHeader(dataheader* header, FILE* file, fileheader* fheader);
void readPalette(int* palette, dataheader* dheader, FILE* img, fileheader* fheader);
//int readImage(unsigned char* image, dataheader* dheader, int* palette, FILE* img, fileheader* fheader);
void initImgProgress(int x, int y, int p);
void endImgProgress();

#endif