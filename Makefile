CC = gcc
CFLAGS = -I/usr/include/SDL2 -I./include -Wall -Wextra
LDFLAGS = -lSDL2 -lSDL2_ttf -pthread

BUILD_DIR = build
SRC_DIR = src

# Source files
SIMULATOR_SRCS = $(SRC_DIR)/simulator.c $(SRC_DIR)/vehicle_queue.c
GENERATOR_SRCS = $(SRC_DIR)/traffic_generator.c

# Object files
SIMULATOR_OBJS = $(SIMULATOR_SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
GENERATOR_OBJS = $(GENERATOR_SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean run

all: $(BUILD_DIR) $(BUILD_DIR)/simulator $(BUILD_DIR)/generator

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/simulator: $(SIMULATOR_OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/generator: $(GENERATOR_OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

# Run both simulator and generator
run: all
	./$(BUILD_DIR)/simulator & sleep 2 && ./$(BUILD_DIR)/generator