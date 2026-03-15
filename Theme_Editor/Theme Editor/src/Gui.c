#include "Gui.h"

char colors[] = {0, 0, 0};
void custom_color_handler(Button button, CallbackData button_data, EventCode eventCode)
{
	if(eventCode == ENTER || eventCode == MOUSECLIC)
	{
		Object *data = (Object *)button_data;
		Button preview = data[0];
		ComboBox mode_combo = data[1];
		ComboBox theme_combo = data[2];
		SpinBox red_value = data[3];
		SpinBox green_value = data[4];
		SpinBox blue_value = data[5];

		Color c = getNthColor_mode(gui_ComboBox_getSelectedIndex(theme_combo),
								   gui_ComboBox_getSelectedIndex(mode_combo));
		colors[0] = c.r;
		colors[1] = c.g;
		colors[2] = c.b;

		String s = string_new();
		string_sprintf_utf16(s, "%\0d\0\0", c.r);
		gui_TextEntry_setText(red_value, s->str, 0);
		string_sprintf_utf16(s, "%\0d\0\0", c.g);
		gui_TextEntry_setText(green_value, s->str, 0);
		string_sprintf_utf16(s, "%\0d\0\0", c.b);
		gui_TextEntry_setText(blue_value, s->str, 0);
		string_free(s);

		if (button)
			gui_Frame_invalidate(gui_getParentFrame(button));
	}
}

int getValue(TextEntry textEntry)
{
	int value = 0;
	int rewrite = 1;
	String s = string_new();
	string_set_utf16(s, gui_TextEntry_getText(textEntry, 0));
	if (s->len > 3)
		value = 255;
	else if (s->len == 0)
		value = 0;
	else
	{
		char *s2 = s->str;
		for (; (char)*s2; s2 += 2)
			value = 10 * value + ((char)*s2) - '0';
		if (value > 255)
			value = 255;
		else
			rewrite = 0;
	}
	if (rewrite)
		string_sprintf_utf16(s, "%\0d\0\0", value);
	
	gui_TextEntry_setText(textEntry, s->str, 0);
	string_free(s);
	return value;
}

void set_color_handler(Button button, CallbackData button_data, EventCode eventCode)
{
	if(eventCode == ENTER || eventCode == MOUSECLIC
	|| eventCode == KEY || eventCode == MATHKEY
	|| eventCode == LOSTFOCUS || eventCode == FOCUS)
	{
		Object *data = (Object *)button_data;
		Button preview = data[0];
		ComboBox mode_combo = data[1];
		ComboBox theme_combo = data[2];
		SpinBox red_value = data[3];
		SpinBox green_value = data[4];
		SpinBox blue_value = data[5];

		Color c = RGB2Color(getValue(red_value),
							getValue(green_value),
							getValue(blue_value));
		setNthColor_mode(gui_ComboBox_getSelectedIndex(theme_combo),
						 _Color(c),
						 gui_ComboBox_getSelectedIndex(mode_combo));
		colors[0] = c.r;
		colors[1] = c.g;
		colors[2] = c.b;

		if (button)
			gui_Frame_invalidate(gui_getParentFrame(button));
	}
}

void randomize(Button button, CallbackData button_data, EventCode eventCode)
{
	if(eventCode == ENTER || eventCode == MOUSECLIC)
	{
		Object *data = (Object *)button_data;
		SpinBox red_value = data[3];
		SpinBox green_value = data[4];
		SpinBox blue_value = data[5];
		
		String s = string_new();
		string_sprintf_utf16(s, "%\0d\0\0", rand()%255);
		gui_TextEntry_setText(red_value, s->str, 0);
		string_sprintf_utf16(s, "%\0d\0\0", rand()%255);
		gui_TextEntry_setText(green_value, s->str, 0);
		string_sprintf_utf16(s, "%\0d\0\0", rand()%255);
		gui_TextEntry_setText(blue_value, s->str, 0);
		string_free(s);
		set_color_handler(button, button_data, eventCode);
	}
}

void invertize(Button button, CallbackData button_data, EventCode eventCode)
{
	if(eventCode == ENTER || eventCode == MOUSECLIC)
	{
		Object *data = (Object *)button_data;
		SpinBox red_value = data[3];
		SpinBox green_value = data[4];
		SpinBox blue_value = data[5];
		
		String s = string_new();
		string_sprintf_utf16(s, "%\0d\0\0", 255-getValue(red_value));
		gui_TextEntry_setText(red_value, s->str, 0);
		string_sprintf_utf16(s, "%\0d\0\0", 255-getValue(green_value));
		gui_TextEntry_setText(green_value, s->str, 0);
		string_sprintf_utf16(s, "%\0d\0\0", 255-getValue(blue_value));
		gui_TextEntry_setText(blue_value, s->str, 0);
		string_free(s);
		set_color_handler(button, button_data, eventCode);
	}
}

int set_value_handler(char *self_text, short *event)
{
	if ((char)(event[0x0]) == 0xD)
		if (event[6] >= '0' && event[6] <= '9')
			return 0;
	return 1;
}

void custom_color_painter(Button button, Gc gc, EventCodeGC eventCode)
{
	int h = 21;
	int w = 46;
	if(eventCode == GC_FOCUS)
	{
		gui_gc_setColorRGB(gc, 0, 0, 0);
		gui_gc_fillRect(gc, 0, 0, w, h);
	}
	gui_gc_setColorRGB(gc, 0, 0, 0);
	gui_gc_fillRect(gc, 1, 1, w + 500 - 2, h - 2);
	gui_gc_setColorRGB(gc, colors[0], colors[1], colors[2]);
	gui_gc_fillRect(gc, 2, 2, w - 4, h - 4);
}


void saveas_fun(Button button, CallbackData button_data, EventCode eventCode)
{
	if(eventCode == ENTER || eventCode == MOUSECLIC)
	{
		Config *conf = (Config *)button_data;
		char name[256];
		char folder[256];
		strcpy(name, "mytheme.theme");
		strcpy(folder, "/");
		if(show_save_dialog(0, folder, name, 0, 0) == 0x13EF)
		{
			if(conf->theme_path)
				free(conf->theme_path);
			conf->theme_path = calloc(1, sizeof(char) * (strlen(name) + strlen(folder) + 1 + 11 + 4));
			strcpy(conf->theme_path, "/documents/");
			strcat(conf->theme_path, folder);
			strcat(conf->theme_path, name);
			strcat(conf->theme_path, ".tns");
			puts(conf->theme_path);
			writeTheme(conf->theme_path);
		}
	}
}

int show_open_dialog(char * folder, char * name) {
	int saved_title = *show_save_dialog_title;
	int saved_subtitle = *show_save_dialog_subtitle;
	int saved_savebutton = *show_save_dialog_savebutton;
	*show_save_dialog_title = 0xE3A01036; /* MOV R1, 0x36 */
	*show_save_dialog_subtitle = 0xE3A010CA; /* MOV R1, 0xCA */
	*show_save_dialog_savebutton = 0xE3A0106C; /* MOV R1, 0x6C */

	int ret = show_save_dialog(0, folder, name, 0, 0);

	*show_save_dialog_title = saved_title;
	*show_save_dialog_subtitle = saved_subtitle;
	*show_save_dialog_savebutton = saved_savebutton;
	return ret;
}

void open_fun(Button button, CallbackData button_data, EventCode eventCode)
{
	if(eventCode == ENTER || eventCode == MOUSECLIC)
	{
		Frame frame = gui_getParentFrame(button);
		Config *conf = (Config *)(((unsigned **)button_data)[0]);
		Object *data = (Object *)(((unsigned **)button_data)[1]);
		char name[256];
		char folder[256];
		strcpy(name, "mytheme.theme");
		strcpy(folder, "/");
		gui_Frame_setVisible(frame, 0);
		do
		{
			if(show_open_dialog(folder, name) == 0x13EF)
			{
				char *fname = calloc(1, sizeof(char) * (strlen(name) + strlen(folder) + 1 + 11 + 4));
				strcpy(fname, "/documents/");
				strcat(fname, folder);
				strcat(fname, name);
				strcat(fname, ".tns");
				puts(fname);
				if (readTheme(fname))
				{
					char undef_buf[8];
					*(char**)undef_buf = "DLG";
					show_dialog_box2_(0, get_res_string(SYST, 0x8E), get_res_string(SYST, 0xC1), undef_buf);
				}
				else
				{
					if(conf->theme_path)
						free(conf->theme_path);
					conf->theme_path = calloc(1, sizeof(char) * (strlen(fname) + 1));
					strcpy(conf->theme_path, fname);
					break;
				}
			}
			else
				break;
		} while (1);
		custom_color_handler(button, (CallbackData)data, ENTER);
		gui_Frame_setVisible(frame, 1);
	}
}

void opt_fun(Button button, CallbackData button_data, EventCode eventCode)
{
	if(eventCode == ENTER || eventCode == MOUSECLIC)
	{
		Frame frame = gui_getParentFrame(button);
		Config *conf = (Config *)button_data;

		char undef_buf[8];
		*(char**)undef_buf = "DLG";
		Frame opt_frame = gui_new_Frame_0(0, get_res_string(DCOL, 0x73), 0x11941, 0, 0, undef_buf);

		Container container = gui_getContentPane(opt_frame);

		Panel panel1 = gui_addNew_Panel(container, 0);
		gui_Panel_setLayout(panel1, 0x128E3);
		CheckButton patch = gui_addNew_CheckButton(panel1, SMALLEST, "P\0a\0t\0c\0h\0 \0O\0S\0 \0m\0e\0n\0u\0\0");
		if(!conf->no_patch_os)
			gui_ToggleButton_setSelected(patch, CHECKED);

		Panel panel2 = gui_addNew_Panel(container, 0);
		gui_Panel_setLayout(panel2, 0x128E3);
		ButtonGroup group = gui_addNew_ButtonGroup(panel2, 3);
		RadioButton mode_Both = gui_addNew_ToggleButton(panel2, RADIOBUTTON, SMALLEST, "B\0o\0t\0h\0\0", NOT_CHECKED, HLEFT, VCENTER, 0, 0);
		gui_ButtonGroup_add(group, mode_Both);
		RadioButton mode_Gui = gui_addNew_ToggleButton(panel2, RADIOBUTTON, SMALLEST, "G\0U\0I\0 \0O\0n\0l\0y\0\0", NOT_CHECKED, HLEFT, VCENTER, 0, 0);
		gui_ButtonGroup_add(group, mode_Gui);
		RadioButton mode_noGui = gui_addNew_ToggleButton(panel2, RADIOBUTTON, SMALLEST, "N\0o\0 \0G\0U\0I\0\0", NOT_CHECKED, HLEFT, VCENTER, 0, 0);
		gui_ButtonGroup_add(group, mode_noGui);

		switch (conf->mode)
		{
			case 2:
				gui_ButtonGroup_setSelected(group, mode_noGui, 1);
				break;
			case 1:
				gui_ButtonGroup_setSelected(group, mode_Gui, 1);
				break;
			case 0:
			default:
				gui_ButtonGroup_setSelected(group, mode_Both, 1);
				break;
		}

		gui_Frame_setVisible(opt_frame, 1);
		if(gui_ToggleButton_isSelected(mode_noGui) == CHECKED)
			conf->mode = 2;
		else if(gui_ToggleButton_isSelected(mode_Gui) == CHECKED)
			conf->mode = 1;
		else
			conf->mode = 0;
		conf->no_patch_os = gui_ToggleButton_isSelected(patch) == NOT_CHECKED;
		gui_Frame_free(opt_frame);
	}
}

void theme_editor_gui(Config *conf)
{
	char undef_buf[8];
	*(char**)undef_buf = "DLG";
	Frame frame = gui_new_Frame_0(0, get_res_string(SYST, 0x34), 0x11941, 0, 0, undef_buf);

	Container container = gui_getContentPane(frame);

	Panel panel1 = gui_addNew_Panel(container, 0);
	gui_Panel_setLayout(panel1, 0x128E1);

	Object objs[6];

	/* ComboBox - B&W or Color */
	char *bw_title = "B\0&\0W\0 \0T\0h\0e\0m\0e\0\0";
	char *color_title = "C\0o\0l\0o\0r\0 \0T\0h\0e\0m\0e\0\0";
	Object mode_title[] = {bw_title, color_title, 0};
	ComboBox mode_combo = gui_addNew_ComboBox(panel1, DISABLED, mode_title, HLEFT, VTOP, (CallbackData)objs, custom_color_handler);
	gui_ComboBox_setSelectedIndex(mode_combo, is_cx ? 1 : 0);
	objs[1] = mode_combo;

	/* CustomButton - Preview */
	Button preview = gui_addNew_CustomButton(panel1, " \0 \0 \0 \0 \0 \0 \0 \0 \0 \0 \0\0", 0, 0, 0, HLEFT, VCENTER, (CallbackData)colors, 0, custom_color_painter, 1, -1);
	gui_Button_setTabable(preview, UNTABABLE);
	objs[0] = preview;

	/* Button - Randomize */
	Button random = gui_addNew_Button_0(panel1, 0, 0x24, NTPD, get_res_string(NTPD, 0x80), HLEFT, VCENTER, (CallbackData)objs, randomize);
	
	/* Button - Invert */
	Button invert = gui_addNew_Button_0(panel1, 0, 0x18, NTPD, get_res_string(DCOL, 0x80), HLEFT, VCENTER, (CallbackData)objs, invertize);
	
	/* ComboBox - Theme item */
	Panel panel2 = gui_addNew_Panel(container, 0);
	gui_Panel_setLayout(panel2, 0x128E1);
	ComboBox theme_combo = gui_addNew_ComboBox(panel2, DISABLED, (Object*)colorTable_desc, HLEFT, VTOP, (CallbackData)objs, custom_color_handler);
	gui_ComboBox_setSelectedIndex(theme_combo, conf->current_index);
	objs[2] = theme_combo;

	/* Line with R G B selectors */
	Panel panel3 = gui_addNew_Panel(container, 0);
	gui_Panel_setLayout(panel3, 0x128E1);
	gui_Panel_setMarginV(panel3, 1, 1);

	Panel panel_red = gui_addNew_Panel(panel3, 0);
	gui_Panel_setLayout(panel_red, 0x128E7);
	gui_Panel_setMarginV(panel_red, 1, 1);

	Label red_label = gui_addNew_Label(panel_red, get_res_string(SYST, 0x2E7), 0, 0, 0, HLEFT, VCENTER);
	TextEntry red_value = gui_addNew_TextEntry(panel_red, ENABLED, HLEFT, VCENTER, (CallbackData)objs, set_color_handler);
	gui_TextEntry_setCharIn(red_value, (Callback)set_value_handler);
	objs[3] = red_value;
	
	Panel panel_green = gui_addNew_Panel(panel3, 0);
	gui_Panel_setLayout(panel_green, 0x128E7);
	gui_Panel_setMarginV(panel_green, 1, 1);

	Label green_label = gui_addNew_Label(panel_green, get_res_string(SYST, 0x2E9), 0, 0, 0, HLEFT, VCENTER);
	TextEntry green_value = gui_addNew_TextEntry(panel_green, ENABLED, HLEFT, VCENTER, (CallbackData)objs, set_color_handler);
	gui_TextEntry_setCharIn(green_value, (Callback)set_value_handler);
	objs[4] = green_value;

	Panel panel_blue = gui_addNew_Panel(panel3, 0);
	gui_Panel_setLayout(panel_blue, 0x128E7);
	gui_Panel_setMarginV(panel_blue, 1, 1);

	Label blue_label = gui_addNew_Label(panel_blue, get_res_string(SYST, 0x2E5), 0, 0, 0, HLEFT, VCENTER);
	TextEntry blue_value = gui_addNew_TextEntry(panel_blue, ENABLED, HLEFT, VCENTER, (CallbackData)objs, set_color_handler);
	gui_TextEntry_setCharIn(blue_value, (Callback)set_value_handler);
	objs[5] = blue_value;

	/* Button - Invisible EnterListner */
	Panel panel_invisible = gui_addNew_Panel(panel3, 0);
	gui_Panel_setLayout(panel_invisible, 0x128E1);
	Button enterListener = gui_addNew_Button(panel_invisible, 0, (CallbackData)objs, set_color_handler);
	gui_Button_setTabable(enterListener, UNTABABLE);

	gui_addNew_Separator(container);

	/* Button - Save as */
	Panel panel4 = gui_addNew_Panel(container, 0);
	gui_Panel_setLayout(panel4, 0x128E3);
	Button saveas = gui_addNew_Button_0(panel4, 0, 0x4C, SYST, get_res_string(SYST, 0x15), HRIGHT, VCENTER, (CallbackData)conf, saveas_fun);

	/* Button - Open */
	void *open_data[] = {conf, objs};
	Button open = gui_addNew_Button_0(panel4, 0, 0x114, SYST, get_res_string(SYST, 0x36), HRIGHT, VCENTER, (CallbackData)open_data, open_fun);

	/* Button - Options */
	Button opts = gui_addNew_Button_0(panel4, 0, 0xFE, SYST, get_res_string(DCOL, 0x73), HRIGHT, VCENTER, (CallbackData)conf, opt_fun);

	custom_color_handler(0, (CallbackData)objs, ENTER);
	gui_Frame_setSelectedObject(frame, theme_combo);
//	gui_Frame_setEscListener(frame, cancel);
	gui_Frame_setEnterListener(frame, enterListener);

	gui_Frame_setVisible(frame, 1);
	conf->current_index = gui_ComboBox_getSelectedIndex(theme_combo);
	gui_Frame_free(frame);
}

void hook_menu_handler(Button button, CallbackData button_data, EventCode eventCode)
{
	if(eventCode == ENTER || eventCode == MOUSECLIC)
	{
		Config *conf = new_config();
		char *filename = "/ThemeEditor.tns";
		char *argv[] = {filename};
		if (read_config(conf, 1, argv))
		{
			free_config(conf);
			return;
		}
		
		if (conf->mode == 2)
			conf->current_index = themeEditor(conf->current_index);
		else
			theme_editor_gui(conf);
		
		write_config(conf);
		free_config(conf);
	}
}

static int32_t* new_menu = NULL;

void hook_menu ()
{
	if(*((int*)HOOK_ADDR) == HOOK_VALUE)
	{
		int32_t new_menu_[] = {
			1, 0xE5,  0xFFFFFFFF, 1, SYST,
			2, 0xE7,  0,          0, SYST,
			3, 0xEB,  0xFFFFFFFF, 2, SYST,
			3, 0x16D, 0xFFFFFFFF, 3, SYST,
			1, 0x176, 0xFFFFFFFF, 4, SYST,
			1, 0x192, 0xFFFFFFFF, 5, SYST,
			1, 0x5E,  0xFFFFFFFF, 6, SYST,
			// sentinel 0x42133769 because we can't get absolute addressing in the declaration of a table
			1, 0x34,  0x42133769, 0, SYST,
			0, 0,     0,          0, 0
		};

		new_menu = malloc (sizeof (new_menu_));
		unsigned i = 0;
		unsigned n = sizeof (new_menu_) / sizeof (new_menu_[0]);
		for (; i < n; ++i)
			if (new_menu_[i] == (int32_t)0xFFFFFFFF)
				new_menu[i] = (int32_t)MENU_CALLBACK;
			else if (new_menu_[i] == 0x42133769)
				new_menu[i] = (int32_t)hook_menu_handler;
			else
				new_menu[i] = (int32_t)new_menu_[i];

		*((int32_t**)HOOK_ADDR) = new_menu;
		puts("patched");
		nl_set_resident();
	}
}
