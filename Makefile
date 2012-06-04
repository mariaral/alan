CFILES   = symbol.c error.c general.c quad.c libalan.c typecheck.c llvm.c
GENFILES = lexer.c parser.h parser.c parser.output
OBJFILES = $(patsubst %.c,%.o,$(CFILES)) lexer.o parser.o
EXEFILES = alan

TOP         = $(shell pwd)
GC_SRCPATH  = $(TOP)/gc
GC_INSTPATH = $(GC_SRCPATH)/inplace

CC      = gcc
CXX     = g++
CFLAGS  = -Wall $(shell llvm-config --cflags) -I$(GC_INSTPATH)/include
LDFLAGS = -lfl $(GC_INSTPATH)/lib/libgc.a \
          $(shell llvm-config --ldflags --libs Core BitWriter)


.PHONY: all
all: $(EXEFILES)

$(EXEFILES): $(OBJFILES)
	$(CXX) $(CFLAGS) -o $(EXEFILES) $(OBJFILES) $(LDFLAGS)

%.o : %.c
	$(CC) $(CFLAGS) -c $<

lexer.o : lexer.c
	$(CC) $(CFLAGS) -Wno-implicit-function-declaration -Wno-unused-function -c $<

lexer.c: lexer.l
	flex -s -o $@ $<

parser.c: parser.y
	bison -v -d -o $@ $<

.PHONY: clean
clean:
	$(RM) $(EXEFILES) $(OBJFILES) $(GENFILES) *~

.PHONY: distclean
distclean: gc_clean clean

##############################
## Hans Boehm garbage collector
.INTERMEDIATE: gc.h
gc.h: $(GC_INSTPATH)/include/gc.h

$(GC_SRCPATH)/Makefile:
	echo "Configuring gc.."
	cd $(GC_SRCPATH) && \
		./configure --enable-threads=no --prefix=$(GC_INSTPATH)

$(GC_INSTPATH)/include/gc.h: $(GC_SRCPATH)/Makefile
	echo "Building gc.."
	$(MAKE) -C $(GC_SRCPATH)
	$(MAKE) -C $(GC_SRCPATH) install

.PHONY: gc_clean
gc_clean:
	echo "Cleaning gc.."
	@test -e $(GC_SRCPATH)/Makefile \
		&& $(MAKE) gc_clean_force || echo "Nothing to do here"

.PHONY: gc_clean_force
gc_clean_force:
	$(MAKE) -C $(GC_SRCPATH) maintainer-clean
	$(RM) -r $(GC_INSTPATH)/*

##############################
## Dependences
symbol.o:    general.h error.h symbol.h
error.o:     general.h error.h
general.o:   general.h error.h symbol.h gc.h
quad.o:      general.h error.h quad.h typecheck.h
libalan.o:   symbol.h
typecheck.o: quad.h error.h
llvm.o:		 llvm.h

lexer.o:  symbol.h quad.h parser.h
parser.o: general.h error.h symbol.h quad.h libalan.h typecheck.h llvm.h gc.h
parser.h: parser.c
