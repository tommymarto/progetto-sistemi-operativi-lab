
CC			=  gcc
CFLAGS	    += -std=c11 -Wall -Werror -pedantic-errors -Wno-unused-function
INCLUDES	= -I ../common/include -I ../api/include -I ./include
LDFLAGS 	=
OPTFLAGS	= -g
MAKEFLAGS 	+= -s

API_LIB = -L../api/bin -Wl,-rpath=$(PWD)/bin/api/bin -lapi
COMMON_LIB = -L../common/bin -Wl,-rpath=$(PWD)/bin/common/bin -lcommon

# folder structure
SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

# aggiungere qui altri targets
SRC_PREFIX	= src
PROJECTS	= server api client common
BIN_DIR_PROJECTS	= $(addprefix ./$(BIN_DIR)/, $(PROJECTS))

export

TARGETS := all clean

.PHONY: $(TARGETS) $(PROJECTS) $(BIN_DIR) run-client run-server valgrind-client valgrind-server test1 test2 test3


all: $(PROJECTS) | $(BIN_DIR_PROJECTS)
	$(foreach project, $(PROJECTS), cp -r ./$(SRC_PREFIX)/$(project)/bin/. ./bin/$(project)/bin;)

clean: $(PROJECTS)
	rm -rf $(BIN_DIR)

$(PROJECTS):
	$(MAKE) -C ./$(SRC_DIR)/$@ $(MAKECMDGOALS)

run-client:
	./bin/client/bin/client

run-server:
	./bin/server/bin/server

valgrind-client:
	valgrind --leak-check=full ./bin/client/bin/client

valgrind-server:
	valgrind --leak-check=full ./bin/server/bin/server	

$(BIN_DIR_PROJECTS):
	mkdir -p $@

test1:
	echo "test1"

test2:
	echo "test2"

test3:
	echo "test3"