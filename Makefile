CC = gcc
SED = sed
RM = rm -f
MFLAGS = -MM
CFLAGS = -W -Wall
XML_CPPFLAGS := $(shell pkg-config --cflags libexslt)
XML_LIBS := $(shell pkg-config --libs libexslt)
CPPFLAGS = $(XML_CPPFLAGS)
LDFLAGS = 
LIBS = $(XML_LIBS)

C_SOURCES = xpath.c

include $(C_SOURCES:.c=.d)

%.d : %.c
	@set -e; $(RM) $@; \
	$(CC) $(MFLAGS) $(CPPFLAGS) $< > $@.$$$$; \
	$(SED) 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	$(RM) $@.$$$$

