#include "color.h"

Color new_Color() {
  Color c = (Color) 0;
  c.grey = 0xF;
  return c;
}

Color new_Color_RGB(char r, char g, char b) {
  Color c = new_Color();
  c.r = r; c.g = g; c.b = b;
  c.grey = (((255-r) + ((255-g)<<1) + (255-b))>>6) & 0xF;
  return c;
}

Color Color_(int color) {
  Color c = new_Color();
  c.r = (color >> 16) & 0xFF;
  c.g = (color >> 8) & 0xFF;
  c.b = (color) & 0xFF;
  c.grey = ((c.r + (c.g<<1) + c.b)>>6) & 0xF;
  return c;
}

int _Color(Color color) {
  color.grey = 0;
  return color.color;
}

Color RGB2Color(char r, char g, char b) {
  Color c = new_Color();
  c.r = r; c.g = g; c.b = b;
  c.grey = ((r + (g<<1) + b)>>6) & 0xF;
  return c;
}