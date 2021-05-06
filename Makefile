
CC			=  gcc
CFLAGS	    += -std=c11 -Wall -Werror -pedantic
INCLUDES	= -I ./utils/includes
LDFLAGS 	= 
LIBS		= 
OPTFLAGS	= -g -O3 

# aggiungere qui altri targets
SUBDIRS			= $(wildcard */.)
SUBDIRSCLEAN 	= $(SUBDIRS)

.PHONY: all clean $(SUBDIRS) $(SUBDIRSCLEAN)
.SUFFIXES: .c .h

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

%: %.c
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $< $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -c -o $@ $<


clean: $(SUBDIRSCLEAN)

$(SUBDIRSCLEAN):
	@cd $@; $(MAKE) clean

cleanall: clean
	-rm -f *.o *~ mat_dump.txt mat_dump.dat



