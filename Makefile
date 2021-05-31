
CC			=  gcc
CFLAGS	    += -std=c11 -D_GNU_SOURCE -Wall -Werror -pedantic-errors -Wno-unused-function -Wno-unused-result
INCLUDES	= -I ../common/include -I ../api/include -I ./include
LDFLAGS 	=
OPTFLAGS	= -O3
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

.PHONY: $(TARGETS) $(PROJECTS) $(BIN_DIR) cleanTestOutput test1 test2 test3


all: $(PROJECTS) | $(BIN_DIR_PROJECTS)
	$(foreach project, $(PROJECTS), cp -r ./$(SRC_PREFIX)/$(project)/bin/. ./bin/$(project)/bin;)

clean: $(PROJECTS) cleanTestOutput
	rm -rf $(BIN_DIR)
	rm -rf testFiles/test/bin
	rm -rf testFiles/test/src

$(PROJECTS):
	$(MAKE) -C ./$(SRC_DIR)/$@ $(MAKECMDGOALS)

$(BIN_DIR_PROJECTS):
	mkdir -p $@

cleanTestOutput:
	rm -rf destDir*
	rm -f serverLog.txt

test1: cleanTestOutput
	$(MAKE) all
	$(MAKE) -C ./$(SRC_DIR)/server $@ $(MAKECMDGOALS)
	cp -r ./$(SRC_PREFIX)/server/bin/. ./bin/server/bin
	cp -r ./bin ./testFiles/test
    
	valgrind --leak-check=full ./bin/server/bin/server & \
	./test1.sh; \
	kill -1 $$!; \
	wait;

test2: cleanTestOutput
	$(MAKE) all
	$(MAKE) -C ./$(SRC_DIR)/server $@ $(MAKECMDGOALS)
	cp -r ./$(SRC_PREFIX)/server/bin/. ./bin/server/bin
	cp -r ./bin ./testFiles/test

	@./bin/server/bin/server & \
    ./test2.sh; \
    kill -1 $$!; \
    wait

test3: cleanTestOutput
	$(MAKE) all
	$(MAKE) -C ./$(SRC_DIR)/server $@ $(MAKECMDGOALS)
	cp -r ./$(SRC_PREFIX)/server/bin/. ./bin/server/bin
	cp -r ./bin ./testFiles/test
	cp -r ./src ./testFiles/test
	cp -r ./src ./testFiles/test/src

	@./bin/server/bin/server & \
	./test3.sh; \
	kill -2 $$!; \
	wait
