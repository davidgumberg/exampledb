# Compiler
CXX = clang++

# Source files
SRCS = main.cpp kv.cpp mdbx.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Libraries
LIBS = -lcrypto
LIBS += -lmdbx

# Compiler flags
DEBUG_FLAGS = -g -DDEBUG
RELEASE_FLAGS = -Wall -Wextra -O2

ifeq ($(DEBUG),1)
  CXXFLAGS := $(DEBUG_FLAGS) $(CXXFLAGS)
else
  CXXFLAGS := $(RELEASE_FLAGS) $(CXXFLAGS)
endif

# Target executable
TARGET = db

# Default rule
all: $(TARGET)

# Rule to link object files into the final executable
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LIBS) -o $@

# Rule to compile source files into object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule to remove generated files
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
