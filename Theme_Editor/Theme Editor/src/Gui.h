#ifndef GUI_H_
# define GUI_H_

# include <os.h>
# include <libndls.h>

# include "nframe.h"
# include "nGC.h"
# include "syscall.h"
# include "color.h"
# include "theme_api.h"
# include "config.h"

void theme_editor_gui(Config *config);
void hook_menu ();

#endif /* !GUI_H_ */