CC=gcc
LIBS=-lsqlite3
OBJS=database.o config.o lstring.o discname.o dconf.o html.o huffman.o tpath.o

all: telaradb discovery

telaradb: telaradb.c $(OBJS) common.h
	$(CC) telaradb.c $(OBJS) -o telaradb $(LIBS)
	cp telaradb cgi-bin/telaradb.cgi

discovery: discovery.c $(OBJS) common.h
	$(CC) discovery.c $(OBJS) -o discovery $(LIBS)
	cp discovery cgi-bin/discovery.cgi

%.o:	%.c common.h
	 $(CC) -c -o $@ $<

clean:
	rm -f core $(OBJS) telaradb discovery

distclean:	clean
	rm -f cgi-bin/*
