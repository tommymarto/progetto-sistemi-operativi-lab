
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
PROJECTS	= common server api client
BIN_DIR_PROJECTS	= $(addprefix ./$(BIN_DIR)/, $(PROJECTS))

export

TARGETS := all clean

.PHONY: $(TARGETS) $(PROJECTS) $(BIN_DIR) test1 test2 test3


all: $(PROJECTS) | $(BIN_DIR_PROJECTS)
	$(foreach project, $(PROJECTS), cp -r ./$(SRC_PREFIX)/$(project)/bin/. ./bin/$(project)/bin;)

clean: $(PROJECTS)
	rm -rf $(BIN_DIR)
	rm -rf destFiles

$(PROJECTS):
	$(MAKE) -C ./$(SRC_DIR)/$@ $(MAKECMDGOALS)

$(BIN_DIR_PROJECTS):
	mkdir -p $@

test1:
	$(MAKE) all
	$(MAKE) -C ./$(SRC_DIR)/server $@ $(MAKECMDGOALS)
	cp -r ./$(SRC_PREFIX)/server/bin/. ./bin/server/bin
	./test1.sh

test2:
	$(MAKE) all
	$(MAKE) -C ./$(SRC_DIR)/server $@ $(MAKECMDGOALS)
	cp -r ./$(SRC_PREFIX)/server/bin/. ./bin/server/bin

test3:
	$(MAKE) all
	$(MAKE) -C ./$(SRC_DIR)/server $@ $(MAKECMDGOALS)
	cp -r ./$(SRC_PREFIX)/server/bin/. ./bin/server/bin
	./test3.sh