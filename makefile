# # Define compiler
# CXX = g++

# # Define flags
# CXXFLAGS = -Wall -g -std=c++11 -O3

# # Define target executable
# TARGET = fault_analysis

# # Define source files
# SRC = fault_analysis.cpp

# # Define object files
# OBJ = $(SRC:.cpp=.o)

# # Default target
# all: $(TARGET)

# # Rule for building the target executable
# $(TARGET): $(OBJ)
# 	$(CXX) $(CXXFLAGS) -o $@ $^

# # Rule for compiling source files into object files
# %.o: %.cpp
# 	$(CXX) $(CXXFLAGS) -c $<

# # lock
# lock_all: $(TARGET)
# 	./$(TARGET) c17
# 	./$(TARGET) c432
# 	./$(TARGET) c3540

# lock_c17: $(TARGET)
# 	./$(TARGET) c17

# lock_c432: $(TARGET)
# 	./$(TARGET) c432

# lock_c3540: $(TARGET)
# 	./$(TARGET) c3540

# # Clean target
# clean:
# 	rm -f $(TARGET) $(OBJ)

# .PHONY: all clean


.PHONY: all
all: fll

fll: 
	g++ fault_analysis.cpp -o fll.out -std=c++11 -O3

.PHONY: run
run: 
	./fll.out c17
	./fll.out c432 
	./fll.out c3540

sld: c17 c432 c3540

.PHONY: c3540
c3540: 
	-sld bench/fall_c3540.bench bench/c3540.bench 

.PHONY: c432
c432: 
	-sld bench/fall_c432.bench bench/c432.bench

.PHONY: c17
c17: 
	-sld bench/fall_c17.bench bench/c17.bench

.PHONY: lcmp
lcmp:
	lcmp bench/c3540.bench bench/fall_c3540.bench key=11101010010001001011111010100100010010111110101001000100101111101010010001001011010111101000101010010100010100100101000010010101
	lcmp bench/c432.bench bench/fall_c432.bench key=111010100100010010111110101001000100101111101010010001001011111010100100010010110101111010001010100101000101001001010000
	lcmp bench/c17.bench bench/fall_c17.bench key=111010