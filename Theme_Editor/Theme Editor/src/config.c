#include "config.h"

static char *config_path = "/documents/config.theme";
static char *extension = "theme";

static char *title = "Theme Editor says";
static char *errorUninst = "Theme Editor is not installed yet !";
static char *successUninst = "Theme Editor successfuly deleted the configfile.";
static char *confirmUninst = "Do you really want to uninstall Theme Editor ?";
static char *nameYourTheme = "Name your theme";
static char *setupCanceled = "Setup Canceled";
static char *warningNdlessCfg = "ndless.cfg was not found.\nDo you want Theme Editor to create it ?";
static char *welcomeMsg = "Hi !\nThis may be your first use of \n\\1keyword Theme \\1keyword Editor, so, welcome !\nTheme Editor will create an hidden configfile. You can remove it by renaming ThemeEditor.tns to uninstall.tns and launch it. You can also open \\1keyword .theme.tns files in order to install a theme. Any modification you do will be automatically written to the last openned theme file. You can abort any modification you make to the system by \nusing the reset button at the \nback of your calculator. You can also put ThemeEditor.tns in the startup folder of ndless.\nMay this program be usefull !\nLevak (levak92@gmail.com)";
static char *yes = "Yes";
static char *no = "No";

Config *new_config()
{
	return calloc(1, sizeof(Config));
}

int read_config(Config *conf, int argc, char *argv[])
{
	int len = strlen(argv[0]);
	char path[len];
	strcpy(path, argv[0]);
	char * p = strrchr(path, '/');
	char self_name[len - ((p) ? p - path + 1: 0) + 1];
	char * p2 = (p) ? p + 1 : path;
	int len2 = strlen(p2) - 4;
	strncpy(self_name, p2, len2);
	self_name[len2] = 0;

	if(strcmp(self_name, "uninstall") == 0) {
		if (show_msgbox_2b(title, confirmUninst, yes, no) == 2)
			return -1;
		if(remove(config_path) == 0)
			show_msgbox(title, successUninst);
		else
			show_msgbox(title, errorUninst);
		return -1;
	}

	FILE *config = fopen(config_path, "r");
	if(config != NULL) { /* If there is a config file containing the theme path */
		fseek(config, 0, SEEK_END);
		char curr_i[7] = {0};
		char mode[2] = {0};
		char no_patch[2] = {0};
		unsigned length = ftell(config) - sizeof(curr_i) - sizeof(mode) - sizeof(no_patch) - 5;
		rewind(config);
		conf->theme_path = (char *) malloc(sizeof(char) * (length + 1));
		fread(conf->theme_path, sizeof(char), length + 1, config);
		conf->theme_path[length] = 0;
		//printf("path: %s\n", conf->theme_path);

		fread(curr_i, sizeof(char), sizeof(curr_i), config);
		curr_i[6] = 0;
		conf->current_index = hex2int(curr_i);
		//printf("curr_i: %s %d\n", curr_i, conf->current_index);

		fread(no_patch, sizeof(char), sizeof(no_patch), config);
		no_patch[1] = 0;
		conf->no_patch_os = no_patch[0] - '0';
		//printf("no_patch_os: %s %d\n", no_patch, conf->no_patch_os);

		fread(mode, sizeof(char), sizeof(mode), config);
		mode[1] = 0;
		conf->mode = mode[0] - '0';
		//printf("mode: %s %d\n", mode, conf->mode);
		fclose(config);

		if(argc >= 2) { /* We have a input theme file */
			free(conf->theme_path);
			conf->theme_path = calloc(1, sizeof(char) * (strlen(argv[1]) + 1));
			strcpy(conf->theme_path, argv[1]);
		}
	}
	else { /* First use */
		show_msgbox(title, welcomeMsg);
		/* patch /documents/ndless/ndless.cfg if necessary : add extension */
		char * ndlesscfg_path = "/documents/ndless/ndless.cfg.tns";
		FILE * ndlesscfg = fopen(ndlesscfg_path, "r");
		if(ndlesscfg != NULL) {
			/* Search for extension in ndless.cfg */
			fseek(ndlesscfg, 0, SEEK_END);
			unsigned length = ftell(ndlesscfg);
			rewind(ndlesscfg);
			char * whole_file = (char *) malloc(sizeof(char) * (length + 1));
			memset(whole_file, 0, length);
			fread(whole_file, sizeof(char), length, ndlesscfg);
			whole_file[length] = 0;
			char * line = whole_file;
			char * eol = whole_file;
			int found = 0;
			while((unsigned)(eol - whole_file) < length) {
				eol = strchr(line, '\n');
				*eol = 0;
				char * point = strchr(line, '.');
				if(point) {
					*point = 0;
					char * ext = point + 1;
					char * equal = strchr(ext, '=');
					if(equal) {
						*equal = 0;
						if(strcmp(ext, extension) == 0) {
							found = 1;							
							break;
						}
					}
				}
				line = eol + 1;
			}
			/* If not found, add the extension */
			if(!found) {
				fclose(ndlesscfg);
				ndlesscfg = fopen(ndlesscfg_path, "a");
				fprintf(ndlesscfg, "\next.%s=%s\n", extension, self_name);
			}
			fclose(ndlesscfg);
			free(whole_file);
		}
		else {
			int option = show_msgbox_2b(title, warningNdlessCfg, yes, no);
			if(option == 1) {
				ndlesscfg = fopen(ndlesscfg_path, "w");
				fprintf(ndlesscfg, "ext.%s=%s\n", extension, self_name);
				fclose(ndlesscfg);
			}
		}

		/* create config file with an inputbox */
		char *name = NULL;//[] = "mytheme";
		//int length = sizeof(name);
				int length = show_msg_user_input(nameYourTheme, "", "mytheme", &name);
		if(length < 0) {
			show_msgbox(title, setupCanceled);
			return -1;
		}
		conf->theme_path = (char *) malloc(sizeof(char) * (length + 1 + sizeof(extension) + 1));
		sprintf(conf->theme_path, "/documents/%s.%s.tns", name, extension);
		free(name);
	}
	return 0;
}

void write_config(Config *conf)
{
	if (conf->theme_path)
		writeTheme(conf->theme_path);

	FILE *config = fopen(config_path, "w");
	if(config != NULL) {
		//printf("%s\n0x%04X\n%d\n%d\n", conf->theme_path, conf->current_index, conf->no_patch_os, conf->mode);
		fprintf(config, "%s\n0x%04X\n%d\n%d\n", conf->theme_path, conf->current_index, conf->no_patch_os, conf->mode);
		fclose(config);
	}
}

void free_config(Config *conf)
{
	if(conf->theme_path)
		free(conf->theme_path);
	free(conf);
}