# OS type: Linux/Win DJGPP
ifdef OS
   EXE=.exe
else
   EXE=
endif

CFILES   = symbol.c error.c general.c symbtest.c
HFILES   = symbol.h error.h general.h
OBJFILES = $(patsubst %.c,%.o,$(CFILES))
EXEFILES = symbtest

SRCFILES = $(HFILES) $(CFILES) parser.y lexer.l

CC=gcc
CFLAGS=-Wall -ansi -pedantic -g

%.o : %.c
	$(CC) $(CFLAGS) -c $<

parser: lexer.o parser.o symbol.o error.o general.o
	$(CC) $(CFLAGS) -o $@ $^ -lfl

lexer.c: lexer.l
	flex -s -o $@ $<

lexer.o: lexer.c parser.h

parser.c parser.h: parser.y
	bison -v -d -o $@ $<

general.o  : general.c general.h error.h
error.o    : error.c general.h error.h
symbol.o   : symbol.c symbol.h general.h error.h

clean:
	$(RM) $(EXEFILES) $(OBJFILES) *~ parser.output

dist:
	rm -rf compiler-0.1 compiler-0.1.tar.gz
	mkdir compiler-0.1
	cp $(SRCFILES) Makefile compiler-0.1
	tar czf compiler-0.1.tar.gz compiler-0.1
	rm -r compiler-0.1
count:
	wc -l -c Makefile $(SRCFILES)
