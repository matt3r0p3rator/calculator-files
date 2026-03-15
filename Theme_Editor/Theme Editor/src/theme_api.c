#include "theme_api.h"

# include "symbols.c"

unsigned num_symbols = sizeof(symbolTable)/sizeof(symbolTable[0]);

inline Color getNthColor_mode(int i, int cx) {
	return Color_(colorTable[(i<<1)+(cx)]);
}

inline void setNthColor_mode(int i, int c, int cx) {
	colorTable[(i<<1)+(cx)] = c;
}

inline Color getNthColor(int i) {
	return getNthColor_mode(i, is_cx);
}

inline void setNthColor(int i, int c) {
	setNthColor_mode(i, c, is_cx);
}

inline char *getNthSymbol(int i)
{
	return symbolTable[i];
}

int hex2int(char * s) {
	int n;
	s = s+2; // '0x'
	for(n = 0; *s; ++s) {
		char c = *s;
		int e;
		if(c >= '0' && c <= '9')
			e = c - '0';
		else if(c >= 'A' && c <= 'F')
			e = c - 'A' + 10;
		else if(c >= 'a' && c <= 'f')
			e = c - 'a' + 10;
		else e = 0;
		n = n*16 + e;
	}
	return n;
}

int readTheme(const char * theme_path) {
	FILE * theme = fopen(theme_path, "r");
	if(theme != NULL) { /* If the theme was found */
		fseek(theme, 0, SEEK_END);
		unsigned theme_length = ftell(theme);
		rewind(theme);
		char * whole_theme = (char *) calloc(1, sizeof(char) * (theme_length + 1));
		fread(whole_theme, sizeof(char), theme_length, theme);
		fclose(theme);
		whole_theme[theme_length] = 0;
		char * line = whole_theme;
		char * eol = whole_theme;
		while((unsigned)(eol - whole_theme) < theme_length) {
			eol = strchr(line, '\n');
			*eol = 0;
			char * first_space = strchr(line, ' ');
			if(first_space) {
				char * name = line;
				*first_space = 0;
				char * noncx = first_space + 1;
				char * second_space = strchr(noncx, ' ');
				if(second_space) {
					*second_space = 0;
					char * cx = second_space + 1;						
					unsigned i;
					for(i = 0; i < num_symbols && strcmp(name, symbolTable[i]) != 0; ++i);
					if(i < num_symbols)
					{
						setNthColor_mode(i, hex2int(noncx), 0);
						setNthColor_mode(i, hex2int(cx), 1);
					}
					else
						printf("%s not found %d %d\n", name, num_symbols, i);
				}
			}
			line = eol + 1;
		}
		free(whole_theme);
	}
	else {
		puts("no theme file");
		return -1;
	}
	return 0;
}

void writeTheme(const char * theme_path) {
	FILE * theme = fopen(theme_path, "w");
	unsigned i;
	for(i = 0; i < num_symbols; ++i) {
		Color c = getNthColor(i);
		fprintf(theme, "%s %08X %08X\n", symbolTable[i], c.grey, c.color);
	}
	fclose(theme);
}
