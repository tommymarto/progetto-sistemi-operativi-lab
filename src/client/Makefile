EXE := $(BIN_DIR)/client
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

.PHONY: all

all: $(EXE)

$(EXE): $(OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -c -o $@ $<

$(OBJ_DIR):
	mkdir -p $@

$(BIN_DIR): 
	mkdir -p $@

clean:
	rm -rf bin obj