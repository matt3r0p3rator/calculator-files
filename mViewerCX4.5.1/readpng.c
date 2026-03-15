#include "zlib.h"
#include "png.h"
#include "utils.h"
#include "image.h"
#include "readpng.h"

png_structp png_ptr;
png_infop info_ptr;

int png_readDataHeader(dataheader* header, FILE* fp)
{   //DEBUG("Creating read structure\n");
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, custom_error, custom_warning);
    if (png_ptr == NULL)
    {
        return 0;
    }
    //DEBUG("Creating info structure\n");
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
    {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return 0;
    }
    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK);
    png_read_info(png_ptr, info_ptr);

    //Setting transforms
    header->height = png_get_image_height(png_ptr, info_ptr);
    header->width = png_get_image_width(png_ptr, info_ptr);

	// grayscale to RGB
	if(!(png_get_color_type(png_ptr, info_ptr)&PNG_COLOR_MASK_COLOR))
			png_set_gray_to_rgb(png_ptr);
	// indexed to RGB
	else if(png_get_color_type(png_ptr, info_ptr)&PNG_COLOR_MASK_PALETTE)
			png_set_palette_to_rgb (png_ptr);

	// remove indexed transparency
	if (png_get_valid(png_ptr, info_ptr,PNG_INFO_tRNS))
	{	png_set_tRNS_to_alpha(png_ptr);
		png_set_strip_alpha(png_ptr);
	}
	// remove grayscale/RGV transparency
	else if(png_get_color_type(png_ptr, info_ptr)&PNG_COLOR_MASK_ALPHA)
		png_set_strip_alpha(png_ptr);
	
    //Setting 8 bits per pixel
    if(png_get_bit_depth(png_ptr, info_ptr) < 8)
    {   if(!(png_get_color_type(png_ptr, info_ptr)&PNG_COLOR_MASK_COLOR))
			png_set_expand_gray_1_2_4_to_8(png_ptr);
		else
			png_set_packing(png_ptr);
    }
    else if (png_get_bit_depth(png_ptr, info_ptr) >= 16)
        png_set_strip_16(png_ptr);

    header->bits = png_get_bit_depth(png_ptr, info_ptr);
    if(png_get_interlace_type(png_ptr, info_ptr)) header->compression = 1;

    png_read_update_info(png_ptr, info_ptr);

    return png_get_bit_depth(png_ptr, info_ptr)==8;
}

int read_png(unsigned short int* gray_image, dataheader* dheader)
{	int i, j;
	unsigned char* buffer;
	if(dheader->compression)
	{	int firstlines=dheader->height*2/3;
		unsigned char* rgbimage1=(unsigned char*) gray_image;
		unsigned char* buffer=(unsigned char*) malloc((dheader->height-firstlines)*dheader->width*3);
		if(!buffer) return 0;
		png_bytep rows[dheader->height];
		for(i = 0; i < firstlines; i ++)
			rows[i]=rgbimage1+i*dheader->width*3;
		for(i = firstlines; i < dheader->height; i ++)
			rows[i]=buffer+(i-firstlines)*dheader->width*3;
		png_read_image(png_ptr, rows);
		//Transforming
		for(i=0;i<dheader->height;i++)
		{	for(j=0;j<dheader->width;j++)
			{	int red, green, blue;
				if(i<firstlines)
				{	red=rgbimage1[(i*dheader->width+j)*3];
					green=rgbimage1[(i*dheader->width+j)*3+1];
					blue=rgbimage1[(i*dheader->width+j)*3+2];
				}
				else
				{	red=buffer[((i-firstlines)*dheader->width+j)*3];
					green=buffer[((i-firstlines)*dheader->width+j)*3+1];
					blue=buffer[((i-firstlines)*dheader->width+j)*3+2];
				}
				red=red>>3;
				blue=blue>>3;
				green=green>>(has_colors?2:3);
				int color;
				if(has_colors)	color=(red<<11)|(green<<5)|blue;
				else		color=(~((red*30+green*59+blue*11)/100/3))<<1;
				gray_image[(i*dheader->width+j)]=color;
			}
			updateProgress(((i+1)*100)/dheader->height);
		}
	}
	else
	{	unsigned char* buffer=(unsigned char*) malloc(dheader->width*3);
		//Transforming
		for(i=0;i<dheader->height;i++)
		{	png_read_row(png_ptr,buffer,NULL);
			for(j=0;j<dheader->width;j++)
			{	int red, green, blue;
				red=buffer[j*3];
				green=buffer[j*3+1];
				blue=buffer[j*3+2];
				red=red>>3;
				blue=blue>>3;
				green=green>>(has_colors?2:3);
				int color;
				if(has_colors)	color=(red<<11)|(green<<5)|blue;
				else		color=(~((red*30+green*59+blue*11)/100/3))<<1;
				gray_image[(i*dheader->width+j)]=color;
			}
			updateProgress(((i+1)*100)/dheader->height);
		}
	}
	png_read_end(png_ptr, NULL);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	free(buffer);
	return 1;
}