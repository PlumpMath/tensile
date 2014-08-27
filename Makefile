CC = gcc
SED = sed
RM = rm -f
SPLINT = splint
SPLINTFLAGS = -standard -posix-lib -Isplint $(CPPFLAGS)
MFLAGS = -MM
CFLAGS = -W -Wall
XML_CPPFLAGS := $(shell pkg-config --cflags libexslt)
XML_LIBS := $(shell pkg-config --libs libexslt)
CPPFLAGS = $(XML_CPPFLAGS)
LDFLAGS = 
LIBS = $(XML_LIBS)

C_SOURCES = xpath.c objects.c

include $(C_SOURCES:.c=.d)

%.o : %.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<
	$(SPLINT) $(SPLINTFLAGS) $<

%.d : %.c
	@set -e; $(RM) $@; \
	$(CC) $(MFLAGS) $(CPPFLAGS) $< > $@.$$$$; \
	$(SED) 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	$(RM) $@.$$$$

