
CC			=  gcc
CFLAGS	    += -std=c11 -Wall -Werror -pedantic
INCLUDES	= -I ./common/src
LDFLAGS 	= 
LIBS		= 
OPTFLAGS	= -g

# folder structure
SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

# aggiungere qui altri targets
SRC_PREFIX 		= src
PROJECTS_LIST	= server client common
PROJECTS		= $(addprefix ./$(SRC_PREFIX)/, $(PROJECTS_LIST))

export

.PHONY: all clean subdir-make subdir-clean
.SUFFIXES: .c .h

all: subdir-make | $(BIN_DIR)
	$(foreach subdir, $(PROJECTS_LIST), cp -r ./$(SRC_PREFIX)/$(subdir)/bin ./bin/$(subdir);)

$(BIN_DIR): 
	mkdir -p $@

subdir-make:
	$(foreach subdir, $(PROJECTS), $(MAKE) -C $(subdir);)

clean: subdir-clean
	rm -rf bin

subdir-clean:
	$(foreach subdir, $(PROJECTS), $(MAKE) -C $(subdir) clean;)


# cleanall: clean
# 	-rm -f *.o *~ mat_dump.txt mat_dump.dat



