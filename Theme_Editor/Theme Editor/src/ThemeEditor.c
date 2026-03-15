#include <os.h>
#include <libndls.h>

#include "charmap_8x12.h"
#include "syscall.h"
#include "color.h"
#include "draw.h"
#include "theme_api.h"
#include "config.h"

#ifdef GUI
# include "Gui.h"
#endif /* GUI */

int getEvt() {
	/* ON      R    I  Left Right       Down  Up Enter Esc
	   x       x    x    x    x          x    x    x    x
	 100      80   40   20   10          8    4    2    1*/
	int evt = 0;
	if(isKeyPressed(KEY_NSPIRE_ESC))	 evt |= 0x1;
	if(isKeyPressed(KEY_NSPIRE_ENTER)) evt |= 0x2;
	if(isKeyPressed(KEY_NSPIRE_CLICK)) evt |= 0x2;
	if(isKeyPressed(KEY_NSPIRE_UP))		evt |= 0x4;
	if(isKeyPressed(KEY_NSPIRE_LEFTUP))		evt |= 0x4;
	if(isKeyPressed(KEY_NSPIRE_UPRIGHT))		evt |= 0x4;
	if(isKeyPressed(KEY_NSPIRE_DOWN))	evt |= 0x8;
	if(isKeyPressed(KEY_NSPIRE_DOWNLEFT))	evt |= 0x8;
	if(isKeyPressed(KEY_NSPIRE_RIGHTDOWN))	evt |= 0x8;
	if(isKeyPressed(KEY_NSPIRE_RIGHT)) evt |= 0x10;
	if(isKeyPressed(KEY_NSPIRE_LEFT))	evt |= 0x20;
	if(isKeyPressed(KEY_NSPIRE_I))	evt |= 0x40;
	if(isKeyPressed(KEY_NSPIRE_R))	evt |= 0x80;
	if(on_key_pressed())	evt |= 0x100;
	//if(evt) printf("%x\n", evt);
	return evt;
}

int getHex() {
	int h = -1;
	if(isKeyPressed(KEY_NSPIRE_0))	 h = 0;
	if(isKeyPressed(KEY_NSPIRE_1))	 h = 1;
	if(isKeyPressed(KEY_NSPIRE_2))	 h = 2;
	if(isKeyPressed(KEY_NSPIRE_3))	 h = 3;
	if(isKeyPressed(KEY_NSPIRE_4))	 h = 4;
	if(isKeyPressed(KEY_NSPIRE_5))	 h = 5;
	if(isKeyPressed(KEY_NSPIRE_6))	 h = 6;
	if(isKeyPressed(KEY_NSPIRE_7))	 h = 7;
	if(isKeyPressed(KEY_NSPIRE_8))	 h = 8;
	if(isKeyPressed(KEY_NSPIRE_9))	 h = 9;
	if(isKeyPressed(KEY_NSPIRE_A))	 h = 10;
	if(isKeyPressed(KEY_NSPIRE_B))	 h = 11;
	if(isKeyPressed(KEY_NSPIRE_C))	 h = 12;
	if(isKeyPressed(KEY_NSPIRE_D))	 h = 13;
	if(isKeyPressed(KEY_NSPIRE_E))	 h = 14;
	if(isKeyPressed(KEY_NSPIRE_F))	 h = 15;
	return h;
}

void wait_no_key_pressed_repeat(int * repeatKey) {
	unsigned counter =	0x5;//0xFFF;
	unsigned counter2 = (*repeatKey) ? 0x5 : 0xFF; //0x7FF 0x1FFFF
	while(any_key_pressed() && (--counter > 0 || (counter = 0xFFF && --counter2 > 0)));
	*repeatKey = counter2 == 0;
}

Color colorPicker(int current_index) {
	int evt = 0;
	int repeatKey = 0;
	int invalidate = 1;
	int idle_mode = 0;
	Color color = getNthColor(current_index);
	int current_channel = 0;
	int num_channels = (is_cx) ? 3 : 1;
	char channels[3] = {color.r, color.g, color.b};

	Color black = new_Color_RGB(0, 0, 0);
	Color white = new_Color_RGB(0xFF, 0xFF, 0xFF);
	Color grey = new_Color_RGB(0x80, 0x80, 0x80);;
	Color blue = new_Color_RGB(0, 0, 0xFF);
	int width = (is_cx) ? 200 : 120;
	int height = 180;
	int pos_x = (320-width)/2;
	int pos_y = (240-height)/2;
	int thumb_x = pos_x + width - 72;
	drawRect(pos_x-1, pos_y-1, width+1, height+1, 1, white);
	fillRect(pos_x, pos_y, width, height, black);
	fillRect(thumb_x-3, pos_y, 1, height, white);
	drawString("Old color", thumb_x, pos_y+5, white, 1);
	fillRect(thumb_x, pos_y+20, 70, 70, color);
	drawString("New color", thumb_x, pos_y+95, white, 1);
	if(is_cx) {
		drawString("R", pos_x+20, pos_y+5, white, 1);
		drawString("G", pos_x+60, pos_y+5, white, 1);
		drawString("B", pos_x+100, pos_y+5, white, 1);
	}
	else
		drawString("Grey", pos_x+10, pos_y+5, white, 1);
	wait_no_key_pressed();
	while(!(evt&1)) {
		if(invalidate) {
			// Draw
			Color color;
			if(is_cx)
				color = RGB2Color(channels[0], channels[1], channels[2]);
			else
				color = RGB2Color(channels[0], channels[0], channels[0]);
			fillRect(thumb_x, pos_y+110, 70, 70, color);
			int i;
			for(i = 0; i < num_channels; ++i) {
				int p = channels[i]>>1;
				fillRect(pos_x+10+i*40, pos_y+28, 30, height-53-p, white);
				fillRect(pos_x+10+i*40, pos_y+25+height-50-p, 30, p, grey);
				char str[3];
				sprintf(str, "%02X", channels[i]);
				fillRect(pos_x+15+i*40, pos_y+height-20, 2*CHAR_WIDTH, CHAR_HEIGHT, black);
				drawString(str, pos_x+15+i*40, pos_y+height-20, white, 1);
				drawRect(pos_x+8+i*40, pos_y+26, 33, height-30, 1, (i == current_channel) ? blue : black);				
			}
			invalidate = 0;
		}

		int prev_evt = evt;
		evt = getEvt();
		if(prev_evt != evt) repeatKey = 0;
		if(evt) {
			if(idle_mode) {
				idle_mode = 0;
				wait_no_key_pressed();
				evt = 0; /* Cancel the event */
			}
		}
		else
			idle();

		if(idle_mode) {
			sleep(500);
		}

		if(evt & 0x2) {
			wait_no_key_pressed();
			// Enter
			if(is_cx)
				color = RGB2Color(channels[0], channels[1], channels[2]);
			else
				color = RGB2Color(channels[0], channels[0], channels[0]);
			evt = 1;
		}

		int key = getHex();
		if(key >= 0) {
			channels[current_channel] = ((channels[current_channel] << 4) & 0xFF) + key;
			wait_no_key_pressed();
			invalidate = 1;
		}

		if(evt & 0x40) {
			int i;
			for(i = 0; i < num_channels; ++i)
				channels[i] = 255 - channels[i];
			invalidate = 1;
			wait_no_key_pressed();
		}		

		if(evt & 0x80) {
			int i;
			for(i = 0; i < num_channels; ++i)
				channels[i] = rand()%255;
			invalidate = 1;
			wait_no_key_pressed();
		}

		if(evt & 0x8) {
			if(channels[current_channel] > 0)
				--channels[current_channel];
			invalidate = 1;
			wait_no_key_pressed_repeat(&repeatKey);
		}

		if(evt & 0x4) {
			if(channels[current_channel] < 255)
				++channels[current_channel];
			invalidate = 1;
			wait_no_key_pressed_repeat(&repeatKey);
		}

		if(evt & 0x20) {
			if(current_channel > 0)
				--current_channel;
			else
				current_channel = num_channels - 1;
			invalidate = 1;
			wait_no_key_pressed();
		}

		if(evt & 0x10) {
			if(current_channel < num_channels - 1)
				++current_channel;
			else
				current_channel = 0;
			invalidate = 1;
			wait_no_key_pressed();
		}
	}
	fillRect(pos_x-1, pos_y-1, width+2, height+2, black);
	return color;
}


int themeEditor(int current_index) {
	int evt = 0;
	int repeatKey = 0;
	int invalidate = 1;
	int big_invalidate = 1;
	int idle_mode = 0;
	int tableLength = num_symbols;
	if(current_index < 0 || current_index >= tableLength) current_index = 0;
	int elementsOnScreen = 16;
	int index_start = (current_index > elementsOnScreen) ? current_index - elementsOnScreen + 1: 0;
	int old_index = -1;
	int width_offset = 0;
	int width_maxoffset = 16;
	int fact = 22000/tableLength;
	clrscr();
	wait_no_key_pressed();	
	while(!(evt&1)) {
		if(invalidate) {
			// Draw
			int i = 0;
			Color white = new_Color_RGB(0xFF, 0xFF, 0xFF);
			Color black = new_Color_RGB(0, 0, 0);
			Color blue = new_Color_RGB(0, 0, 0xFF);
			Color color = new_Color();
 
			if(big_invalidate) {
				for(i = 0; i < elementsOnScreen; ++i) {
					int pos_y = i*14;
					color = getNthColor(i+index_start);
					fillRect(CHAR_WIDTH, pos_y, 37*CHAR_WIDTH, CHAR_HEIGHT, black);
					if(i+index_start != current_index)
						drawString(getNthSymbol(i+index_start)+width_offset, CHAR_WIDTH, i*14, white, 1);
					if(1) {
						fillRect(0, pos_y, CHAR_WIDTH, CHAR_HEIGHT, black);
						drawString("*", 0, pos_y, white, 1);
					}
					drawRect(300, pos_y, 10, 10, 1, white);
					fillRect(301, pos_y+1, 10-1, 10-1, color);
				}
			}
			if(old_index >= index_start && old_index < index_start + elementsOnScreen) {
				drawString(getNthSymbol(old_index)+width_offset, CHAR_WIDTH, (old_index-index_start)*14, white, 1);
			}			
			if(current_index >= index_start && current_index < index_start + elementsOnScreen) {
				drawString(getNthSymbol(current_index)+width_offset, CHAR_WIDTH, (current_index-index_start)*14, blue, 1);
				color = getNthColor(current_index);
				char str[15];
				sprintf(str, "R:%02X G:%02X B:%02X", color.r, color.g, color.b);
				fillRect(10, 228, 15*CHAR_WIDTH, CHAR_HEIGHT, black);
				drawString(str, 10, 228, white, 1);
			}
			fillRect(313, 0, 6, 220, white);
			fillRect(314, (index_start*fact)/100, 4, (elementsOnScreen*fact)/100, black);

			if(!big_invalidate)
				sleep(10);

			invalidate = 0;
			big_invalidate = 0;
		}
		old_index = current_index;
		int prev_evt = evt;
		evt = getEvt();
		if(prev_evt != evt) repeatKey = 0;
		if(evt) {
			if(idle_mode) {
				idle_mode = 0;
				wait_no_key_pressed();
				evt = 0; /* Cancel the event */
			}
		}
		else
			idle();

		if(idle_mode) {
			sleep(500);
		}

		if(evt & 0x40) {
			int i;
			for(i = 0; i < tableLength; ++i) {
				Color c = getNthColor(i);
				setNthColor(i, _Color(new_Color_RGB(255-c.r, 255-c.g, 255-c.b)));
			}
			invalidate = 1;
			big_invalidate = 1;
			wait_no_key_pressed();
		}

		if(evt & 0x80) {
			int i;
			for(i = 0; i < tableLength; ++i) {
				int a, b, c;
				a = rand()%255;
				b = rand()%255;
				c = rand()%255;
				setNthColor(i, _Color(new_Color_RGB(a, b, c)));
			}
			invalidate = 1;
			big_invalidate = 1;
			wait_no_key_pressed();
		}
		
		if(evt & 0x2) {
			wait_no_key_pressed();			
			// Enter
			setNthColor(current_index, _Color(colorPicker(current_index)));
			invalidate = 1;
			big_invalidate = 1;
			wait_no_key_pressed();
		}
		
		if(evt & 0x20) {
			if(width_offset > 0)
				--width_offset;
			invalidate = 1;
			big_invalidate = 1;
			wait_no_key_pressed_repeat(&repeatKey);
		}

		if(evt & 0x10) {
			if(width_offset < width_maxoffset - 1)
				++width_offset;
			invalidate = 1;
			big_invalidate = 1;
			wait_no_key_pressed_repeat(&repeatKey);
		}

		if(evt & 0x4) {
			if(current_index > 0) {
				--current_index;
				if(current_index < index_start) {
					index_start = current_index;
					big_invalidate = 1;
				}
			}
			else {
				current_index = tableLength - 1;
				index_start = current_index - elementsOnScreen + 1;
				big_invalidate = 1;
			}
			invalidate = 1;
			wait_no_key_pressed_repeat(&repeatKey);
		}

		if(evt & 0x8) {
			if(current_index < tableLength - 1) {
				++current_index;
				if(current_index >= index_start + elementsOnScreen) {
					index_start = current_index - elementsOnScreen + 1;
					big_invalidate = 1;
				}
			}
			else {
				current_index = 0;
				index_start = 0;
				big_invalidate = 1;
			}
			invalidate = 1;
			wait_no_key_pressed_repeat(&repeatKey);
		}
	}
	return current_index;
}

int main(int argc, char* argv[]) {
	assert_ndless_rev(680);
	if(argc < 1) return 0;

	Config *conf = new_config();
	if (read_config(conf, argc, argv) == 0)
	{
		readTheme(conf->theme_path);
# ifdef GUI
		if (!conf->no_patch_os)
			hook_menu();
# endif /* GUI */
		if(!nl_isstartup())
		{
			if(conf->mode == 0 || conf->mode == 2)
				conf->current_index = themeEditor(conf->current_index);
			# ifdef GUI
			else
				theme_editor_gui(conf);
			#endif /* GUI */
			write_config(conf);
//			refresh_osscr(); /* Produces too much lags */
		}
	}
	else
	{
# ifdef GUI
		hook_menu();
# endif /* GUI */
	}

	free_config(conf);
	return 0;
}
