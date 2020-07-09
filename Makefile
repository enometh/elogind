all: fake-journal.so

fake-journal.so: fake-journal.c
	gcc -shared -fPIC  fake-journal.c -o fake-journal.so -ldl -ggdb
