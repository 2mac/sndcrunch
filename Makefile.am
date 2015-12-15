ACLOCAL_AMFLAGS = -I m4 --install
dist_doc_DATA = README

noinst_LIBRARIES = libsndcrunch.a
bin_PROGRAMS = sndcrunch

libsndcrunch_a_SOURCES = sndcrunch.c
libsndcrunch_a_CFLAGS = -Wall -Wextra -Wunreachable-code -ftrapv -std=c11 \
                        $(sndfile_CFLAGS)

sndcrunch_SOURCES = main.c
sndcrunch_CFLAGS = -Wall -Wextra -Wunreachable-code -ftrapv -std=c11
sndcrunch_LDADD = libsndcrunch.a $(pfxtree_LIBS) $(sndfile_LIBS)

AM_CFLAGS = $(DEPS_CFLAGS)
AM_LIBS = $(DEPS_LIBS)
