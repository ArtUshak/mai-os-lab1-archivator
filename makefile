CC=clang
CFLAGS=-c -std=gnu99 -Wall -Wextra -Wnull-dereference --pedantic -O3 -Os -I./include
LDFLAGS=
CPPCHECKFLAGS=-I./include -I/usr/local/include -I/usr/lib/clang/9.0.1/include -I/usr/include --enable=all --force --suppress=missingIncludeSystem
SOURCE_DIR=src
SOURCES=$(SOURCE_DIR)/main.c $(SOURCE_DIR)/listdir.c $(SOURCE_DIR)/util.c $(SOURCE_DIR)/archive.c $(SOURCE_DIR)/file_wrapper.c
OBJ_DIR=obj/release
OBJECTS=$(patsubst $(SOURCE_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))
BIN_DIR=bin/release
EXECUTABLE=$(BIN_DIR)/main

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

runpack: $(EXECUTABLE)
	$(EXECUTABLE) pack . ../output.anchorfield

runlist: $(EXECUTABLE)
	$(EXECUTABLE) list ../output.anchorfield

rununpack: $(EXECUTABLE)
	rm -rf ../output/
	mkdir -p ../output/
	$(EXECUTABLE) unpack ../output.anchorfield ../output
