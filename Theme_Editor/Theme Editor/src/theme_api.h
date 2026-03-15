#ifndef THEME_API_H_
# define THEME_API_H_

# include <os.h>
# include "libndls.h"
# include "syscall.h"
# include "color.h"

unsigned num_symbols;

inline Color getNthColor_mode(int i, int cx);
inline void setNthColor_mode(int i, int c, int cx);
inline Color getNthColor(int i);
inline void setNthColor(int i, int c);
inline char *getNthSymbol(int i);
int hex2int(char * s);
int readTheme(const char * theme_path);
void writeTheme(const char * theme_path);

#endif /* !THEME_API_H_ */