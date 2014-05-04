CC=gcc
CFLAGS=-g -Wall -I/usr/local/include
LDFLAGS=-lutil -lc -lm -L/usr/local/lib -lconfig
SRCS=tunneld.c arg_parse.c
INSTALL=/usr/bin/install
INSTALLDIR=/home/tunnel/bin
RCDIR=/usr/local/etc/rc.d
MANDIR=/usr/local/man
BINNAME=tunneld
OBJS=${SRCS:.c=.o}
HFILES=arg_parse.h

$(BINNAME): $(OBJS)
	$(CC) $(CFLAGS) -o $(BINNAME) $(OBJS) $(LDFLAGS) 

$(OBJS): $(HFILES)

build: $(BINNAME)

makedirs: 
	$(INSTALL) -d $(INSTALLDIR)

$(BINNAME).8.gz:
	gzip -c $(BINNAME).8 > $(BINNAME).8.gz

install-rc: 
	$(INSTALL) $(BINNAME)-rc $(RCDIR)/$(BINNAME)

install-man: $(BINNAME).8.gz
	$(INSTALL) $(BINNAME).8.gz $(MANDIR)/$(BINNAME)

install-bin: $(BINNAME)
	$(INSTALL) $(BINNAME) $(INSTALLDIR)/$(BINNAME)

install: makedirs install-bin install-rc install-man
