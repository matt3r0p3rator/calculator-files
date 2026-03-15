#include "image.h"
#include "console.h"
#include "tools.h"
#include "charmaps.h"
#include "zlib.h"
#include "png.h"
#include "readpng.h"
#include "screen.h"

void dispHeader(fileheader* fheader, dataheader* header)
{	displn("",0,1);
	displn("",0,1);
	displn("",0,1);
	displn("",0,1);
	displn("",0,1);
	displn("",0,1);
	displn("",0,1);
	if(!fheader->type)
		displn("Type        : unknown",0,1);
	else
	{ if(fheader->type)
	  {
/*		disp("File Header size : ",0,1);
		dispi(fheader->size,0,1);
		displn("",0,1);
		if(fheader->type==1)
		{	disp("Data Header size: ",0,1);
			dispi(header->size,0,1);
			displn("",0,1);
		}
*/
		disp("Width         : ",0,1);
		dispi(header->width,0,1);
		displn("",0,1);
		disp("Height        : ",0,1);
		dispi(header->height,0,1);
		displn("",0,1);
		disp("RAM needed    : ",0,1);
		int i=0;
		int size = (header->width)*(header->height)*2;
		if(size>=1024) { size=size/1024; i++; }
		if(size>=1024) { size=size/1024; i++; }
		dispi(size,0,1);
		if(i==2)	displn(" MB",0,1);
		else if(i==1)	displn(" KB",0,1);
		else		displn(" B",0,1);
		displn("",0,1);
		if(fheader->type)
		  {	disp("Type          : ",0,1);
			if(fheader->type==1)
	 		 	displn("BMP",0,1);
			else if(fheader->type==2)
				displn("PNG",0,1);
			else if(fheader->type==3)
				displn("JPEG",0,1);
		  }
/*		if(fheader->type==1)
		{	disp("Planes          : ",0,1);
			dispi(header->planes,0,1);
			displn("",0,1);
		}
*/
		disp("Bits          : ",0,1);
		dispi(header->bits,0,1);
		displn("",0,1);
/*		disp("Uncompressed size: ",0,1);
		dispi((header->width)*(header->height)*(header->bits)/8,0,1);
		displn("",0,1);
*/

		if(fheader->type==1)
		{	disp("Compression   : ",0,1);
			if(!header->compression)		displn("none",0,1);
			else if(header->compression==1)	displn("RLE",0,1);
			else						displn("unknown",0,1);
/*			disp("Data size     : ",0,1);
			dispi(header->sourcedatasize,0,1);
			displn("",0,1);
*/
		}
		if(fheader->type==2)
		{	disp("Interlacing   : ",0,1);
			if(!header->compression)		displn("none",0,1);
			else if(header->compression==1)	displn("ADAM7",0,1);
			else						displn("unknown",0,1);
		}
		disp("Indexed colors: ",0,1);
		dispi(header->indexedcolors,0,1);
		displn("",0,1);
/*		if(fheader->type==1)
		{	disp("Red Mask        : ",0,1);
			dispi(header->redmask,0,1);
			displn("",0,1);
			disp("Green Mask      : ",0,1);
			dispi(header->greenmask,0,1);
			displn("",0,1);
			disp("Blue Mask       : ",0,1);
			dispi(header->bluemask,0,1);
			displn("",0,1);
			disp("Alpha Mask      : ",0,1);
			dispi(header->alphamask,0,1);
			displn("",0,1);
			disp("Red Bits        : ",0,1);
			dispi(header->redbits,0,1);
			displn("",0,1);
			disp("Green Bits      : ",0,1);
			dispi(header->greenbits,0,1);
			displn("",0,1);
			disp("Blue Bits       : ",0,1);
			dispi(header->bluebits,0,1);
			displn("",0,1);
			disp("Alpha Bits      : ",0,1);
			dispi(header->alphabits,0,1);
			displn("",0,1);
		}
*/
	  }
	}
}

int readFileHeader(fileheader* header, FILE* file)
{	unsigned char data[MAX_FILEHEADER_SIZE];
	memset(header,0,sizeof(fileheader));
	int read=0;
	read+=fread(data, 1, 2, file);
	if(read==2)
	{	int magic2=readhexint(data,2,0);
		if(	magic2==0x424D || magic2==0x4241 || magic2==0x4349
			|| magic2==0x4350 || magic2==0x4943 || magic2==0x5054 )
		{	header->type=1;
			header->size=14;
		}
		else if(magic2==0xFFD8)
		{	header->type=3;
			header->size=2;
			fseek(file,0,SEEK_SET);
		}
		else
		{	read+=fread(data+2,1,PNG_BYTES_TO_CHECK-2,file);
			if(!png_sig_cmp(data, (png_size_t)0, PNG_BYTES_TO_CHECK))
			{	header->type=2;
				header->size=PNG_BYTES_TO_CHECK;
			}
		}
	}
	if(header->type) read+=fread(data+read,1,header->size-read,file);
	return header->type && read==header->size;
}

int readDataHeader(dataheader* header, FILE* file, fileheader* fheader)	
{	unsigned char data[MAX_DATAHEADER_SIZE];
	memset(header,0,sizeof(dataheader));
	int read=0;
	int valid=0;
	if(fheader->type==1)
	{	read+=fread(data,1,4,file);
		header->invertedlines=1;
		if(read==4)
		{	header->size=readhexint(data,4,1);
			if(header->size<=MAX_DATAHEADER_SIZE)
			{	read+=fread(data+read,1,header->size-read,file);
				if(read>=40)
				{	header->width=readhexint(data+4,4,1);
					header->height=readhexint(data+8,4,1);
					if(header->height<0)	header->height=-header->height;
					else			header->invertedlines=1;
					header->planes=readhexint(data+12,2,1);
					header->bits=readhexint(data+14,2,1);
					header->compression=readhexint(data+16,4,1);
					// 0 =	none
					// 1 =	RLE 8-bits
					// 2 =	RLE 4-bits
					// 3 =	compression Huffman1D si le header est un BITMAPCOREHEADER2
					//	sinon, compression par utilisation de bitmask, ce qui est géré par défaut
					// 4 =	JPEG (dans un BMP ? . . .)
					// 5 =	PNG (dans un BMP ? . . .)
					if(header->compression==3 && header->size!=64) header->compression=0; 
					if(header->compression==2) header->compression=1; 
					header->rawlinesize=header->bits*header->width/8;
					if(fheader->type==1)	// padding pour les BMP
						if(header->rawlinesize%4)
							header->rawlinesize+=4-(header->rawlinesize%4);
					if(!header->compression)
					{	header->sourcedatasize=header->rawlinesize*header->height;
					}
					else	header->sourcedatasize=readhexint(data+20,4,1);
					if(header->bits<=8) // présence d'une palette
					{	header->indexedcolors=readhexint(data+32,4,1);
						if(!header->indexedcolors)
							header->indexedcolors=1<<header->bits;
					}
					if(read>=44)
						header->redmask=readhexint(data+40,4,1);
					else
					{	if(header->bits>=24 || header->bits<=8) header->redmask=0x00FF0000;
						if(header->bits==16) header->redmask=0x00007C00;
					}
					if(read>=48)
						header->greenmask=readhexint(data+44,4,1);
					else
					{	if(header->bits>=24 || header->bits<=8) header->greenmask=0x0000FF00;
						if(header->bits==16) header->greenmask=0x000003E0;
					}
					if(read>=52)
						header->bluemask=readhexint(data+48,4,1);
					else
					{	if(header->bits>=24 || header->bits<=8) header->bluemask=0x000000FF;
						if(header->bits==16) header->bluemask=0x0000001F;
					}
					if(read>=56)
						header->alphamask=readhexint(data+52,4,1);
					else
						if(header->bits==32) header->alphamask=0xFF000000;
					header->redbits=nbbits(header->redmask);
					header->greenbits=nbbits(header->greenmask);
					header->bluebits=nbbits(header->bluemask);
					header->alphabits=nbbits(header->alphamask);

					valid=1;
				}
			}
		}
	}
	else if(fheader->type==2)
	{	return png_readDataHeader(header, file);
	}
	else if(fheader->type==3)
	{	return jpeg_readDataHeader(header, file);
	}
	return valid && read==header->size;
}

void readPalette(int* palette, dataheader* dheader, FILE* img, fileheader* fheader)
{	int i;
	unsigned char* data;
	data=(unsigned char*) malloc(dheader->indexedcolors*4);
	int colorsize = 4;
	// OS2 BITMAPCOREHEADER -> palette sur 3 octets
	if(fheader->type==1 && dheader->size==12) colorsize=3;
	fread(data,1,dheader->indexedcolors*colorsize,img);
	for(i=0;i<dheader->indexedcolors;i++)
		palette[i]=readhexint(data+i*colorsize,3,1);
	free(data);
}

void setImgPixel(unsigned short int* image, int y, int x, int color, dataheader* dheader)
{	if(dheader->invertedlines)
		image[((dheader->height-1-y)*dheader->width+x)]=color;
	else	image[(y*dheader->width+x)]=color;

/*	if(dheader->invertedlines)
		image[((dheader->height-1-y)*dheader->width+x)*(has_colors?2:1)]=color&0b11111111;
	else	image[(y*dheader->width+x)*(has_colors?2:1)]=color&0b11111111;
	if(has_colors)
	{	if(dheader->invertedlines)
			image[((dheader->height-1-y)*dheader->width+x)*2+1]=(color&0b1111111100000000)>>8;
		else	image[(y*dheader->width+x)*2+1]=(color&0b1111111100000000)>>8;
	}*/
}

int px=-1,py=-1;

void updateProgress(int p)
{	if(px>=0)
	{	char tmp[4];
		sprintf(tmp,"%d",p);
		drwBufStr(getScreen(),px*CHAR_WIDTH,py*CHAR_HEIGHT,tmp,0,0);
	}
}

void initImgProgress(int x, int y, int p)
{	px=x;
	py=y;
	drwBufStr(getScreen(),(x+3)*CHAR_WIDTH,y*CHAR_HEIGHT,"%",0,1);
	updateProgress(p);
}

void endImgProgress()
{	px=-1;
	py=-1;
}

unsigned short int makeData(int color,dataheader* dheader)
{	int data;
	int r=maskright(color,dheader->redmask);
	int g=maskright(color,dheader->greenmask);
	int b=maskright(color,dheader->bluemask);
	int a=maskright(color,dheader->alphamask);
	int sr=(dheader->redbits-(has_colors?5:5));
	int sg=(dheader->greenbits-(has_colors?6:5));
	int sb=(dheader->bluebits-(has_colors?5:5));
	unsigned int alphamax=maskright(dheader->alphamask,dheader->alphamask);
	if(sr) r=r>>sr;
	if(sg>0)	g=g>>sg;
	else		g=g<<(-sg);
	if(sb) b=b>>sb;
	if(!has_colors)
	{//	data=(r*30+g*59+b*11)/100;
		data=(~((r*30+g*59+b*11)/100/3))<<1;
//		if(alphamax) data=32-((32-data)*a/alphamax);
	}
	else
	{	data=(r<<11)|(g<<5)|b;
	}
	return data;
}

int readRLEImage(unsigned short int* image, dataheader* dheader, int* palette, FILE* img)
{	//memset(image,0,dheader->width*dheader->height);
	unsigned char* rawdatabuf = (char*) malloc(dheader->sourcedatasize);
	if(!rawdatabuf) return 0;
	unsigned int i=0,j,y=0,x=0, stop=0;
	if( fread(rawdatabuf, 1, dheader->sourcedatasize, img)==dheader->sourcedatasize )
	{	unsigned int color, ccolor, data;
		int n = 8/dheader->bits, t;
		if(!n) n=1;
		unsigned int* tmp = (unsigned int*) malloc(n*sizeof(unsigned int));
		if(tmp)
		{	unsigned char mask=0;
			int pixbytes; // bytes to read for each pixel
			if(dheader->bits<8) pixbytes=1;
			else pixbytes=dheader->bits/8;
			while(!stop && i<dheader->sourcedatasize && y<dheader->height)
			{
				if(rawdatabuf[i]==0)
				{	if(rawdatabuf[i+1]==0)	{ x=0;y++; if(py>=0) updateProgress(y*100/dheader->height);}
					else if(rawdatabuf[i+1]==1) stop=1;
					else if(rawdatabuf[i+1]==2)	
					{	x+=rawdatabuf[i+2];
						y+=rawdatabuf[i+3];
					}
					else
					{	mask=0;
						for(j=0;j<rawdatabuf[i+1];j++)
						{	color=readhexint(rawdatabuf+i+2+j*dheader->bits/8,pixbytes,1);
							if(dheader->bits<8) //rotation du masque de lecture pour les données de moins de 8-bits
							{	mask=mask>>dheader->bits;
								if(!mask)	mask=(~0)<<(8-dheader->bits);
								color=maskright(color,mask);
							}
							if(dheader->indexedcolors) color=palette[color];
							data = makeData(color,dheader);
							setImgPixel(image,y,x,data,dheader);
							x++;
						}
						t=rawdatabuf[i+1]%(16/dheader->bits);
						if(dheader->bits<=8 && t)
						{	i+=(rawdatabuf[i+1]+16/dheader->bits-t)*dheader->bits/8;
						}
						else	i+=rawdatabuf[i+1]*dheader->bits/8;
					}
					i+=2;	
				}
				else
				{	ccolor=readhexint(rawdatabuf+i+1,pixbytes,1);
					color=ccolor;
					mask=(~0)<<(8-dheader->bits);
					for(j=0;j<n;j++)
					{	if(dheader->bits<8) //rotation du masque de lecture pour les données de moins de 8-bits
						{	color=maskright(ccolor,mask);
							mask=mask>>dheader->bits;
						}
						if(dheader->indexedcolors) color=palette[color];
						data = makeData(color,dheader);
						tmp[j]=data;
					}
					for(j=0;j<rawdatabuf[i];j++)
					{	setImgPixel(image,y,x,tmp[j%n],dheader);
						x++;
					}
					i+=1+pixbytes;
					if(!(pixbytes%2)) i++;
				}
			}
			free(tmp);
			if(!stop && i<dheader->sourcedatasize-1)
			{	if(rawdatabuf[i]==0 && rawdatabuf[i+1]==1)
				{	stop=1;
					if(i<dheader->sourcedatasize-1) i+=2;
					if(y<dheader->height) y++;
				}
			}
			if(y<dheader->height && i<dheader->sourcedatasize-1)
			{	if(rawdatabuf[i]==0 && rawdatabuf[i+1]==0)
				{	x=0;
					if(i<dheader->sourcedatasize-1) i+=2;
					if(y<dheader->height) y++;			
				}
			}
		}
	}
	free(rawdatabuf);
	return stop && i==dheader->sourcedatasize;
}

int readRAWImage(unsigned short int* image, dataheader* dheader, int* palette, FILE* img)
{	int i,j;
	unsigned char* rawdatabuf = (char*) malloc(dheader->sourcedatasize);
	if(!rawdatabuf) return 0;
	if( fread(rawdatabuf, 1, dheader->sourcedatasize, img)==dheader->sourcedatasize )
	{	unsigned int color, data;
		int pixbytes; // bytes to read for each pixel
		if(dheader->bits<8) pixbytes=1;
		else pixbytes=dheader->bits/8;
		unsigned char mask=0;
		for (i = 0; i < dheader->height; i++) {
			for(j=0; j < dheader->width; j++)
			{	color=readhexint(rawdatabuf+dheader->rawlinesize*i+dheader->bits*j/8,pixbytes,1);
				if(dheader->bits<8) //rotation du masque de lecture pour les données de moins de 8-bits
				{	mask=mask>>dheader->bits;
					if(!mask)	mask=(~0)<<(8-dheader->bits);
					color=maskright(color,mask);
				}
				if(dheader->indexedcolors) color=palette[color];
				data = makeData(color,dheader);
				setImgPixel(image,i,j,data,dheader);
			}
			if(py>=0) updateProgress(i*100/dheader->height);
		}
	}
	free(rawdatabuf);
	return 1;
}

int readImage(unsigned short int* image, dataheader* dheader, int* palette, FILE* img, fileheader* fheader)
{	if(fheader->type==1 && dheader->compression==0)
		return readRAWImage(image,dheader,palette,img);
	if(fheader->type==1 && dheader->compression==1)
		return readRLEImage(image,dheader,palette,img);
	if(fheader->type==2)
		return read_png(image, dheader);
	if(fheader->type==3)
		return read_jpeg(image, dheader);
	return 0;
}