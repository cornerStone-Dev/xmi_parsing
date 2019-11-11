
all: xmiCompiler

xmiCompiler: xmi_compiler.c gram_xmi.c xmi_lex.c
	gcc -O2 -march=native -s -o xmiCompiler xmi_compiler.c /opt/sqlite3/sqlite3.o -Wall -Winline -Wignored-attributes -ldl

gram_xmi.c: gram_xmi.y
	./lemon gram_xmi.y -s

xmi_lex.c: xmi_lex.re
	re2c -W xmi_lex.re -o xmi_lex.c

clean:
	rm xmiCompiler
