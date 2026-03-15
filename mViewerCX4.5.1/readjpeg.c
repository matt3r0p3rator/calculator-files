#include "zlib.h"
#include "utils.h"
#include "jpeglib.h"
#include "image.h"
#include "readjpeg.h"
#include <setjmp.h>

struct my_error_mgr {
  struct jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
};
typedef struct my_error_mgr * my_error_ptr;

METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

//  (*cinfo->err->output_message) (cinfo);

  longjmp(myerr->setjmp_buffer, 1);
}

struct jpeg_decompress_struct cinfo;
struct my_error_mgr jerr;

int jpeg_readDataHeader(dataheader* header, FILE* fp)
{	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	/* Establish the setjmp return context for my_error_exit to use. */
	  if (setjmp(jerr.setjmp_buffer)) {
	    /* If we get here, the JPEG code has signaled an error.
	     * We need to clean up the JPEG object, close the input file, and return.
	     */
	    jpeg_destroy_decompress(&cinfo);
	    return 0;
	  }
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, fp);
	(void) jpeg_read_header(&cinfo, TRUE);
	header->height = cinfo.image_height;
	header->width = cinfo.image_width;
	header->bits = cinfo.num_components*8;
	cinfo.out_color_space=JCS_RGB;
	cinfo.out_color_components=3;
	cinfo.output_components=3;
	return 1;
}

int read_jpeg(unsigned short int* gray_image, dataheader* dheader)
{	int i, j;
	/* Establish the setjmp return context for my_error_exit to use. */
	  if (setjmp(jerr.setjmp_buffer)) {
	    /* If we get here, the JPEG code has signaled an error.
	     * We need to clean up the JPEG object, close the input file, and return.
	     */
	    jpeg_destroy_decompress(&cinfo);
	    return 0;
	  }
	(void) jpeg_start_decompress(&cinfo);
	JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, cinfo.output_width * cinfo.output_components, 1);
	while(cinfo.output_scanline<cinfo.output_height)
	{   	jpeg_read_scanlines(&cinfo, buffer,1);
		for(j=0;j<dheader->width;j++)
		{	int red, green, blue;
			red=buffer[0][j*3];
			green=buffer[0][j*3+1];
			blue=buffer[0][j*3+2];
			red=red>>3;
			blue=blue>>3;
			green=green>>(has_colors?2:3);
			int color;
			if(has_colors)	color=(red<<11)|(green<<5)|blue;
			else		color=(~((red*30+green*59+blue*11)/100/3))<<1;
			gray_image[((cinfo.output_scanline-1)*dheader->width+j)]=color;
		}
		updateProgress((cinfo.output_scanline*100)/dheader->height);
	}
    //Finishing
    (void) jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    return 1;
}