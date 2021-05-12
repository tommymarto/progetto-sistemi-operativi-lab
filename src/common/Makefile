EXE := $(BIN_DIR)/libcommon.so
SRC := $(wildcard ./$(SRC_DIR)/*.c)
OBJ := $(SRC:./$(SRC_DIR)/%.c=./$(OBJ_DIR)/%.o)

LDFLAGS += -shared

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -fPIC $(INCLUDES) $(OPTFLAGS) -c -o $@ $<

clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR)

$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@