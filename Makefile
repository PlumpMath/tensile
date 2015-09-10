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
LDLIBS = -lm -lfl -lunistring -ltre
CHECK_CPPFLAGS = 
CHECK_LDLIBS = -lcheck -lrt 																																				-lpthread 
MFLAGS = -MM -MT $(<:.c=.o)
CHECKMK = checkmk

C_SOURCES = tensile.tab.c lex.yy.c

TESTS = support allocator stack queue taglist

APPLICATION = tensile

$(APPLICATION) : $(C_SOURCES:.c=.o)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS)

tensile.tab.h : tensile.tab.c
	touch $@

tensile.tab.c : tensile.y
	$(BISON) $<

tensile.tab.o lex.yy.o : CFLAGS += -Wno-conversion -Wno-unused-function -Wno-suggest-attribute=pure

lex.yy.c : tensile.l
	$(FLEX) $<

lex.yy.o lex.yy.d : lex.yy.c tensile.tab.h

tests/%.o : CPPFLAGS += $(CHECK_CPPFLAGS)
tests/% : LDLIBS += $(CHECK_LDLIBS)

%.c : %.ts
	$(CHECKMK) $< >$@


ifneq ($(MAKECMDGOALS),clean)
include $(C_SOURCES:.c=.d)
include $(patsubst %,tests/%.d,$(TESTS))
endif

.PHONY : check
check : $(addprefix tests/,$(TESTS))
	set -e; \
	for t in $^; do \
		$(CHECK_TOOL) ./$$t;		\
	done

.PHONY : doc
doc :
	$(DOXYGEN) Doxyfile

TAGS : $(C_SOURCES)
	$(ETAGS)


.PHONY : clean 
clean :
	rm -f *.o tests/*.o
	rm -f *.d tests/*.d
	rm -f tests/*.c
	rm -f $(APPLICATION)
	rm -f lex.yy.c
	rm -f tensile.tab.*

%.o : %.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

%.d : %.c
	@set -e; $(RM) $@; \
	$(CC) $(MFLAGS) $(CPPFLAGS) $< > $@.$$$$; \
	$(SED) 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	$(RM) $@.$$$$

