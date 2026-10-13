# Makefile for Shortest Path Finder
# Supports Linux, macOS, and Windows (MinGW/Clang/GCC)

CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -Ibackend/include
LDFLAGS ?= 

# Windows socket library
ifeq ($(OS),Windows_NT)
    LDFLAGS += -lws2_32
    TARGET = pathfinder.exe
    TEST_TARGET = test_pathfinder.exe
    RM = del /Q /F
else
    TARGET = pathfinder
    TEST_TARGET = test_pathfinder
    RM = rm -f
endif

SRCS = backend/src/graph.c \
       backend/src/minheap.c \
       backend/src/dijkstra.c \
       backend/src/storage.c \
       backend/src/server.c \
       backend/src/main.c

TEST_SRCS = backend/src/graph.c \
            backend/src/minheap.c \
            backend/src/dijkstra.c \
            backend/src/storage.c \
            backend/tests/test_pathfinder.c

.PHONY: all clean test run serve

all: $(TARGET) $(TEST_TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $(TARGET)

$(TEST_TARGET): $(TEST_SRCS)
	$(CC) $(CFLAGS) $(TEST_SRCS) -o $(TEST_TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

serve: $(TARGET)
	./$(TARGET) --serve 8080

clean:
	$(RM) $(TARGET) $(TEST_TARGET)
