EXE := $(BIN_DIR)/server
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
LIBS += -lpthread

.PHONY: all clean test1 test2 test3

all: $(EXE)
	./generate_config.sh
	mv -f config.txt ./bin

$(EXE): $(OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS) $(COMMON_LIB)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -c -o $@ $<

test1: $(EXE)
	./generate_config.sh --max-files 10000 --max-memory 128 --workers-count 1
	mv -f config.txt ./bin

test2: $(EXE)
	./generate_config.sh --max-files 10 --max-memory 1 --workers-count 4
	mv -f config.txt ./bin

test3: $(EXE)
	./generate_config.sh --max-files 100 --max-memory 32 --workers-count 8
	mv -f config.txt ./bin

clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR)

$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@