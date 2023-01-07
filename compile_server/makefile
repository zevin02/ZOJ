cc=g++
CFLAG=-std=c++11

compile_server:compile_server.cc
	$(cc) $< -o $@  $(CFLAG)  -g  

.PHONY:clean
clean:	
	rm -f  compile_server ./temp/*.stderr ./temp/*.stdin ./temp/*.stdout ./temp/*.out ./temp/*.compiler_error
