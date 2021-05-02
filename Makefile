SRCDIR=$(shell pwd)/src
TESTDIR=$(shell pwd)/test
BINDIR=$(shell pwd)/bin
OBJDIR=$(shell pwd)/obj
export BINDIR
export OBJDIR

.PHONY: build
build:
	$(MAKE) -C $(SRCDIR)

.PHONY: test
test: build
	$(MAKE) -C $(TESTDIR)
	$(BINDIR)/tests

.PHONY: run
run: build
	$(BINDIR)/ncurest

.PHONY: clean
clean:
	rm -rf $(BINDIR)
	rm -rf $(OBJDIR)
