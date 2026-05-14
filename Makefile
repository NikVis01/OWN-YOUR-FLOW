CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

HOMEBREW_PREFIX := $(shell brew --prefix 2>/dev/null || echo /opt/homebrew)
INCLUDES = -I. -I./src -I$(HOMEBREW_PREFIX)/include
LDFLAGS = -L$(HOMEBREW_PREFIX)/lib -L/usr/local/lib \
          -Wl,-rpath,$(HOMEBREW_PREFIX)/lib -Wl,-rpath,/usr/local/lib
LIBS = -lredis++ -lcurl -lyaml-cpp -lfmt -lpthread -lhiredis

# Directories
SRC_DIR = src
BUILD_DIR = build
DB_DIR = $(SRC_DIR)/db
TOOLS_DIR = $(SRC_DIR)/tools

# Source files (only existing ones)
MAIN_SRC = $(SRC_DIR)/main.cpp
COT_SRC = $(SRC_DIR)/cot.cpp
PARSER_SRC = $(SRC_DIR)/parser.cpp

# Object files (in build directory)
MAIN_OBJ = $(BUILD_DIR)/main.o
COT_OBJ = $(BUILD_DIR)/cot.o
PARSER_OBJ = $(BUILD_DIR)/parser.o

# Target executable
TARGET = server

# Default target
all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(MAIN_OBJ) $(COT_OBJ) $(PARSER_OBJ)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $^ $(LIBS)

# Compile rules
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Database setup
setup-db:
	cd $(DB_DIR) && docker-compose up -d

# Clean
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean setup-db 