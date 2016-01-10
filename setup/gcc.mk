CFLAGS += -gdwarf-4 -g3 -fstack-protector -W -Wall -Werror -Wmissing-declarations -Wformat=2 -Winit-self -Wuninitialized \
		  -Wsuggest-attribute=pure -Wsuggest-attribute=const -Wconversion -Wstack-protector -Wpointer-arith -Wwrite-strings \
		  -Wmissing-format-attribute
GENERATED_CFLAGS = -Wno-conversion -Wno-unused-function -Wno-suggest-attribute=pure
