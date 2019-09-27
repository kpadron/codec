CC = gcc
AR = gcc-ar

CPPFLAGS = -I inc/
CFLAGS = -std=c99 -Wall -flto -ffat-lto-objects -fPIC
LDFLAGS = -flto=auto -shared
ARFLAGS = rcs
DEPFLAGS = -MMD -MP -MT $@ -MF $*.d

DEBUG = -g -Og
RELEASE = -O3 -DNDEBUG
OPT = $(RELEASE)

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
DEPS = $(SRC:.c=.d)

TARGET = libshared.a libshared.so hash

default: release

clean:
	$(RM) $(DEPS) $(OBJ) $(TARGET)

release: CFLAGS += $(RELEASE)
release: LDFLAGS += $(RELEASE)
release: $(TARGET)

debug: CFLAGS += $(DEBUG)
debug: LDFLAGS += $(DEBUG)
debug: $(TARGET)

libshared.a: $(OBJ)
	$(AR) $(ARFLAGS) $@ $^

libshared.so: $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

hash: libshared.a
	$(CC) -flto=auto -L. $^ -o $@

%.o: %.c %.d
	$(CC) $(DEPFLAGS) $(CPPFLAGS) $(CFLAGS) $(MODE) -c $< -o $@

$(DEPS):
-include $(wildcard $(DEPS))
