CC=gcc
LIBS=-lsqlite3
OBJS=database.o config.o lstring.o discname.o dconf.o html.o huffman.o

all: telaradb discovery

telaradb: telaradb.c $(OBJS)
	$(CC) telaradb.c $(OBJS) -o telaradb $(LIBS)
	cp telaradb cgi-bin/telaradb.cgi

discovery: discovery.c $(OBJS)
	$(CC) discovery.c $(OBJS) -o discovery $(LIBS)
	cp discovery cgi-bin/discovery.cgi

%.o:	%.c
	 $(CC) -c -o $@ $<

clean:
	rm -f core $(OBJS) telaradb discovery

distclean:	clean
	rm -f cgi-bin/*
