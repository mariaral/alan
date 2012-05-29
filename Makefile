GCLIB	 = ./gc/lib/libgc.a
CFILES   = symbol.c error.c general.c quad.c libalan.c typecheck.c
HFILES   = symbol.h error.h general.h quad.h libalan.h typecheck.h
GENFILES = lexer.c parser.h parser.c parser.output
OBJFILES = $(patsubst %.c,%.o,$(CFILES)) lexer.o parser.o
EXEFILES = alan

SRCFILES = $(HFILES) $(CFILES) parser.y lexer.l

CC=gcc
CFLAGS=-Wall -ansi -pedantic -g -I ./gc/include


all: $(OBJFILES)
	$(CC) $(CFLAGS) -o $(EXEFILES) $(OBJFILES) $(GCLIB) -lfl

lexer.o : lexer.c
	$(CC) $(CFLAGS) -Wno-implicit-function-declaration -Wno-unused-function -c $<

%.o : %.c
	$(CC) $(CFLAGS) -c $<

lexer.c: lexer.l parser.h
	flex -s -o $@ $<

parser.h: parser.c

parser.c: parser.y
	bison -v -d -o $@ $<

clean:
	$(RM) $(EXEFILES) $(OBJFILES) $(GENFILES) *~

dist:
	rm -rf compiler-0.1 compiler-0.1.tar.gz
	mkdir compiler-0.1
	cp $(SRCFILES) compiler-0.1
	cp Makefile README.md compiler-0.1
	tar czf compiler-0.1.tar.gz compiler-0.1
	rm -r compiler-0.1
count:
	wc -l -c Makefile $(SRCFILES)
