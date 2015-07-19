DEVELFLAGS =

AM_CFLAGS =
AM_LDFLAGS =
AM_LIBTOOLFLAGS =
AM_CPPFLAGS = -I$(includedir)/umview -DFATWRITE -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE $(DEVELFLAGS)

bin_PROGRAMS = fatutil

COMMONSRC = libfat.c libfat.h bits/libfat.h \
	unicode.h utf16be.c utf16le.c utf8.c \
	simple_fold.c

fatutil_SOURCES = fatutil.c $(COMMONSRC)
fatutil_LDFLAGS = -lpthread
# Leave this also if it seems a no-op
fatutil_CFLAGS = $(AM_CFLAGS)

extraclean: maintainer-clean
	rm -rf aclocal.m4 autom4te.cache autoscan.log configure.scan \
	config.h.in* configure depcomp install-sh Makefile.in \
	config.guess config.sub \
	ltmain.sh missing `find . -name Makefile.in`


