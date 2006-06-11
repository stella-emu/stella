# $Id: Makefile,v 1.1 2006-06-11 07:13:20 urchlay Exp $


SRCDIR       = .
CC           = gcc
RANLIB       = ranlib
PREFIX       = /usr/local
BIN_DIR      = $(PREFIX)/bin
LIB_DIR      = $(PREFIX)/lib/dict
LDLIBS       = -lm -lgdbm -ldb -L/usr/local/lib -lasound 
XLIBS        =  -L/usr/X11R6/lib64 -lXt  -L/usr/X11R6/lib64 -lX11 -lnsl 
CFLAGS       = -g -O2
CPPFLAGS     = -I. -I.  -I/usr/local/include -I/usr/X11R6/include
DEFS         = -DDICT_DIR=\"$(LIB_DIR)\" -DDEFAULT_DICT=\"$(DEFAULT_DICT)\"
PROGS        = say mkdictdb dlookup mkdictdb dlookup
ADICT        = 
BDICT        = 
BUILD        =  perlstuff
DICTS        = 
PERL         = /usr/bin/perl
FESTIVAL     = 
DEFAULT_DICT = b
INSTALL      = /usr/bin/ginstall -c
INSTALL_PROG = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
EXTRA_INST   =  install_perl
MESSAGE      = Hello there
LIBRARY      = Rsynth/librsynth.a

.c.o :
	$(CC) $(CFLAGS) $(CPPFLAGS) -IRsynth $(DEFS) -c -o $@ $<

all :: $(PROGS) $(BUILD)

check :: all perl_check
	(./say $(MESSAGE) || ./nasay $(MESSAGE))


SYNOBJ   = opsynth.o elements.o holmes.o trie.o phtoelm.o darray.o
PHONOBJ  = say.o phones.o text.o english.o deutsch.o charset.o

$(LIBRARY) : $(SYNOBJ) $(PHONOBJ)
	$(AR) rc $@ $(SYNOBJ) $(PHONOBJ)
	$(RANLIB) $@

SAY_OBJS = $(PHONOBJ) l2u.o aufile.o getarg.o Revision.o dict.o  saymain.o

say  : $(LIBRARY) $(SAY_OBJS)  hplay.o
	$(CC) -o $@ $(LDFLAGS) $(SAY_OBJS) $(SYNOBJ) hplay.o $(LIBRARY) $(LDLIBS)
	
nasay : $(LIBRARY) $(SAY_OBJS) naplay.o
	$(CC) -o $@ $(LDFLAGS) $(SAY_OBJS) $(SYNOBJ) naplay.o $(LIBRARY) $(XLIBS) $(LDLIBS)

mkdictdb : mkdictdb.o $(LIBRARY)
	$(CC) -o $@ $(LDFLAGS) mkdictdb.o $(LIBRARY) $(LDLIBS)

dlookup  : dlookup.o getarg.o $(LIBRARY)
	$(CC) -o $@ $(LDFLAGS) dlookup.o getarg.o dict.o $(LIBRARY) $(LDLIBS)

clean ::
	if test -f Rsynth/Makefile ; then cd Rsynth && make PERL=$(PERL) clean ; fi  	
	rm -f $(LIBRARY) $(PROGS) $(DICTS) *.o tmp.par core *~ *%  ,* .e[cks]1

distclean :: clean
	if test -f Rsynth/Makefile ; then cd Rsynth && make PERL=$(PERL) distclean ; fi  	
	rm -f Makefile config.status config.h config.log config.cache hplay.c


hplay.o : hplay.c getargs.h hplay.h

aDict.db : $(ADICT) mkdictdb
	LANG=en_US mkdictdb $(ADICT) aDict.db
	touch $@

bDict.db : $(BDICT) mkdictdb
	LANG=en_GB mkdictdb $(BDICT) bDict.db
	touch $@

install  : $(PROGS) $(DICTS) $(EXTRA_INST)
	mkdir -p $(PREFIX) $(BIN_DIR) $(LIB_DIR)
	for f in $(PROGS); do \
	$(INSTALL_PROG) $$f $(BIN_DIR); \
	done
	$(INSTALL) -d $(LIB_DIR)
	for f in $(DICTS); do \
	$(INSTALL_DATA) $$f $(LIB_DIR); \
	done
	
perl_check :
	if test -f Rsynth/Makefile ; then cd Rsynth && make PERL=$(PERL) test ; fi  	
	
perlstuff : $(LIBRARY) Rsynth/Makefile.PL
	cd Rsynth && $(PERL) Makefile.PL && $(MAKE) PERL=$(PERL)
	
festival  :

install_perl : 		
	cd Rsynth && $(MAKE) PERL=$(PERL) install
	
install_festival : 	

phtoelm.def : Rsynth/phtoelm.def
	$(INSTALL_DATA) $< $@


# This bit of Makefile only works if GNU make is available
# The classic make version would have `$(SRCDIR)/getrev $(SRCDIR)`
# which does not work as it gets evaluated by 'sh' once we have cd'ed
# elsewhere. GNU make scheme uses := and $(shell...) to evaluate
# REVISION once at Makefile parse time
# It is really only needed by developer

REVISION  := $(shell $(SRCDIR)/getrev $(SRCDIR))

distribution: 
	rm -rf /tmp/rsynth-$(REVISION)
	mkdir /tmp/rsynth-$(REVISION)
	(cd $(SRCDIR); tar cf - `cat MANIFEST` ) | (cd /tmp/rsynth-$(REVISION); tar xf - )
	(cd /tmp; tar cvf - rsynth-$(REVISION) ) | gzip -c > $(SRCDIR)/../rsynth-$(REVISION).tar.gz

disttest : 
	rm -rf /tmp/rsynth-$(REVISION)
	mkdir /tmp/rsynth-$(REVISION)
	(cd $(SRCDIR); tar cf - `cat MANIFEST` ) | (cd /tmp/rsynth-$(REVISION); tar xf - )
	(cd /tmp/rsynth-$(REVISION); CC=$(CC) configure ; make CC=$(CC) -k check)


#MM Fri Jan  2 11:28:16 2004
aufile.o: aufile.c config.h useconfig.h getargs.h l2u.h hplay.h aufile.h
charset.o: charset.c charset.h
darray.o: darray.c config.h useconfig.h darray.h
deutsch.o: deutsch.c config.h darray.h lang.h say.h deutsch.h
dict.o: dict.c config.h charset.h phones.h phones.def useconfig.h dbif.h \
  lang.h darray.h dict.h
dlookup.o: dlookup.c config.h useconfig.h dict.h lang.h darray.h phones.h \
  phones.def getargs.h english.h
elements.o: elements.c config.h rsynth.h darray.h trie.h phfeat.h \
  Elements.def
english.o: english.c config.h darray.h lang.h say.h charset.h english.h \
  phones.h phones.def
getarg.o: getarg.c config.h useconfig.h getargs.h
guess.o: guess.c darray.h charset.h text.h phones.h phones.def lang.h \
  english.h say.h
holmes.o: holmes.c config.h useconfig.h rsynth.h darray.h trie.h \
  getargs.h plotparm.h hplay.h
hplay.o: hplay.c config.h useconfig.h l2u.h getargs.h hplay.h
l2u.o: l2u.c
mkdictdb.o: mkdictdb.c config.h useconfig.h trie.h darray.h phones.h \
  phones.def charset.h dbif.h
opsynth.o: opsynth.c config.h useconfig.h rsynth.h darray.h trie.h \
  plotparm.h
phones.o: phones.c config.h phones.h phones.def
phtoelm.o: phtoelm.c config.h useconfig.h rsynth.h darray.h trie.h \
  hplay.h phfeat.h phtoelm.def
Revision.o: Revision.c
rulebuild.o: rulebuild.c config.h useconfig.h darray.h lang.h english.h \
  phones.h phones.def charset.h text.h say.h
say.o: say.c config.h useconfig.h darray.h rsynth.h trie.h hplay.h dict.h \
  lang.h text.h getargs.h phones.h phones.def aufile.h charset.h \
  english.h deutsch.h say.h
saymain.o: saymain.c config.h useconfig.h darray.h rsynth.h trie.h \
  hplay.h dict.h lang.h text.h getargs.h phones.h phones.def aufile.h \
  charset.h english.h deutsch.h say.h
text.o: text.c config.h useconfig.h darray.h phtoelm.h lang.h text.h \
  say.h
trie.o: trie.c config.h useconfig.h trie.h
