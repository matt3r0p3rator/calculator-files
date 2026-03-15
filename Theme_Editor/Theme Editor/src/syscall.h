#ifndef SYSCALL_H_
# define SYSCALL_H_

static unsigned colorTable_addrs[] = {0x10855CF4, 0x10895D7C, 0x10836AD8, 0x10892E08};
#define colorTable ((int*)nl_osvalue((int*)colorTable_addrs, 4))

static unsigned colorTable_desc_addrs[] = {0x10A769F8, 0x10AB22D0, 0x10A52628, 0x10AAF388};
#define colorTable_desc ((int32_t***)nl_osvalue((int*)colorTable_desc_addrs, 4))

static unsigned MENU_CALLBACK_addrs[] = {0x107C2608, 0x10802668, 0x107A35F4, 0x107FF8FC};
#define MENU_CALLBACK ((int32_t***)nl_osvalue((int*)MENU_CALLBACK_addrs, 4))

static const int hook_addrs[] = {0x107C25DC, 0x1080263C, 0x107A35C8, 0x107FF8D0};
#define HOOK_ADDR (nl_osvalue((int*)hook_addrs, sizeof(hook_addrs)/sizeof(hook_addrs[0])))

// ROM:107FF8D0 off_107FF8D0 DCD dword_10922710
static const int hook_values[] = {0x108EA708, 0x10925658, 0x108C6338, 0x10922710};
#define HOOK_VALUE (nl_osvalue((int*)hook_values, sizeof(hook_values)/sizeof(hook_values[0])))

static unsigned get_res_string_addrs[] = {0x100E9B20, 0x100E9E10, 0x100E9634, 0x100E994C};
#define get_res_string SYSCALL_CUSTOM(get_res_string_addrs, char *, int, int)

static unsigned show_save_dialog_addrs[] = {0x10209800, 0x10209EF8, 0x10209270, 0x102099CC};
#define show_save_dialog SYSCALL_CUSTOM(show_save_dialog_addrs, int, int, char *, char *, int, int)
static int show_save_dialog_title_addrs[] = {0x102098C8, 0x10209FC0, 0x10209338, 0x10209A94};
#define show_save_dialog_title ((int *)nl_osvalue((int *)show_save_dialog_title_addrs, 4))
static int show_save_dialog_subtitle_addrs[] = {0x102098D8, 0x10209FD0, 0x10209348, 0x10209AA4};
#define show_save_dialog_subtitle ((int *)nl_osvalue((int *)show_save_dialog_subtitle_addrs, 4))
static int show_save_dialog_savebutton_addrs[] = {0x10043A24, 0x10043960, 0x10043120, 0x10043084};
#define show_save_dialog_savebutton ((int *)nl_osvalue((int *)show_save_dialog_savebutton_addrs, 4))

#endif /* !SYSCALL_H_ */