# Directories
TASK1_DIR = task1_src
TASK2_DIR = task2_src
SPARK_DIR = spark_src
TEST_DIR = test_driver
INCLUDE_DIR = include
# Java and Spark paths
JAVA_HOME = /usr/lib/jvm/java-17-openjdk
SPARK_HOME = /usr/share/apache-spark
# Java options for Spark and JVM
JAVA_OPTS = -Xmx4g \
 --add-opens=java.base/sun.nio.ch=ALL-UNNAMED \
 -Dlog4j.configuration=file:spark-matcher/src/main/resources/log4j.properties \
 -Dspark.master=local[*] \
 -verbose:class \
 -XX:+PrintClassHistogram
# Source files
TASK1_SRC = $(TASK1_DIR)/Task1.cpp
TASK1_OBJ = $(TASK1_SRC:.cpp=.o)
# Task2 source files
TASK2_SRC = $(wildcard $(TASK2_DIR)/*.cpp)
TASK2_OBJ = $(TASK2_SRC:.cpp=.o)
SPARK_SRC = $(wildcard $(SPARK_DIR)/*.cpp)
SPARK_OBJ = $(SPARK_SRC:.cpp=.o)
TEST_OBJ = $(TEST_DIR)/test.o
SPEED_TEST_OBJ = $(TEST_DIR)/speed_test.o
SPARK_TEST_OBJ = $(TEST_DIR)/spark_test.o
# Compiler flags
CC = gcc
CXX = g++
JNI_INCLUDES = -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux
SPARK_INCLUDES = -I$(SPARK_HOME)/include
CFLAGS = -O3 -fPIC -Wall -g -I$(INCLUDE_DIR) -I. $(JNI_INCLUDES) $(SPARK_INCLUDES)
CXXFLAGS = $(CFLAGS) -std=c++17
LDFLAGS = -lpthread -L$(JAVA_HOME)/lib/server -ljvm -lstdc++
# Add runtime path for shared libraries
RPATH = -Wl,-rpath,'$$ORIGIN'
# The programs that will be built
PROGRAMS = task1driver task2driver spark_testdriver speed_test
# The libraries that will be built
TASK1_LIB = Task1
TASK2_LIB = Task2
SPARK_LIB = sparkTask1
# Build all programs
all: $(PROGRAMS)
# Task1 library
lib$(TASK1_LIB).so: $(TASK1_OBJ)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^
# Task2 library
lib$(TASK2_LIB).so: $(TASK2_OBJ)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^
# Spark library
lib$(SPARK_LIB).so: $(SPARK_OBJ)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(LDFLAGS)
# Task1 test driver
task1driver: lib$(TASK1_LIB).so $(TEST_OBJ)
	$(CXX) $(CXXFLAGS) $(RPATH) -o $@ $(TEST_OBJ) -L. -l$(TASK1_LIB)
# Task2 test driver
task2driver: lib$(TASK2_LIB).so $(TEST_OBJ)
	$(CXX) $(CXXFLAGS) $(RPATH) -o $@ $(TEST_OBJ) -L. -l$(TASK2_LIB)
# Spark test driver
spark_testdriver: lib$(SPARK_LIB).so $(TEST_OBJ)
	$(CXX) $(CXXFLAGS) $(RPATH) -o $@ $(TEST_OBJ) -L. -l$(SPARK_LIB) $(LDFLAGS)
# Speed comparison test
speed_test: lib$(TASK1_LIB).so lib$(SPARK_LIB).so $(SPEED_TEST_OBJ)
	$(CXX) $(CXXFLAGS) $(RPATH) -o $@ $(SPEED_TEST_OBJ) -L. -l$(TASK1_LIB) -l$(SPARK_LIB) $(LDFLAGS)
# Build individual source files into objects
$(TASK1_DIR)/%.o: $(TASK1_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(TASK2_DIR)/%.o: $(TASK2_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(SPARK_DIR)/%.o: $(SPARK_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(TEST_DIR)/%.o: $(TEST_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
# Clean target
clean:
	rm -f $(PROGRAMS) lib$(TASK1_LIB).so lib$(TASK2_LIB).so lib$(SPARK_LIB).so
	find . -name '*.o' -print | xargs rm -f
# Phony targets
.PHONY: all clean