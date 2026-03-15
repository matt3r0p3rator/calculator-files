#ifndef DRAW_H_
# define DRAW_H_

# include <os.h>
# include "libndls.h"
# include "charmap_8x12.h"
# include "color.h"

inline void setPixel(int x, int y, Color c);
void clrscr(void);
void putChar(int x, int y, char ch, Color c, int size);
void fillRect(int x, int y, int w, int h, Color c);
void drawRect(int x, int y, int w, int h, int s, Color c);
void drawString(char * string, int xpos, int ypos, Color c, int size);

#endif /* !DRAW_H_ */