# Makefile for FaztWeb
#
PREFIX?=/usr
DESTDIR?=

PACKAGE=faztweb
VERSION=1.8

CC?=gcc

all:
	$(CC) src/main.c -o $(PACKAGE) \
		`pkg-config --cflags --libs gtk+-2.0 webkit-1.0`
	@du -sh $(PACKAGE)

install:
	mkdir -p \
		$(DESTDIR)$(PREFIX)/bin \
		$(DESTDIR)$(PREFIX)/share/faztweb \
		$(DESTDIR)$(PREFIX)/share/pixmaps \
		$(DESTDIR)$(PREFIX)/share/applications
	install -m 0755 $(PACKAGE) $(DESTDIR)$(PREFIX)/bin
	install -m 0644 data/faztweb.png $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 0644 data/faztweb.desktop $(DESTDIR)$(PREFIX)/share/applications
clean:
	rm -f $(PACKAGE)
	rm -f src/Makefile src/*.o 

help:
	@echo "make [ install | clean ]"
