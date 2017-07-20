XCC_HDR = xcc.h  AST.h type.h symbol.h misc.h codegen.h
XCC_SRC =        AST.c type.c symbol.c misc.c codegen.c
WARNING = -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations

all: xcc

xcc:  $(XCC_SRC) $(XCC_HDR) xcc.tab.c xcc.tab.h lex.yy.c 
	gcc $(WARNING) -g -o xcc $(XCC_SRC) xcc.tab.c lex.yy.c 

xcc.tab.c xcc.tab.h xcc.output: xcc.y
	bison -d -v xcc.y

lex.yy.c: xcc.l xcc.tab.h
	flex xcc.l

clean:
	-rm -f xcc a.out *.o *.s lex.yy.c xcc.tab.c xcc.tab.h xcc.output *~ \#*\#
	-rm -rf xcc.dSYM

wc:
	make clean
	wc -l *.[chly]
