CC=gcc
CFLAGS=-std=gnu11 -pedantic
LDLIBS=-lncurses
SRCS=http.c rswin.c main.c net.c
OBJS=$(addprefix $(OBJDIR)/,$(SRCS:.c=.o))
DEPS=
BINDIR?=bin

all: $(BINDIR)/ncurest

$(BINDIR)/ncurest: $(OBJS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(LDLIBS) -o $(BINDIR)/ncurest $(OBJS) 

$(OBJDIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(LDLIBS) -c $^ -o $@
