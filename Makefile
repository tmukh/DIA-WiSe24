# Directories
IMPL_DIR = task2_src
TEST_DIR = test_driver
INCLUDE_DIRS = include \

# Source files
IMPL_SRC = $(wildcard $(IMPL_DIR)/*.cpp)
TEST_SRC = $(wildcard $(TEST_DIR)/*.cpp)

# Object files
IMPL_O = $(IMPL_SRC:.cpp=.o)
TEST_O = $(TEST_SRC:.cpp=.o)

# Compiler and flags
CC  = gcc
CXX = g++
INCLUDE_FLAGS = $(foreach dir,$(INCLUDE_DIRS),-I$(dir))
CFLAGS = -O0 -fPIC -Wall -g $(INCLUDE_FLAGS)
CXXFLAGS = $(CFLAGS)
LDFLAGS = -lpthread

# Add RPATH to link flags for the executable
RPATH = -Wl,-rpath,'$$ORIGIN'

# The programs that will be built
PROGRAMS = testdriver

# The name of the library that will be built
LIBRARY = core

# Default target
.DEFAULT_GOAL := all

# Build all programs
all: $(PROGRAMS)

# Pattern rule for object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Library target
lib: $(IMPL_O)
	$(CXX) $(CXXFLAGS) -shared -o lib$(LIBRARY).so $(IMPL_O)

# Test driver target
testdriver: lib $(TEST_O)
	$(CXX) $(CXXFLAGS) -o testdriver $(TEST_O) -L. -l$(LIBRARY) $(LDFLAGS) $(RPATH)

# Clean target
clean:
	rm -f $(PROGRAMS) lib$(LIBRARY).so
	find . -name '*.o' -print | xargs rm -f

# Show configuration
show-config:
	@echo "Implementation sources: $(IMPL_SRC)"
	@echo "Test sources: $(TEST_SRC)"
	@echo "Include paths: $(INCLUDE_DIRS)"

# Phony targets
.PHONY: all clean lib show-config

# Dependencies
-include $(IMPL_O:.o=.d)
-include $(TEST_O:.o=.d)