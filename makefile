BUILD_TARGET ?= debug

CC = clang
CFLAGS = -c -std=gnu99 -Wall -Wextra -Wnull-dereference --pedantic -I./include
LDFLAGS =
CPPCHECKFLAGS = --std=c99 -I./include -I/usr/local/include -I/usr/lib/clang/9.0.1/include -I/usr/include --force --suppress=missingIncludeSystem

ifeq ($(BUILD_TARGET),release)
CFLAGS += -O3 -Os
else
ifeq ($(BUILD_TARGET),debug)
CFLAGS += -ggdb -fsanitize=address,undefined,nullability,unsigned-integer-overflow
LDFLAGS += -fsanitize=address,undefined,nullability,unsigned-integer-overflow
else
$(error Invalid build target '$(BUILD_TARGET)')
endif
endif

SOURCE_DIR = src
SOURCES = $(SOURCE_DIR)/main.c $(SOURCE_DIR)/listdir.c $(SOURCE_DIR)/util.c $(SOURCE_DIR)/archive.c $(SOURCE_DIR)/file_wrapper.c $(SOURCE_DIR)/program_options.c
OBJ_DIR = obj/$(BUILD_TARGET)
OBJECTS = $(patsubst $(SOURCE_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))
BIN_DIR = bin/$(BUILD_TARGET)
EXECUTABLE = $(BIN_DIR)/anchorfield

build: $(EXECUTABLE)

clean:
	rm -rf $(OBJ_DIR)/*
	rm -f $(EXECUTABLE)

rebuild: clean build

cppcheck: $(SOURCES)
	cppcheck $(CPPCHECKFLAGS) $(SOURCES)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

$(OBJ_DIR)/%.o: $(SOURCE_DIR)/%.c
	$(CC) $(CFLAGS) $< -o $@

