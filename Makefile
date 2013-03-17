CXXFLAGS=-std=c++0x -Wall -Wextra -Weffc++ -g
CXX=g++
STATICFLAGS=$(CXXFLAGS) -c -fPIC
SHAREDFLAGS=$(CXXFLAGS) -shared

all: examples test

examples: examples.o libmysqlcpp.so
	$(CXX) $(CXXFLAGS) examples.o libmysqlcpp.so -lmysqlclient_r -o examples

test: tests/test.o tests/testInputBinder.o tests/testInputBinder.hpp
	$(CXX) $(CXXFLAGS) tests/test.o tests/testInputBinder.o -lboost_unit_test_framework -o test

examples.o: examples.cpp MySql.hpp MySqlException.hpp InputBinder.hpp OutputBinder.hpp

MySql.o: MySql.cpp MySql.hpp InputBinder.hpp OutputBinder.hpp MySqlException.o MySqlException.hpp
	$(CXX) $(STATICFLAGS) MySql.cpp -o MySql.o

MySqlException.o: MySqlException.cpp MySqlException.hpp
	$(CXX) $(STATICFLAGS) MySqlException.cpp -o MySqlException.o

libmysqlcpp.so: MySql.o MySql.hpp MySqlException.o MySqlException.hpp InputBinder.hpp OutputBinder.hpp
	$(CXX) $(SHAREDFLAGS) -W1,-soname,libmysqlcpp.so -o libmysqlcpp.so MySql.o MySqlException.o


.PHONY: clean
clean:
	rm -f *.o
	rm -f libmysqlcpp.so
	rm -f examples
	rm -f tests/*.o
	rm -f test
