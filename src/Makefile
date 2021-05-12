CC=gcc
CFLAGS=-std=gnu11 -pedantic -Wall
LDLIBS=-lncurses
SRCS=http.c ctwin.c main.c net.c gui.c
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
