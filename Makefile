CC=c99
CFLAGS=-Wall -Wextra -Wunreachable-code -ftrapv -fPIC -g -D_POSIX_C_SOURCE=2
LDFLAGS=-lpfxtree -lsndfile
PREFIX=/usr/local
BINDIR=$(DESTDIR)$(PREFIX)/bin

all: sndcrunch

sndcrunch_deps=main.o libsndcrunch.a
sndcrunch: $(sndcrunch_deps)
	$(CC) -o $@ $(sndcrunch_deps) $(LDFLAGS)

libsndcrunch_deps=sndcrunch.o
libsndcrunch.a: $(libsndcrunch_deps)
	$(AR) rcs $@ $(libsndcrunch_deps)

install: sndcrunch
	install -m755 sndcrunch $(BINDIR)/sndcrunch

clean:
	rm -f *.o *.a sndcrunch
