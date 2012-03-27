
CFILES   = symbol.c error.c general.c quad.c
HFILES   = symbol.h error.h general.h quad.h
OBJFILES = $(patsubst %.c,%.o,$(CFILES))
EXEFILES = alan

SRCFILES = $(HFILES) $(CFILES) parser.y lexer.l

CC=gcc
CFLAGS=-Wall -ansi -pedantic -g


all: $(OBJFILES)
	$(CC) $(CFLAGS) -o $(EXEFILES) $(OBJFILES) -lfl

%.o : %.c parser.y
	$(CC) $(CFLAGS) -c $<

lexer.c: lexer.l
	flex -s -o $@ $<

parser.c parser.h: parser.y
	bison -v -d -o $@ $<

clean:
	$(RM) $(EXEFILES) $(OBJFILES) *~ parser.output

dist:
	rm -rf compiler-0.1 compiler-0.1.tar.gz
	mkdir compiler-0.1
	cp $(SRCFILES) compiler-0.1
	cp Makefile README.md compiler-0.1
	tar czf compiler-0.1.tar.gz compiler-0.1
	rm -r compiler-0.1
count:
	wc -l -c Makefile $(SRCFILES)
