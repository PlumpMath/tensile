CC = gcc
SED = sed
RM = rm -f
BISON = bison -d
FLEX = flex
PKGCONFIG = pkg-config

CFLAGS = -O2 -fstack-protector -fmudflapth -W -Wall -Werror -Wmissing-declarations -Wformat=2 -Winit-self -Wuninitialized \
	-Wsuggest-attribute=pure -Wsuggest-attribute=const -Wconversion -Wstack-protector -Wpointer-arith -Wwrite-strings \
	-Wmissing-format-attribute
CPPFLAGS = $(APR_CPPFLAGS) $(APU_CPPFLAGS)
LDFLAGS = $(APR_LDFLAGS) $(APU_LDFLAGS) -Wl,-export-dynamic
LIBS = $(APU_LIBS) $(APR_LIBS) -lm -lfl -lunistring -lmudflap
MFLAGS = -MM

APR_PKGCONFIG = $(PKGCONFIG) apr-1
APU_PKGCONFIG = $(PKGCONFIG) apr-util-1

APR_CPPFLAGS := $(shell $(APR_PKGCONFIG) --cflags)
APU_CPPFLAGS := $(shell $(APU_PKGCONFIG) --cflags)

APR_LDFLAGS := $(shell $(APR_PKGCONFIG) --libs-only-L)
APU_LDLAGS := $(shell $(APU_PKGCONFIG) --libs-only-L)

APR_LIBS := $(shell $(APR_PKGCONFIG) --libs-only-l)
APU_LIBS := $(shell $(APU_PKGCONFIG) --libs-only-l)

C_SOURCES = utils.c tensile.tab.c lex.yy.c

APPLICATION = tensile

$(APPLICATION) : $(C_SOURCES:.c=.o)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)


tensile.tab.h : tensile.tab.c
	touch $@

tensile.tab.c : tensile.y
	$(BISON) $<

lex.yy.c : tensile.l
	$(FLEX) $<

include $(C_SOURCES:.c=.d)

%.o : %.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

%.d : %.c
	@set -e; $(RM) $@; \
	$(CC) $(MFLAGS) $(CPPFLAGS) $< > $@.$$$$; \
	$(SED) 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	$(RM) $@.$$$$

