CXXFLAGS=-std=c++11 -Wall -Wextra -Weffc++ -shared -fPIC -g
CXX=g++

libmysqlcpp.so: mysql.o mysql.h prepared_statement.o mysql_exception.o mysql_exception.h
	$(CXX) $(CXXFLAGS) mysql.o mysql_exception.o prepared_statement.o -lmysqlclient -o libmysqlcpp.so

.PHONY: clean
clean:
	rm -f *.o
	rm -f examples
