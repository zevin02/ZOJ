cc=g++
CFLAG=-std=c++11
LFLAG=-ljsoncpp -lpthread -lmysqlclient -lboost_system -lboost_filesystem -lctemplate 

oj_server:oj_server.cc
	$(cc) $< -o $@  $(CFLAG)  $(LFLAG) -g  

.PHONY:clean
clean:	
	rm oj_server