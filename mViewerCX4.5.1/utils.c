#include "zlib.h"
#include "utils.h"


double pow(double base, double exponent)
{
    //Dummy version for now...
    return 1.f;
}

void custom_error(png_structp pngs, png_const_charp str)
{
    disp((char*)str, 1,0);
    abort();
}

void custom_warning(png_structp pngs, png_const_charp str)
{
    disp((char*)str, 1,0);
}

double fabs(double x)
{
    return x > 0 ? x : -x;
}