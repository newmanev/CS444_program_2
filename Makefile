#
# R Jesse Chaney
# 

CC = gcc
DEBUG = -g
DEFINES =
DEFINES += -DCHECK

CFLAGS = $(DEBUG) -Wall -Wshadow -Wunreachable-code -Wredundant-decls \
        -Wmissing-declarations -Wold-style-definition -Wmissing-prototypes \
        -Wdeclaration-after-statement $(DEFINES)
PROG = beavalloc


all: $(PROG)


beavalloc: beavalloc.o main.o
	$(CC) $(CFLAGS) -o $@ $^
	chmod a+rx,g-w $@

beavalloc.o: beavalloc.c beavalloc.h
	$(CC) $(CFLAGS) -c $<

main.o: main.c beavalloc.h
	$(CC) $(CFLAGS) -c $<

opt: clean
	make DEBUG=-O3

tar: clean
	tar cvfz $(PROG).tar.gz *.[ch] ?akefile

# clean up the compiled files and editor chaff
clean cls:
	rm -f $(PROG) *.o *~ \#*

ci:
	ci -m"auto-checkin" -l *.[ch] ?akefile
