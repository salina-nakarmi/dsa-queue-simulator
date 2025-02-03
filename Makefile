CC = gcc
CFLAGS = -I./include -Wall
LDFLAGS = -lSDL2

# Output directory
BUILD_DIR = build

# Source files
GENERATOR_SRC = src/traffic_generator.c
SIMULATOR_SRC = src/simulator.c

# Object files
GENERATOR_OBJ = $(BUILD_DIR)/traffic_generator.o
SIMULATOR_OBJ = $(BUILD_DIR)/simulator.o

# Executables
GENERATOR = $(BUILD_DIR)/generator
SIMULATOR = $(BUILD_DIR)/simulator

all: $(BUILD_DIR) $(GENERATOR) $(SIMULATOR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(GENERATOR): $(GENERATOR_OBJ)
	$(CC) $(GENERATOR_OBJ) -o $@ $(LDFLAGS)

$(SIMULATOR): $(SIMULATOR_OBJ)
	$(CC) $(SIMULATOR_OBJ) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean