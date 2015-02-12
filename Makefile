CC = gcc
SED = sed
RM = rm -f
CFLAGS = -W -Wall -Wmissing-declarations
CPPFLAGS = 
LDFLAGS = 
LIBS = $(XML_LIBS)

C_SOURCES = xpath.c objects.c errors.c parser.c

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

