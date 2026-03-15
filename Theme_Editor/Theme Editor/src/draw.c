#include "draw.h"

#include "charmap_8x12.c"

inline void setPixel(int x, int y, Color c) {
  if(is_cx) {
    unsigned short* p = (unsigned short*)(SCREEN_BASE_ADDRESS + (x << 1) + (y << 9) + (y << 7));
    *p = ((c.b) >> 3) | (((c.g) >> 2) << 5) | (((c.r) >> 3) << 11);
  }
  else {
    unsigned char* p = (unsigned char*)(SCREEN_BASE_ADDRESS  + ((x >> 1) + (y << 7) + (y << 5)));
    *p = (x & 1) ? ((*p & 0xF0) | c.grey) : ((*p & 0x0F) | (c.grey << 4));
  }
}

void clrscr(void) {
  if(is_cx)
    memset(SCREEN_BASE_ADDRESS, 0x00, SCREEN_BYTES_SIZE);
  else
    memset(SCREEN_BASE_ADDRESS, 0xFF, SCREEN_BYTES_SIZE);
}


void putChar(int x, int y, char ch, Color c, int size)
{
  int i, j, k1, k2;
  for(i = 0; i < CHAR_HEIGHT; ++i) {
    unsigned char code = charMap_ascii[(unsigned char)ch][i];
    for(j = CHAR_WIDTH; j >= 0; --j, code >>= 1)
      if (code & 1)
	for(k1 = 0; k1 < size; ++k1)
	  for(k2 = 0; k2 < size; ++k2)
	    setPixel(x+j*size + k1, y+i*size + k2, c); 
  }
}

void fillRect(int x, int y, int w, int h, Color c) {
  int i, j;
  for(i = x; i < x+w; ++i)
    for(j = y; j < y+h; ++j)
      setPixel(i, j, c);
}

void drawRect(int x, int y, int w, int h, int s, Color c) {
  int i, j;
  for(i = x; i < x+w; ++i)
    for(j = y; j < y+s; ++j)
      setPixel(i, j, c);
  for(i = x; i < x+w; ++i)
    for(j = y+h; j > y+h-s; --j)
      setPixel(i, j, c);
  for(j = y; j < y+h; ++j)
    for(i = x; i < x+s; ++i)
      setPixel(i, j, c);
  for(j = y; j < y+h; ++j)
    for(i = x+w; i > x+w-s; --i)
      setPixel(i, j, c);
}

void drawString(char * string, int xpos, int ypos, Color c, int size) {
  int i;
  for(i = 0; string[i] && i < 36; ++i)
    putChar(xpos+(CHAR_WIDTH)*size*i, ypos, string[i], c, size);
}