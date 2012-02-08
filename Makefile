# OS type: Linux/Win DJGPP
ifdef OS
   EXE=.exe
else
   EXE=
endif

CFLAGS=-g -Wall
CC=gcc

parser$(EXE): lexer.o parser.o
	$(CC) $(CFLAGS) -o $@ $^ -lfl

lexer.c: lexer.l
	flex -s -o $@ $<

lexer.o: lexer.c parser.h

parser.c parser.h: parser.y
	bison -v -d -o $@ $<

.PHONY: clean distclean

clean:
	$(RM) lexer.c parser.c parser.h parser.output *.o *~

distclean: clean
	$(RM) parser$(EXE)

