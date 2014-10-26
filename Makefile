all:
	bison -d parser.y
	flex -o lexer.{c,l}
	gcc -o shell parser.tab.c lexer.c shl.c -lreadline -ll
	rm parser.tab.c parser.tab.h lexer.c
