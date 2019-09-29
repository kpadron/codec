CC = clang
AR = ar
RM = rm -rf

INCDIR = inc
SRCDIR = src
OBJDIR = .obj

LIBSRC = rle.c
EXESRC = main.c

LIBOBJ = $(LIBSRC:%.c=$(OBJDIR)/%.o)
EXEOBJ = $(EXESRC:%.c=$(OBJDIR)/%.o)
OBJ = $(LIBOBJ) $(EXEOBJ)

TARGET = rle librle.so #librle.a

vpath %.h $(INCDIR)
vpath %.c $(SRCDIR)

CPPFLAGS = -I $(INCDIR) -I ../shared/inc/
CFLAGS = -std=c99 -Wall -Wextra
CCFLAGS = -fPIC -flto
LDFLAGS = -fPIC -flto -Wl,-L../shared/,-lshared, -Wl,-rpath,../shared/
ARFLAGS = rcs
DEPFLAGS = -MMD -MP -MT $@ -MF $(OBJDIR)/$*.d

DEPS = $(OBJ:%.o=%.d)

default: release

clean:
	$(RM) $(OBJDIR) $(TARGET)

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

%.a: $(LIBOBJ)
	$(AR) $(ARFLAGS) $@ $^

%.so: $(LIBOBJ)
	$(CC) $(LDFLAGS) -shared $^ -o $@

rle: $(LIBOBJ) $(EXEOBJ)
	$(CC) $(LDFLAGS) -pie $^ -o $@

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(DEPFLAGS) $(CPPFLAGS) $(CFLAGS) $(CCFLAGS) -c $< -o $@

$(OBJDIR):
	@mkdir -p $@

$(DEPS):
-include $(wildcard $(DEPS))
