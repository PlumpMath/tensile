ifeq ($(PLATFORM_CC_FAMILY),gcc)
CFLAGS += -O3
else ifeq ($(PLATFORM_CC_FAMILY),clang)
CFLAGS += -O3
else
CFLAGS += -O
endif
