all: fake-journal.so a.out

ELOGIND_CFLAGS=$(shell pkg-config --cflags libelogind)
ELOGIND_LIBS=$(shell pkg-config --libs libelogind)

fake-journal.so: fake-journal.c
	$(CC) -ggdb -shared -fPIC -ldl -o $@ $< $(ELOGIND_CFLAGS)

a.out: test.c
	$(CC) -ggdb -o $@ $^ $(ELOGIND_LIBS) $(ELOGIND_CFLAGS)

clean:
	rm -fv a.out fake-journal.so