CC = gcc
AR = gcc-ar
NM = gcc-nm
RANLIB = gcc-ranlib
RM = rm -rf

INCDIR = inc
SRCDIR = src
OBJDIR = .obj
LIBDIR = lib

LIBSRC = crc.c
DLLSRC = entry.c
EXESRC = main.c

LIBOBJ = $(LIBSRC:%.c=$(OBJDIR)/%.o)
DLLOBJ = $(DLLSRC:%.c=$(OBJDIR)/%.o)
EXEOBJ = $(EXESRC:%.c=$(OBJDIR)/%.o)
OBJ = $(LIBOBJ) $(DLLOBJ) $(EXEOBJ)

LIB = $(LIBDIR)/libcrc.a
DLL = $(LIBDIR)/libcrc.so
TARGET = crc.exe

INC = -I $(INCDIR) -I ../inc/

vpath %.h $(INCDIR)
vpath %.c $(SRCDIR)

CPPFLAGS = $(INC)
CFLAGS = -std=c99 -Wall -Wextra
CCFLAGS = -march=native -fopenmp -fPIC -flto=auto -fuse-linker-plugin -ffat-lto-objects
LDFLAGS = -L $(LIBDIR) -Wl,-z,relro,-z,now
LDLIBS = $(LIB)
ARFLAGS = -rUcus
DEPFLAGS = -MMD -MP -MT $@ -MF $(OBJDIR)/$*.d

DEPS = $(OBJ:%.o=%.d)

default: release

clean:
	$(RM) $(OBJDIR) $(LIBDIR) $(TARGET)

release: CPPFLAGS += -DRELEASE -DNDEBUG
release: CCFLAGS += -O3
release: $(TARGET)

debug: CPPFLAGS += -DDEBUG
debug: CCFLAGS += -g -ggdb3 -Og
debug: $(TARGET)

%.exe: $(EXEOBJ) $(LIB)
	$(CC) $(CCFLAGS) $(LDFLAGS) -pie $(EXEOBJ) $(LDLIBS) -o $@

$(LIBDIR)/%.so: $(DLLOBJ) $(LIB) | $(LIBDIR)
	$(CC) $(CCFLAGS) $(LDFLAGS) -shared $(DLLOBJ) $(LDLIBS) -o $@

$(LIBDIR)/%.a: $(LIBOBJ) | $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(DEPFLAGS) $(CPPFLAGS) $(CFLAGS) $(CCFLAGS) -c $< -o $@

$(LIBDIR):
	@mkdir -p $@

$(OBJDIR):
	@mkdir -p $@

$(DEPS):
-include $(wildcard $(DEPS))

.PRECIOUS: $(OBJDIR)/%.o $(LIBDIR)/%.a $(LIBDIR)/%.so
