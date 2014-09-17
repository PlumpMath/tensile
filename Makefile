CC = gcc
SED = sed
RM = rm -f
SPLINT = splint
SPLINTFLAGS = -checks -posix-lib -isoreserved -declundef -Isplint $(CPPFLAGS)
MFLAGS = -MM
CFLAGS = -W -Wall -Wmissing-declarations
XML_CPPFLAGS := $(shell pkg-config --cflags libexslt)
XML_LIBS := $(shell pkg-config --libs libexslt)
CPPFLAGS = $(XML_CPPFLAGS)
LDFLAGS = 
LIBS = $(XML_LIBS)

C_SOURCES = xpath.c objects.c errors.c

include $(C_SOURCES:.c=.d)

.PHONY : lint
lint : $(C_SOURCES:.c=.o)
	set -e; for f in $(C_SOURCES); do \
		$(SPLINT) $(SPLINTFLAGS) $$f; \
	done


%.o : %.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

%.d : %.c
	@set -e; $(RM) $@; \
	$(CC) $(MFLAGS) $(CPPFLAGS) $< > $@.$$$$; \
	$(SED) 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	$(RM) $@.$$$$

