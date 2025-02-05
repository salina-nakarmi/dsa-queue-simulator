CC = gcc					#Defines the compile (gcc)
CFLAGS = -I./include -Wall	# compilation flags (-I includes the "include" directory, -wall enables warnings)
LDFLAGS = -lSDL2 -lSDL2_ttf	# Links the SDL2 and SDL2_ttf libraries

# Output directory 
BUILD_DIR = build

# Source files
GENERATOR_SRC = src/traffic_generator.c
SIMULATOR_SRC = src/simulator.c

# Object files(compiled files)
GENERATOR_OBJ = $(BUILD_DIR)/traffic_generator.o
SIMULATOR_OBJ = $(BUILD_DIR)/simulator.o

# Executables
GENERATOR = $(BUILD_DIR)/generator
SIMULATOR = $(BUILD_DIR)/simulator

#all targets default build(when we run make it ensure build dir exits)
all: $(BUILD_DIR) $(GENERATOR) $(SIMULATOR)

#creating 
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

#rules for creating executables
$(GENERATOR): $(GENERATOR_OBJ)
	$(CC) $(GENERATOR_OBJ) -o $@ $(LDFLAGS)

$(SIMULATOR): $(SIMULATOR_OBJ)
	$(CC) $(SIMULATOR_OBJ) -o $@ $(LDFLAGS)

#compiling .c to .o
$(BUILD_DIR)/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

#make clean removes compiled files
clean:
	rm -rf $(BUILD_DIR)

#Marks all and clean as special targets (not actual files).
.PHONY: all clean

run: $(SIMULATOR)
	./$(SIMULATOR)
