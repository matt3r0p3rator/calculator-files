#ifndef CONFIG_H_
# define CONFIG_H_

# include <os.h>
# include <libndls.h>

typedef struct
{
	int current_index;
	int mode; /* 0 = Both, 1 = Gui only, 2 = No Gui */
	int no_patch_os;
	char *theme_path;
} Config;

Config *new_config();
int read_config(Config *conf, int argc, char *argv[]);
void write_config(Config *conf);
void free_config(Config *conf);

#endif /* !CONFIG_H_ */