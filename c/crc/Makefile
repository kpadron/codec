CC = clang
AR = ar

CPPFLAGS = -I inc/ -I ../shared/inc/
CFLAGS = -std=c99 -Wall -Wextra
CCFLAGS = -fPIC -flto
LDFLAGS = -fPIC -flto -Wl,-L../shared/,-lshared, -Wl,-rpath,../shared/
ARFLAGS = rcs
DEPFLAGS = -MMD -MP -MT $@ -MF $*.d

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
DEPS = $(SRC:.c=.d)

TARGET = crc libcrc.so #libcrc.a

default: release

clean:
	$(RM) $(DEPS) $(OBJ) $(TARGET)

clean_shared:
	$(MAKE) -C ../shared/ clean

release: CPPFLAGS += -DNDEBUG
release: CCFLAGS += -O2
release: LDFLAGS += -O2 -s -Wl,-O2,-s
release: release_shared $(TARGET)

release_shared:
	$(MAKE) -C ../shared/ release

debug: CCFLAGS += -g -ggdb3 -Og
debug: debug_shared $(TARGET)

debug_shared:
	$(MAKE) -C ../shared/ debug

libcrc.a: $(OBJ)
	$(AR) $(ARFLAGS) $@ $^

libcrc.so: $(OBJ)
	$(CC) $(LDFLAGS) -shared $^ -o $@

crc: $(OBJ)
	$(CC) $(LDFLAGS) -pie $^ -o $@

%.o: %.c
	$(CC) $(DEPFLAGS) $(CPPFLAGS) $(CFLAGS) $(CCFLAGS) -c $< -o $@

$(DEPS):
-include $(wildcard $(DEPS))
