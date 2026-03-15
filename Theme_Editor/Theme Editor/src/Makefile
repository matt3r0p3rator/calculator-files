# This option enables to compile ThemeEditor with the native GUI.
# It requires nframe and nGC that are libraries composed of +200 syscalls
# still under developpement and may not be distribued until it reaches
# a certain level of security.
MAKE_GUI = 1
INCLUDES_NFRAME = ../../../nFrame/nFrame/src/
INCLUDES_NGC = ../../../nGC/src/

GCC = nspire-gcc
LD = nspire-ld
GCCFLAGS = -O2 -W -marm # -Wall 
ifeq (${MAKE_GUI}, 1)
	GCCFLAGS := $(GCCFLAGS) -DGUI
endif

EXE = ThemeEditor.tns
OBJS = color.o draw.o theme_api.o config.o ThemeEditor.o
ifeq (${MAKE_GUI}, 1)
	OBJS := $(OBJS) Gui.o
endif
DISTDIR = ..

LDFLAGS = 
INCLUDES =
ifeq (${MAKE_GUI}, 1)
	INCLUDES := $(INCLUDES) -I${INCLUDES_NGC} -I${INCLUDES_NFRAME}
endif

OBJCOPY := "$(shell which arm-elf-objcopy 2>/dev/null)"
ifeq (${OBJCOPY},"")
	OBJCOPY := arm-none-eabi-objcopy
endif
vpath %.tns $(DISTDIR)

all: $(EXE)

%.o: %.c
	$(GCC) $(INCLUDES) $(GCCFLAGS) -c $<

$(EXE): $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $(@:.tns=.elf)
	mkdir -p $(DISTDIR)
	$(OBJCOPY) -O binary $(@:.tns=.elf) $(DISTDIR)/$@

clean:
	rm -f *.o *.elf *~
