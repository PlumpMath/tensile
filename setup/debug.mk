ifeq ($(PLATFORM_CC_FAMILY),gcc)
CFLAGS += -gdwarf-4 -g3
else ifeq ($(PLATFORM_CC_FAMILY),clang)
CFLAGS += -gdwarf-4 -g3 
else
CFLAGS += -g
endif
