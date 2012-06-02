CFILES   = symbol.c error.c general.c quad.c libalan.c typecheck.c
GENFILES = lexer.c parser.h parser.c parser.output

OBJFILES = $(patsubst %.c,%.o,$(CFILES)) lexer.o parser.o
EXEFILES = alan

CC     = gcc
CFLAGS = -Wall -ansi -pedantic -g -I ./gc/inplace/include
GCLIB  = ./gc/inplace/lib/libgc.a


all: $(OBJFILES)
	$(CC) $(CFLAGS) -o $(EXEFILES) $(OBJFILES) $(GCLIB) -lfl

%.o : %.c
	$(CC) $(CFLAGS) -c $<

lexer.o : lexer.c
	$(CC) $(CFLAGS) -Wno-implicit-function-declaration -Wno-unused-function -c $<

lexer.c: lexer.l
	flex -s -o $@ $<

parser.c: parser.y
	bison -v -d -o $@ $<

clean:
	$(RM) $(EXEFILES) $(OBJFILES) $(GENFILES) *~

distclean: gc_clean clean

##############################
## Hans Boehm garbage collector
gc/Makefile:
	echo "Configuring gc.."
	cd gc && ./configure --enable-threads=no \
		--prefix=`pwd`/inplace

gc/inplace/include/gc.h: gc/Makefile
	echo "Building gc.."
	$(MAKE) -C gc
	$(MAKE) -C gc install

gc_clean:
	echo "Cleaning gc.."
	@test -e gc/Makefile && $(MAKE) gc_clean_force || echo "Nothing to do here"

gc_clean_force:
	$(MAKE) -C gc maintainer-clean
	$(RM) -r gc/inplace/*

##############################
## Dependences
symbol.o:    general.h error.h symbol.h
error.o:     general.h error.h
general.o:   general.h error.h symbol.h gc.h
quad.o:      general.h error.h quad.h typecheck.h
libalan.o:   symbol.h
typecheck.o: quad.h error.h

gc.h: gc/inplace/include/gc.h

lexer.o:  parser.h
parser.o: parser.h
parser.h: parser.c
