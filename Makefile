CXXFLAGS=-std=c++0x -Wall -Wextra -Weffc++ -g
CXX=g++
STATICFLAGS=$(CXXFLAGS) -c -fPIC
SHAREDFLAGS=$(CXXFLAGS) -shared

all: examples

examples.o: examples.cpp MySql.hpp MySqlException.hpp Binder.hpp

MySql.o: MySql.cpp MySql.hpp Binder.hpp
	$(CXX) $(STATICFLAGS) MySql.cpp -o MySql.o

MySqlException.o: MySqlException.cpp MySqlException.hpp
	$(CXX) $(STATICFLAGS) MySqlException.cpp -o MySqlException.o

libmysqlcpp.so: MySql.o MySql.hpp MySqlException.o MySqlException.hpp Binder.hpp
	$(CXX) $(SHAREDFLAGS) -W1,-soname,libmysqlcpp.so -o libmysqlcpp.so MySql.o MySqlException.o

examples: examples.o libmysqlcpp.so
	$(CXX) $(CXXFLAGS) examples.o libmysqlcpp.so -lmysqlclient -o examples

.PHONY: clean
clean:
	rm -f *.o
	rm -f libmysqlcpp.so
	rm -f examples
