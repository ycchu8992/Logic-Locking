
.PHONY: all
all: sll

sll: 
	g++ strongLogicLocking.cpp -o sll.out -std=c++11 

.PHONY: runs
runs:
	./sll.out c17.bench 
	./sll.out c432.bench 
	./sll.out c3540.bench 
.PHONY: runr
runr:
	./rll.out c17.bench 
	./rll.out c432.bench 
	./rll.out c3540.bench 

sld: c17_sll c432_sll c3540_sll

.PHONY: c3540_sll
c3540_sll: 
	-sld -N 1 -c10 locked_c3540.bench c3540.bench 

.PHONY: c432_sll
c432_sll: 
	-sld -N 1 -c10 locked_c432.bench c432.bench 

.PHONY: c17_sll
c17_sll: 
	-sld -N 1 -c10 locked_c17.bench c17.bench 

.PHONY: lcmp
lcmp: 
	lcmp c3540.bench locked_c3540.bench key=11101010010001001011111010100100010010111110101001000100101111101010010001001011010111101000101010010100010100100101000010010101

.PHONY: source
source:
	source /home/HardwareSecurity112/tools/HS_Final_Project/env.cshrc 
clean:
	rm sll.out locked_c*.bench 
