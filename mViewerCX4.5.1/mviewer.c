#include <os.h>
#include "console.h"
#include "dirlist.h"
#include "screen.h"
#include "image.h"
#include "browse.h"
#include "tools.h"
#include "charmaps.h"
#include "touchpad.h"
#include "readpng.h"

#define ESC_DELAY	200

#define SCROLL_WIDTH 6

#define MEM1	0x900B0000
#define MEM2	0x900B000C

#define CONFIG_FILE	"/documents/ndless/mviewer.cfg.tns"
#define PATH_SIZE 1024

char lastpath[PATH_SIZE];

extern unsigned char back[320*240*3];
unsigned short int sscreen[320*240];

float zoomlevels[5];
int nzooms=0;

float zoom;

float zoom_max;
float zoom_min;

void setzoomlimits()
{	int i;
	zoom_max=zoom;
	int f=0;
	for(i=0;i<nzooms && !f;i++)
		if(zoomlevels[i]>zoom)
		{	f=1;
			zoom_max=zoomlevels[i];
		}
	f=0;
	zoom_min=zoom;
	for(i=nzooms-1;i>=0 && !f;i--)
		if(zoomlevels[i]<zoom)
		{	f=1;
			zoom_min=zoomlevels[i];
		}
}

void addzoomlevel(float zoom)
{	int i;
	int ins=-1;
	for(i=0;i<nzooms && ins<0;i++)
		if(zoomlevels[i]>zoom)
			ins=i;
		else if(zoomlevels[i]==zoom)
			return;
	if(ins>=0)
	{	for(i=nzooms-1;i>=ins;i--)
			zoomlevels[i+1]=zoomlevels[i];
		zoomlevels[ins]=zoom;
	}
	else
		zoomlevels[nzooms]=zoom;
	nzooms++;
}

static const unsigned int hook_addr_calc[] = {
	0x1006CBBC, 0x1006CAEC, // ClickPad / TouchPad 3.1
	0x1006C2B8, 0x1006C210, // CX 3.1
	0x10068d0c, 0x10068c64, // CM 3.1
    0x1007c650, 0x1007c5a4, // Clickpad / Touchpad 3.6
    0x1007bc30, 0x1007bbb4, // CX 3.6
    0x1007d350, 0x1007cdb8, // ClickPad / Touchpad 3.9.0
    0, 0, // ClickPad / Touchpad 3.9.1
    0x1007c924, 0x1007c3bc, // CX 3.9.0
    0x1007c948, 0x1007c3e0, // CX 3.9.1
	0x1007d444, 0x1007cef8, // CX 4.0.0
    0x10080b10, 0x100805c4, // CX 4.0.3
    0x100810dc, 0x10080b90, // CX 4.2
    0x10081710, 0x100811b8, // CX 4.3
    0x10083060, 0x10082b18, // CX 4.4
    0x10083af0, 0x100835a4, // CX 4.5
    0x10083d44, 0x100837f4, // CX 4.5.1
};

#define HOOK_ADDR_CALC nl_osvalue((int*)hook_addr_calc, sizeof(hook_addr_calc)/sizeof(hook_addr_calc[0]))

const unsigned char mviewerstring1[] = {0x6D, 0x00, 0x76, 0x00, 0x28, 0x00, 0x29, 0x00, 0x00, 0x00};
const unsigned char mviewerstring2[] = {0x6D, 0x00, 0x76, 0x00, 0x6C, 0x00, 0x28, 0x00, 0x29, 0x00, 0x00, 0x00};
const unsigned char resultstring[]  = {0x00, 0x00};

void loadlast(int truncdir)
{	FILE* h =fopen(CONFIG_FILE,"r");
	*lastpath=0;
	if(h)
	{	int i = fread(lastpath,1,PATH_SIZE,h);
		lastpath[i]=0;
		fclose(h);
	}
	if(!*lastpath)
		strcpy(lastpath,"/documents/");
	if(truncdir) {
		char* p=strrchr(lastpath,'/');
		*(p+1)=0;
	}
}

void savelast()
{	FILE* h =fopen(CONFIG_FILE,"w");
	if(h)
	{	fwrite(lastpath,1,strlen(lastpath),h);
		fclose(h);
	}
}

int last=0;
HOOK_DEFINE(calc) {

    char* calcul = (char *) HOOK_SAVED_REGS(calc)[2];
    // Disable the watchdog on CX that may trigger a reset - code frag from ndless
	if(has_colors)
    {	*(volatile unsigned*)0x90060C00 = 0x1ACCE551; // enable write access to all other watchdog registers
		*(volatile unsigned*)0x90060008 = 0; // disable reset, counter and interrupt
		*(volatile unsigned*)0x90060C00 = 0; // disable write access to all other watchdog registers
	}
    if( !memcmp( calcul, &mviewerstring1, sizeof(mviewerstring1) ) )
    {
		loadlast(1);
        int intmask = TCT_Local_Control_Interrupts(-1);
        main(0, NULL);
        TCT_Local_Control_Interrupts(intmask);
        HOOK_SAVED_REGS(calc)[2] = (int) resultstring;
    }
	else if( !memcmp( calcul, &mviewerstring2, sizeof(mviewerstring2) ) )
    {	loadlast(0);
        int intmask = TCT_Local_Control_Interrupts(-1);
        main(0, NULL);
        TCT_Local_Control_Interrupts(intmask);
        HOOK_SAVED_REGS(calc)[2] = (int) resultstring;
    }
    HOOK_RESTORE_RETURN(calc);
}

int main(int argc, char* argv[])
{	int i;
	if(argc>=2) {
		strcpy(lastpath,argv[1]);
	}
	else if(argc==1)
		loadlast(1);
	if(argv>=1) {
		char* progname=0;
		for(i=strlen(argv[0])-1;i>=0 && !progname;i--)
			if(argv[0][i]=='/')
				progname=argv[0]+i+1;
		if(progname)
		{	for(i=strlen(progname)-1;i>=0;i--)
				if(progname[i]=='.')
				{	progname[i]=0;
					i=0;
				}
			addshellext("jpg",progname);
			addshellext("jpeg",progname);
			addshellext("bmp",progname);
			addshellext("dib",progname);
			addshellext("png",progname);
			addshellext("JPG",progname);
			addshellext("JPEG",progname);
			addshellext("BMP",progname);
			addshellext("DIB",progname);
			addshellext("PNG",progname);
		}
	}
	if(nl_isstartup())
	{	nl_set_resident();
		HOOK_INSTALL(HOOK_ADDR_CALC, calc);
		return 0;
	}
	float zxmax,zymax,zxmin,zymin;
	initScreen();
	startScreen();
	int size=320*240*2;
	unsigned int contrast=getContrast();
	unsigned int ocontrast=contrast;
	unsigned short int* screen = getScreen();
	unsigned short int* baseoffscreen = (unsigned short int*) malloc(SCREEN_WIDTH*SCREEN_HEIGHT*2+(has_colors?7:0));
	unsigned short int* offscreen = baseoffscreen;
	int r;
	if(has_colors)
	{	r = ((unsigned long int)offscreen)%8;
		if(r)
			offscreen=(unsigned short int*)(((unsigned long int)offscreen)+8-r);
	}
	unsigned short int* t;
	fileheader fheader;
	dataheader dheader;
	unsigned short int* image;
	char path[PATH_SIZE];

	int ximg, yimg;
	zoom=1.0;
	strcpy(path, lastpath);

	FILE* img;

	int selected=1;
	dispBufImgRGB(sscreen,0,0,back,320,240,0);
	memcpy(screen,sscreen,size);
	switchScrOffOn(1);
	while(selected)
	{	if(path[strlen(path)-1]=='/') selected=chooseFile(path,path,"  mViewer CX 4.4");
		if(selected)	// choix du fichier non annulé
		{ 
			resetConsole(0);		
			img   = fopen(path, "rb");
			if(readFileHeader(&fheader,img)) {	
				if(fheader.type) { // support BMP / PNG / JPEG
					if(readDataHeader(&dheader,img,&fheader)) {
						if(fheader.type==3 || fheader.type==2 && (dheader.compression==0 || dheader.compression==1) || (fheader.type==1 && (dheader.compression==0 || dheader.compression==1 || dheader.compression==3))) {	// formats gérés
							int* palette=0;
							if(dheader.indexedcolors) {
								palette=(int*)malloc(dheader.indexedcolors*sizeof(int));
								readPalette(palette, &dheader, img, &fheader);
							}
							image = (unsigned short int*) malloc(dheader.width*dheader.height*2);
							if(image) {
								if(fheader.type) initImgProgress(MAX_COL-4,10,0);
								if(readImage(image, &dheader, palette, img, &fheader)) {
									endImgProgress();
									ximg=0; yimg=0;
									zxmax=dheader.width/320.0;		
									zymax=dheader.height/240.0;
									zoom=1.0; //zoom=(zxmax>zymax) ? zxmax : zymax;
									zoom=(zoom > 1.0) ? zoom : 1.0;
									nzooms=0;
									zxmin=zxmax*320.0;
									zymin=zymax*240.0;
									addzoomlevel(0.0125);
									addzoomlevel(1.0);
									addzoomlevel(zxmax);
									addzoomlevel(zymax);
									addzoomlevel((zxmin>zymin) ? zymin : zxmin);
									setzoomlimits();
									clrBuf(offscreen);	
									unsigned int e=1;
									int m=0;
									// Binary-Events e:
									//  * -> refresh
									//  2 -> scroll up
									//  4 -> scroll down
									//  8 -> scroll left
									// 16 -> scroll right
									// 32 -> zoom
									// 64 -> dezoom
									int rstep=0, lstep=0, ustep=0, dstep=0;
									float istep=0.0,ostep=0.0;	// for progressive zoom & scrolling
									int mode=2;	//mode: bit0 = 16/32 colors | bit1 = Screen Off/On
									int torelease=0;	// forbids action repeat before key is released: bit1=keyCTRL | bit2=keyC
									int zwidth=(int)(dheader.width/zoom), zheight=(int)(dheader.height)/zoom;	// actual (zoomed) image dimensions
									int zwidthold=zwidth, zheightold=zheight;	// keeps previous (zoomed) image dimensions
									clrBuf(screen);	
									initTP();
									strcpy(lastpath,path);
									savelast();
									while(!isKeyPressed(KEY_NSPIRE_ESC) && !isKeyPressed(KEY_NSPIRE_LP) && !isKeyPressed(KEY_NSPIRE_RP)) {
										int step=max(1.0F,(int)(1.0F/zoom));
										if(!(e&2)) ustep=step;
										else ustep+=step;
										if(!(e&4)) dstep=step;
										else dstep+=step;
										if(!(e&8)) lstep=step;
										else lstep+=step;
										if(!(e&16)) rstep=step;
										else rstep+=step;
										if(!(e&32)) istep=0.0;
										else istep+=0.01;
										if(!(e&64)) ostep=0.0;
										else ostep+=0.01;
										if(isTouchpad()) {
											wait(TOUCHPAD_DELAY);
											readTP();
										}
										if (isTPTouched() && !isTPPressed()) e|=128;
										if(e || m) {
											if(!e) m=0;
											if(e&128) {
												ximg-=getX_Velocity()*10*step;
												yimg+=getY_Velocity()*10*step;
												if(yimg>0) yimg=0;
												if(ximg>0) ximg=0;
												if(ximg+zwidth<SCREEN_WIDTH)	ximg=SCREEN_WIDTH-zwidth;
												if(yimg+zheight<SCREEN_HEIGHT)	yimg=SCREEN_HEIGHT-zheight;
											}
											if(e&2	)
												yimg-=ustep;
											if(e&4	)
												yimg+=dstep;
											if(e&8	)
												ximg-=lstep;
											if(e&16	)
												ximg+=rstep;
											if(e&32){//zoom in
												//if(zoom-istep<1.0) istep=zoom-1.0;
												//if(zoom-istep<0.025) istep=zoom-0.025;
												if(zoom-istep<zoom_min) istep=zoom-zoom_min;
												//if(zoom-istep<=0.0) istep=0;
												zoom-=istep;
												if(zoom<=zoom_min) {
													setzoomlimits();
													istep=0;
												}
												zwidthold=zwidth;
												zheightold=zheight;
												zwidth=(int)(dheader.width/zoom);
												zheight=(int)(dheader.height/zoom);
												ximg = (ximg-SCREEN_WIDTH/2)*zwidth/zwidthold+SCREEN_WIDTH/2;
												yimg = (yimg-SCREEN_HEIGHT/2)*zheight/zheightold+SCREEN_HEIGHT/2;
											}
											if(e&64) {//zoom out
												if(zoom+ostep>zoom_max) ostep=zoom_max-zoom;
												zoom+=ostep;
												if(zoom>=zoom_max) {
													setzoomlimits();
													ostep=0;
												}
												zwidthold=zwidth;
												zheightold=zheight;
												zwidth=(int)(dheader.width/zoom);
												zheight=(int)(dheader.height/zoom);
												ximg = (ximg-SCREEN_WIDTH/2)*zwidth/zwidthold+SCREEN_WIDTH/2;
												yimg = (yimg-SCREEN_HEIGHT/2)*zheight/zheightold+SCREEN_HEIGHT/2;
												if(zwidth<SCREEN_WIDTH)
													clrBufBox(offscreen,zwidth,0,zwidthold-zwidth+1,SCREEN_HEIGHT);
												if(zheight<SCREEN_HEIGHT)
													clrBufBox(offscreen,0,zheight,SCREEN_WIDTH,zheightold-zheight+1);
											}
											if(e) m=1;
											ximg = (ximg+zwidth  < SCREEN_WIDTH ) ?
												(SCREEN_WIDTH  - zwidth ): ximg;
											yimg = (yimg+zheight < SCREEN_HEIGHT) ?
												(SCREEN_HEIGHT - zheight): yimg;
											if(yimg>0) yimg=0;//(SCREEN_HEIGHT-zheight)/2;
											if(ximg>0) ximg=0;//(SCREEN_WIDTH-zwidth)/2;
											dispBufIMG(offscreen,ximg, yimg, image, dheader.width, dheader.height, zoom);
											int trunk=0;
											if(dheader.width/zoom>SCREEN_WIDTH) trunk=1;
											if(m && dheader.height/zoom>SCREEN_HEIGHT) {
												setCurColorRGB(0,0,0);
												drwBufVert(offscreen,SCREEN_WIDTH-1,0,SCREEN_HEIGHT-1-SCROLL_WIDTH*trunk);
												drwBufVert(offscreen,SCREEN_WIDTH-SCROLL_WIDTH,0,SCREEN_HEIGHT-1-SCROLL_WIDTH*trunk);
												setCurColorRGB(0xFF,0xFF,0xFF);
												for(i=1;i<SCROLL_WIDTH-1;i++)
													drwBufVert(offscreen,SCREEN_WIDTH-i-1,0,SCREEN_HEIGHT-1-SCROLL_WIDTH*trunk);
												setCurColorRGB(0,0,0);
												drwBufHoriz(offscreen,0,SCREEN_WIDTH-SCROLL_WIDTH+1,SCREEN_WIDTH-2);
												drwBufHoriz(offscreen,SCREEN_HEIGHT-1-SCROLL_WIDTH*trunk,SCREEN_WIDTH-SCROLL_WIDTH+1,SCREEN_WIDTH-2);
												for(i=2;i<SCROLL_WIDTH-2;i++)
													drwBufVert(offscreen,SCREEN_WIDTH-i-1,2-yimg*(SCREEN_HEIGHT-4-SCROLL_WIDTH*trunk)/zheight,2-yimg*(SCREEN_HEIGHT-4-SCROLL_WIDTH*trunk)/zheight+(int)((SCREEN_HEIGHT-4-SCROLL_WIDTH*trunk)*zoom/zymax));
											}
											if(m && dheader.width/zoom>SCREEN_WIDTH) {
												setCurColorRGB(0,0,0);
												drwBufHoriz(offscreen,SCREEN_HEIGHT-1,0,SCREEN_WIDTH-1-SCROLL_WIDTH*trunk);
												drwBufHoriz(offscreen,SCREEN_HEIGHT-SCROLL_WIDTH,0,SCREEN_WIDTH-1-SCROLL_WIDTH*trunk);
												setCurColorRGB(0xFF,0xFF,0xFF);
												for(i=1;i<SCROLL_WIDTH-1;i++)
													drwBufHoriz(offscreen,SCREEN_HEIGHT-i-1,0,SCREEN_WIDTH-1-SCROLL_WIDTH*trunk);
												setCurColorRGB(0,0,0);
												drwBufVert(offscreen,0,SCREEN_HEIGHT-SCROLL_WIDTH+1,SCREEN_HEIGHT-2);
												drwBufVert(offscreen,SCREEN_WIDTH-1-SCROLL_WIDTH*trunk,SCREEN_HEIGHT-SCROLL_WIDTH+1,SCREEN_HEIGHT-2);
												for(i=2;i<SCROLL_WIDTH-2;i++)
													drwBufHoriz(offscreen,SCREEN_HEIGHT-i-1,2-ximg*(SCREEN_WIDTH-4-SCROLL_WIDTH*trunk)/zwidth,2-ximg*(SCREEN_WIDTH-4-SCROLL_WIDTH*trunk)/zwidth+(int)((SCREEN_WIDTH-4-SCROLL_WIDTH*trunk)*zoom/zxmax));
											}
											setScreen(offscreen); // offscreen buffer becomes onscreen buffer
											t=offscreen;offscreen=screen;screen=t; // switch offscreen & onscreen pointers
											if(e&64) {
												if(zwidth<SCREEN_WIDTH)
													clrBufBox(offscreen,zwidth,0,zwidthold-zwidth+1,SCREEN_HEIGHT);
												if(zheight<SCREEN_HEIGHT)
													clrBufBox(offscreen,0,zheight,SCREEN_WIDTH,zheightold-zheight+1);
											}
											e = 0;
										}
										if((isKeyPressed(KEY_NSPIRE_UP) || isKeyPressed(KEY_NSPIRE_UPRIGHT) || isKeyPressed(KEY_NSPIRE_LEFTUP) || isKeyPressed(KEY_NSPIRE_7) || isKeyPressed(KEY_NSPIRE_8) || isKeyPressed(KEY_NSPIRE_9)) && !isKeyPressed(KEY_NSPIRE_5) && !isKeyPressed(KEY_NSPIRE_CLICK) && yimg<0 )	e|=4 ;
										if((isKeyPressed(KEY_NSPIRE_DOWN) || isKeyPressed(KEY_NSPIRE_RIGHTDOWN) || isKeyPressed(KEY_NSPIRE_DOWNLEFT) || isKeyPressed(KEY_NSPIRE_1) || isKeyPressed(KEY_NSPIRE_2) || isKeyPressed(KEY_NSPIRE_3)) && !isKeyPressed(KEY_NSPIRE_5)  && !isKeyPressed(KEY_NSPIRE_CLICK) && yimg+zheight  > SCREEN_HEIGHT 	)	e|=2 ;
										if((isKeyPressed(KEY_NSPIRE_LEFT) || isKeyPressed(KEY_NSPIRE_DOWNLEFT) || isKeyPressed(KEY_NSPIRE_LEFTUP) || isKeyPressed(KEY_NSPIRE_1) || isKeyPressed(KEY_NSPIRE_4) || isKeyPressed(KEY_NSPIRE_7)) && !isKeyPressed(KEY_NSPIRE_5)  && !isKeyPressed(KEY_NSPIRE_CLICK) && ximg<0 )	e|=16;
										if((isKeyPressed(KEY_NSPIRE_RIGHT) || isKeyPressed(KEY_NSPIRE_RIGHTDOWN) || isKeyPressed(KEY_NSPIRE_UPRIGHT) || isKeyPressed(KEY_NSPIRE_3) || isKeyPressed(KEY_NSPIRE_6) || isKeyPressed(KEY_NSPIRE_9)) && !isKeyPressed(KEY_NSPIRE_5)  && !isKeyPressed(KEY_NSPIRE_CLICK) && ximg+zwidth  > SCREEN_WIDTH	)	e|=8 ;
										if(isKeyPressed(KEY_NSPIRE_MULTIPLY)/* && zoom>1.0*/)	e|=32;
										if(isKeyPressed(KEY_NSPIRE_DIVIDE)/* && zoom<zmax*/	)	e|=64;
		
										if(!(e&64) && !(e&32))
											setzoomlimits();
										if (isKeyPressed(KEY_NSPIRE_PLUS) && (contrast&0b11111111)<CONTRAST_MAX) {
											contrast=setContrast(contrast+1);
										} else if (isKeyPressed(KEY_NSPIRE_MINUS) && (contrast&0b11111111)>CONTRAST_MIN) {
											contrast=setContrast(contrast-1);
										}
										if(isKeyPressed(KEY_NSPIRE_5) || isKeyPressed(KEY_NSPIRE_CLICK)) {
											if(!(torelease&4) && (isKeyPressed(KEY_NSPIRE_2) || isKeyPressed(KEY_NSPIRE_1) || isKeyPressed(KEY_NSPIRE_3) || isKeyPressed(KEY_NSPIRE_DOWN) || isKeyPressed(KEY_NSPIRE_RIGHTDOWN) || isKeyPressed(KEY_NSPIRE_DOWNLEFT))) {
												yimg-=(int)(SCREEN_HEIGHT);
												torelease|=4;
												e|=1;
											}
											if(!(torelease&8) && (isKeyPressed(KEY_NSPIRE_8) || isKeyPressed(KEY_NSPIRE_7) || isKeyPressed(KEY_NSPIRE_9) || isKeyPressed(KEY_NSPIRE_UP) || isKeyPressed(KEY_NSPIRE_UPRIGHT) || isKeyPressed(KEY_NSPIRE_LEFTUP))) {
												yimg+=(int)(SCREEN_HEIGHT);
												torelease|=8;
												e|=1;
											}
											if(!(torelease&16) && (isKeyPressed(KEY_NSPIRE_4) || isKeyPressed(KEY_NSPIRE_7) || isKeyPressed(KEY_NSPIRE_1) || isKeyPressed(KEY_NSPIRE_LEFT) || isKeyPressed(KEY_NSPIRE_LEFTUP) || isKeyPressed(KEY_NSPIRE_DOWNLEFT))) {
												ximg+=(int)(SCREEN_WIDTH);
												torelease|=16;
												e|=1;
											}
											if(!(torelease&32) && (isKeyPressed(KEY_NSPIRE_6) || isKeyPressed(KEY_NSPIRE_3) || isKeyPressed(KEY_NSPIRE_9) || isKeyPressed(KEY_NSPIRE_RIGHT) || isKeyPressed(KEY_NSPIRE_UPRIGHT) || isKeyPressed(KEY_NSPIRE_RIGHTDOWN))) {
												ximg-=(int)(SCREEN_WIDTH);
												torelease|=32;
												e|=1;
											}
										}
										if(!(torelease&1) && isKeyPressed(KEY_NSPIRE_CTRL)) {
											if(mode&2) mode&=~2;
											else mode|=2;
											switchScrOffOn(mode&2);
											torelease|=1;
										}
										if(torelease) {	// no-repeat actions
											if(!isKeyPressed(KEY_NSPIRE_CTRL))	torelease &= ~1;
											if((!isKeyPressed(KEY_NSPIRE_5) && !isKeyPressed(KEY_NSPIRE_CLICK)) || (!isKeyPressed(KEY_NSPIRE_2) && !isKeyPressed(KEY_NSPIRE_1) && !isKeyPressed(KEY_NSPIRE_3) && !isKeyPressed(KEY_NSPIRE_DOWN)))		torelease &= ~4;
											if((!isKeyPressed(KEY_NSPIRE_5) && !isKeyPressed(KEY_NSPIRE_CLICK)) || (!isKeyPressed(KEY_NSPIRE_8) && !isKeyPressed(KEY_NSPIRE_7) && !isKeyPressed(KEY_NSPIRE_9) && !isKeyPressed(KEY_NSPIRE_UP)))		torelease &= ~8;
											if((!isKeyPressed(KEY_NSPIRE_5) && !isKeyPressed(KEY_NSPIRE_CLICK)) || (!isKeyPressed(KEY_NSPIRE_4) && !isKeyPressed(KEY_NSPIRE_1) && !isKeyPressed(KEY_NSPIRE_7) && !isKeyPressed(KEY_NSPIRE_LEFT)))		torelease &= ~16;
											if((!isKeyPressed(KEY_NSPIRE_5) && !isKeyPressed(KEY_NSPIRE_CLICK)) || (!isKeyPressed(KEY_NSPIRE_6) && !isKeyPressed(KEY_NSPIRE_3) && !isKeyPressed(KEY_NSPIRE_9) && !isKeyPressed(KEY_NSPIRE_RIGHT)))		torelease &= ~32;
										}
										if(	!isTouchpad())	wait(TOUCHPAD_DELAY);
									}
									endTP();
									if(!(mode&2))	switchScrOffOn(1);
									setContrast(ocontrast);
									contrast=ocontrast;
								}	
								else {
									displn("",0,1);
									pausecons("Error reading image data...",0, 1,1);
								}
								free(image);
								if(palette) free(palette);
							}
							else {
								displn("",0,1);
								displn("Not enough memory...",0,1);
								pausecons(NULL,0,1,1);
							}
						}
						else {
							displn("Unsupported image data...",0,1);
							dispHeader(&fheader,&dheader);
							pausecons(NULL,0,1,1);
						}	
					}
					else {
						displn("Unrecognized image data...",0,1);
						dispHeader(&fheader,&dheader);
						pausecons(NULL,0,1,1);
					}
				}
				else {
					displn("Unsupported image type...",0,1);
					dispHeader(&fheader,&dheader);
					pausecons(NULL,0,1,1);
				}
			}
			else
				pausecons("Unrecognized image type...",0, 1,1);
			fclose(img);
			if(isKeyPressed(KEY_NSPIRE_RP)) nextFile(path,path);
			else if(isKeyPressed(KEY_NSPIRE_LP)) prevFile(path,path);
			else if(selected) {
				char* p=strrchr(path,'/');
				*(p+1)=0;
				wait(ESC_DELAY);
			}
		}
	}
	stopScreen();
	free(baseoffscreen);
	return 0;
}
