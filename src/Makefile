CC=gcc
CFLAGS=-std=c11 -pedantic
LDLIBS=-lncurses -lform -lpanel -lcurl
SRCS=main.c
OBJS=$(addprefix $(OBJDIR)/,$(SRCS:.c=.o))
DEPS=
BINDIR?=bin

all: $(BINDIR)/ncurlses

$(BINDIR)/ncurlses: $(OBJS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(LDLIBS) -o $(BINDIR)/ncurlses $(OBJS) 

$(OBJDIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(LDLIBS) -c $^ -o $@
