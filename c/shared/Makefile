CC = clang
AR = ar
RM = rm -rf

INCDIR = inc
SRCDIR = src
OBJDIR = .obj

SRC = utility.c hash.c codec.c
OBJ = $(SRC:%.c=$(OBJDIR)/%.o)

TARGET = libshared.so #libshared.a

vpath %.h $(INCDIR)
vpath %.c $(SRCDIR)

CPPFLAGS = -I $(INCDIR)
CFLAGS = -std=c99 -Wall -Wextra
CCFLAGS = -fPIC -flto
LDFLAGS = -fPIC -flto
ARFLAGS = rcs
DEPFLAGS = -MMD -MP -MT $@ -MF $(OBJDIR)/$*.d

DEPS = $(OBJ:%.o=%.d)

default: release

clean:
	$(RM) $(OBJDIR) $(TARGET)

release: CPPFLAGS += -DNDEBUG
release: CCFLAGS += -O2
release: LDFLAGS += -O2 -s -Wl,-O2,-s
release: $(TARGET)

debug: CCFLAGS += -g -ggdb3 -Og
debug: $(TARGET)

libshared.a: $(OBJ)
	$(AR) $(ARFLAGS) $@ $^

libshared.so: $(OBJ)
	$(CC) $(LDFLAGS) -shared $^ -o $@

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(DEPFLAGS) $(CPPFLAGS) $(CFLAGS) $(CCFLAGS) -c $< -o $@

$(OBJDIR):
	@mkdir -p $@

$(DEPS):
-include $(wildcard $(DEPS))
