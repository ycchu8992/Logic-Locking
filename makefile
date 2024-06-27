# Define compiler
CXX = g++

# Define flags
CXXFLAGS = -Wall -g -std=c++11 -O3

# Define target executable
TARGET = fault_analysis

# Define source files
SRC = fault_analysis.cpp

# Define object files
OBJ = $(SRC:.cpp=.o)

# Default target
all: $(TARGET)

# Rule for building the target executable
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Rule for compiling source files into object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

# Clean target
clean:
	rm -f $(TARGET) $(OBJ)

.PHONY: all clean