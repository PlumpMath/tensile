CC = gcc
SED = sed
RM = rm -f
BISON = bison -d
FLEX = flex
PKGCONFIG = pkg-config
ETAGS = ctags-exuberant -e -R --exclude='doc/*' --exclude='tests/*'
CTANGLE = ctanglex
CWEAVE = cweavex

CFLAGS = -gdwarf-4 -g3 -fstack-protector -W -Wall -Werror -Wmissing-declarations -Wformat=2 -Winit-self -Wuninitialized \
	-Wsuggest-attribute=pure -Wsuggest-attribute=const -Wconversion -Wstack-protector -Wpointer-arith -Wwrite-strings \
	-Wmissing-format-attribute
CPPFLAGS = -I.
LDFLAGS = -Wl,-export-dynamic
LDLIBS = -lm -lfl -lunistring -ltre
MFLAGS = -MM -MT $(<:.c=.o)

LP_SOURCES =
LP_HEADERS = 

C_SOURCES = $(LP_SOURCES:.w=.c)
C_SOURCES += tensile.tab.c lex.yy.c

TESTABLES =

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

%.h : %.w
	$(CTANGLE) $< - $@

tests/% : CPPFLAGS += -I.
tests/% : CFLAGS += --coverage

tests/% : tests/%.o
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS)

tests/%.c : %.c
	touch $@

tests/%.c : %.h
	touch $@

ifneq ($(MAKECMDGOALS),clean)
include $(C_SOURCES:.c=.d)
include $(patsubst %,tests/%.d,$(TESTS))
endif

.PHONY : check
check : $(addprefix tests/,$(TESTABLES))
	@set -e; \
	for t in $(filter tests/%,$^); do \
		$(CHECK_TOOL) ./$$t;		\
	done

.PHONY : doc
doc : manual.tex

manual.tex : $(LP_SOURCES) $(LP_HEADERS)

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
	rm -f $(LP_SOURCES:.w=.c)
	rm -f $(LP_HEADERS:.w=.h)

%.o : %.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

%.d : %.c
	@set -e; $(RM) $@; \
	$(CC) $(MFLAGS) $(CPPFLAGS) $< > $@.$$$$; \
	$(SED) 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	$(RM) $@.$$$$

