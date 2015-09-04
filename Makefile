CC = gcc
SED = sed
RM = rm -f
BISON = bison -d
FLEX = flex
PKGCONFIG = pkg-config
DOXYGEN = doxygen
ETAGS = ctags-exuberant -e -R --exclude='doc/*' --exclude='tests/*'

CFLAGS = -ggdb3 -fstack-protector -W -Wall -Werror -Wmissing-declarations -Wformat=2 -Winit-self -Wuninitialized \
	-Wsuggest-attribute=pure -Wsuggest-attribute=const -Wconversion -Wstack-protector -Wpointer-arith -Wwrite-strings \
	-Wmissing-format-attribute
CPPFLAGS = -I.
LDFLAGS = -Wl,-export-dynamic
LIBS = -lm -lfl -lunistring -ltre
CUNIT_CPPFLAGS = 
CUNIT_LIBS = -lcunit
MFLAGS = -MM -MT $(<:.c=.o)

C_SOURCES = tensile.tab.c lex.yy.c

TESTS = allocator support stack queue

C_TEST_SOURCES = tests/engine.c
C_TEST_SOURCES += $(patsubst %,tests/%.c,$(TESTS))

APPLICATION = tensile
TESTENGINE = tests/testengine

$(APPLICATION) : $(C_SOURCES:.c=.o)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

$(TESTENGINE) : $(C_TEST_SOURCES:.c=.o)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS) $(CUNIT_LIBS)


tensile.tab.h : tensile.tab.c
	touch $@

tensile.tab.c : tensile.y
	$(BISON) $<

tensile.tab.o lex.yy.o : CFLAGS += -Wno-conversion -Wno-unused-function -Wno-suggest-attribute=pure

tests/%.o : CPPFLAGS += $(CUNIT_CPPFLAGS)

lex.yy.c : tensile.l
	$(FLEX) $<

lex.yy.o lex.yy.d : lex.yy.c tensile.tab.h

include $(C_SOURCES:.c=.d)
include $(C_TEST_SOURCES:.c=.d)

.PHONY : doc
doc :
	$(DOXYGEN) Doxyfile

TAGS : $(C_SOURCES)
	$(ETAGS)


.PHONY : clean
clean :
	rm -f *.o tests/*.o
	rm -f *.d tests/*.d
	rm -f $(APPLICATION)
	rm -f $(TESTENGINE)
	rm -f lex.yy.c
	rm -f tensile.tab.*

%.o : %.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

%.d : %.c
	@set -e; $(RM) $@; \
	$(CC) $(MFLAGS) $(CPPFLAGS) $< > $@.$$$$; \
	$(SED) 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	$(RM) $@.$$$$

