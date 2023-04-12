cc=g++
CFLAG=-std=c++17
LFLAG=-ljsoncpp -lpthread -lboost_system -lboost_filesystem -lzmq
compile_server:compile_server.cc
	$(cc) $< -o $@  $(CFLAG)  $(LFLAG) -g  

.PHONY:clean
clean:	
	rm -f  compile_server ./temp/*.stderr ./temp/*.stdin ./temp/*.stdout ./temp/*.out ./temp/*.compiler_error ./temp/*.cpp
