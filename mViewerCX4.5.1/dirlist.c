#include <os.h>
#include <dirent.h>
#include "dirlist.h"

int mystrcmp(dfileinfo* f1, dfileinfo* f2)
{	if((f1->isdir && f2->isdir) || (!(f1->isdir) && !(f2->isdir)))
		return strcmp(f1->filename,f2->filename);
	else
	{	if(f1->isdir)
			return -1;
		else
			return 1;
	}
}

void sortlist(dfileinfo** result, int n)
{	if(n<=1) return;
	dfileinfo* ref = result[0];
	dfileinfo* tmp;
	int c,i;
	int start = 0;
	int end = n-1;
	for(i=1;i<=end;i++)
	{	c=mystrcmp(ref,result[i]);
		if(c<0)
		{	tmp=result[end];
			result[end]=result[i];
			result[i]=tmp;
			end--;
			i--;
		}
		else if(c>0)
		{	tmp=result[start];
			result[start]=result[i];
			result[i]=tmp;
			start++;
		}
	}
	sortlist(result,start);
	sortlist(result+start+1,n-start-1);
}

int isdir(char* path) {
	struct stat s;
	s.st_mode=0;
	stat(path, &s);
	return s.st_mode & S_IFDIR;
}

int dirlist(char* path, dfileinfo** result) {
	DIR* dir;
	struct dirent *dent;
	if (!(dir = opendir(path)))
		return -1;
	chdir(path);
	int i = 0;
	while((dent=readdir(dir))!=NULL) {
		if(strcmp(dent->d_name,".") && strcmp(dent->d_name,"..")) {
			dfileinfo* dirname = (dfileinfo*) malloc(sizeof(dfileinfo));
			dirname->isdir=isdir(dent->d_name);
			strcpy(dirname->filename, (char*) dent->d_name);
				if(result) result[i] = dirname;
				i++;
		}
	}
	closedir(dir);
	if(result) sortlist(result,i);
	return i;
}
