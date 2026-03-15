#ifndef COLOR_H_
# define COLOR_H_

typedef union {
  struct {
    char b;
    char g;
    char r;
    char grey;
  };
  int color;
} Color;

Color new_Color();
Color new_Color_RGB(char r, char g, char b);
Color Color_(int color);
int _Color(Color color);
Color RGB2Color(char r, char g, char b);

#endif /* !COLOR_H_ */