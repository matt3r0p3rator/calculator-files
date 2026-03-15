typedef struct fileinfo
{	char filename[128];
	int isdir;
} dfileinfo;

extern int dirlist(char* path, dfileinfo** result);
