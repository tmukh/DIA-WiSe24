# Directories
TASK1_DIR = task1_src
TASK2_DIR = task2_src
SPARK_DIR = spark_src
TEST_DIR = test_driver
INCLUDE_DIR = include

# Java paths
JAVA_HOME = /usr/lib/jvm/java-17-openjdk
JNI_INCLUDES = -I$(JAVA_HOME)/include \
               -I$(JAVA_HOME)/include/linux

# Source files
TASK1_SRC = $(TASK1_DIR)/Task1.cpp
TASK1_OBJ = $(TASK1_SRC:.cpp=.o)
SPARK_SRC = $(wildcard $(SPARK_DIR)/*.cpp)
SPARK_OBJ = $(SPARK_SRC:.cpp=.o)
TEST_OBJ = $(TEST_DIR)/test.o
SPEED_TEST_OBJ = $(TEST_DIR)/speed_test.o
SPARK_TEST_OBJ = $(TEST_DIR)/spark_test.o

# Compiler flags
CC = gcc
CXX = g++
CFLAGS = -O3 -fPIC -Wall -g -I$(INCLUDE_DIR) $(JNI_INCLUDES)
CXXFLAGS = $(CFLAGS) -std=c++17
LDFLAGS = -lpthread -L$(JAVA_HOME)/lib/server -ljvm

# The programs that will be built
PROGRAMS = task1driver spark_testdriver speed_test

# The libraries that will be built
TASK1_LIB = task1
SPARK_LIB = sparkTask1

# Build all programs
all: $(PROGRAMS)

# Task1 library
task1_lib: $(TASK1_OBJ)
	$(CXX) $(CXXFLAGS) -shared -o lib$(TASK1_LIB).so $^

# Spark library
spark_lib: $(SPARK_OBJ)
	$(CXX) $(CXXFLAGS) -shared -o lib$(SPARK_LIB).so $^ $(LDFLAGS)

# Task1 test driver
task1driver: task1_lib $(TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_OBJ) -L. -l$(TASK1_LIB)

# Spark test driver
spark_testdriver: spark_lib $(SPARK_TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(SPARK_TEST_OBJ) -L. -l$(SPARK_LIB) $(LDFLAGS)

# Speed comparison test
speed_test: task1_lib spark_lib $(SPEED_TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(SPEED_TEST_OBJ) -L. -l$(TASK1_LIB) -l$(SPARK_LIB) $(LDFLAGS)

# Build individual source files into objects
$(TASK1_DIR)/%.o: $(TASK1_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(SPARK_DIR)/%.o: $(SPARK_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TEST_DIR)/%.o: $(TEST_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean target
clean:
	rm -f $(PROGRAMS) lib$(TASK1_LIB).so lib$(SPARK_LIB).so
	find . -name '*.o' -print | xargs rm -f

# Phony targets
.PHONY: all task1_lib spark_lib clean