CXXFLAGS=-std=c++0x -g -Wall -Wextra -Weffc++
CXX=g++

all: examples

examples.o: examples.cpp MySql.hpp MySqlException.hpp

examples: examples.o MySql.o MySql.hpp PreparedStatement.o MySqlException.o MySqlException.hpp
	$(CXX) $(CXXFLAGS) examples.o MySql.o MySqlException.o PreparedStatement.o -lmysqlclient -o examples

.PHONY: clean
clean:
	rm -f *.o
	rm -f examples
