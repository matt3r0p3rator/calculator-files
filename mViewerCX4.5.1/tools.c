#include "os.h"
#include "config.h"
#include "tools.h"

void addshellext(char* extension, char* name)
{	FILE * ndlesscfg = fopen(NDLESS_CONFIG, "r");
	if(ndlesscfg != NULL)
	{	/* Search for extension in ndless.cfg */
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
		while((unsigned)(eol - whole_file) < length)
		{	eol = strchr(line, '\n');
			*eol = 0;
			char * point = strchr(line, '.');
			if(point)
			{	*point = 0;
				char * ext = point + 1;
				char * equal = strchr(ext, '=');
				if(equal)
				{	*equal = 0;
					if(strcmp(ext, extension) == 0)
					{	found = 1;	      
						break;
					}
				}
			}
			line = eol + 1;
		}
		/* If not found, add the extension */
		if(!found)
		{	fclose(ndlesscfg);
			ndlesscfg = fopen(NDLESS_CONFIG, "a");
			fprintf(ndlesscfg, "\next.%s=%s\n", extension, name);
		}
		fclose(ndlesscfg);
		free(whole_file);
	}
	else
	{	ndlesscfg = fopen(NDLESS_CONFIG, "w");
		fprintf(ndlesscfg, "ext.%s=%s\n", extension, name);
		fclose(ndlesscfg);
	}
}

void wait(int timesec)
{
	msleep(timesec * 1000);
}

/*void wait(int timesec)
{	volatile i;
	for(i=0;i<timesec*10000;i++) {}
}*/

/*
char* itoa (char *buf, unsigned int n, int base)
{
    unsigned int tmp;
    int i = 0, j;

    do
    {
        tmp = n % base;
        buf[i++] = (tmp < 10) ? (tmp + '0') : (tmp + 'A' - 10);
    } while (n /= base);
    buf[i--] = 0;

    for (j = 0; j < i; ++j, --i)
    {
        tmp = buf[j];
        buf[j] = buf[i];
        buf[i] = tmp;
    }
    return buf;
}
*/

unsigned int readhexint(unsigned char* buf, int size, int bigendian)
{	int i;
	unsigned int r=0,t;
	for(i=0;i<size;i++)
	{	if(bigendian)	t=buf[size-1-i];
		else		t=buf[i];
		r=(r<<8)|t;
	}
	return r;
}

unsigned int maskright(unsigned int data, unsigned int mask)
{	data=data&mask;
	while(mask && !(mask&0b1))
	{	data=data>>1;
		mask=mask>>1;
	}
	return data;
}

int nbbits(unsigned int mask)
{	int n=0;
	while(mask)
	{	if(mask&0b1) n++;
		mask=mask>>1;
	}
	return n;
}
void _fini(void) {} void _init(void) {}
