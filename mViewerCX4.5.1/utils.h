#ifndef _UTILS_H_
#define _UTILS_H_

#include "console.h"

#include "png.h"
#include "os.h"
#include "libndls.h"
#include "tools.h"

#define DEBUG(s) disp(s, 1,0)


double pow(double base, double exponent);
void custom_error(png_structp pngs, png_const_charp str);
void custom_warning(png_structp pngs, png_const_charp str);
double fabs(double x);

#endif

