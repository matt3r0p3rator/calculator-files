#define PATH_SIZE 128
#define NBR_SIZE 16
#define NDLESS_CONFIG	"/documents/ndless/ndless.cfg.tns"

#define max(a,b) (((a)<(b))?(b):(a))
#define min(a,b) (((a)<(b))?(a):(b))

void addshellext(char* extension, char* name);
void wait(int timesec);
unsigned int readhexint(unsigned char* buf, int size, int bigendian);
unsigned int maskright(unsigned int data, unsigned int mask);
int nbbits(unsigned int mask);
