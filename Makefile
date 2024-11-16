# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -std=c11 -O3 #-fopenmp
LDFLAGS := 
LIBS := -lGL -lGLEW -lglfw

# Source and output
SRC := full_ogl_single.c
OBJ := full_ogl_single.o
TARGET := full_ogl_single

# Detect platform
UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
    # Linux specific settings
    LIBS := -lGL -lGLEW -lglfw -lrt -lm
    CFLAGS += -D_POSIX_C_SOURCE=199309L
endif

ifeq ($(UNAME), Darwin)
    # macOS specific settings
    CFLAGS += -I/opt/homebrew/include -I/opt/homebrew/opt/llvm/include \
#    -isysroot $(shell xcrun --show-sdk-path) \
#    -F$(shell xcrun --show-sdk-path)/System/Library/Frameworks
    LDFLAGS += -L/opt/homebrew/lib -L/opt/homebrew/opt/llvm/lib
    LIBS := -framework OpenGL -framework System -lGLEW -lglfw
#    CC := /opt/homebrew/opt/llvm/bin/clang
endif

# For Windows, use MinGW or a similar environment
ifeq ($(OS), Windows_NT)
    LIBS := -lopengl32 -lglew32 -lglfw3
endif

# Rules
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean

