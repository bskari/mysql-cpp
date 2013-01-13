CXXFLAGS=-std=c++0x -Wall -Wextra -Weffc++ -g
CXX=g++
STATICFLAGS=$(CXXFLAGS) -c -fPIC
SHAREDFLAGS=$(CXXFLAGS) -shared

all: examples

examples.o: examples.cpp MySql.hpp MySqlException.hpp

MySql.o: MySql.cpp MySql.hpp
	$(CXX) $(STATICFLAGS) MySql.cpp -o MySql.o

MySqlException.o: MySqlException.cpp MySqlException.hpp
	$(CXX) $(STATICFLAGS) MySqlException.cpp -o MySqlException.o

PreparedStatement.o: PreparedStatement.cpp MySql.hpp PreparedStatement.hpp
	$(CXX) $(STATICFLAGS) PreparedStatement.cpp -o PreparedStatement.o

libmysqlcpp.so: MySql.o MySql.hpp PreparedStatement.o PreparedStatement.hpp MySqlException.o MySqlException.hpp
	$(CXX) $(SHAREDFLAGS) -W1,-soname,libmysqlcpp.so -o libmysqlcpp.so MySql.o PreparedStatement.o MySqlException.o

examples: examples.o libmysqlcpp.so
	$(CXX) $(CXXFLAGS) examples.o libmysqlcpp.so -lmysqlclient -o examples

.PHONY: clean
clean:
	rm -f *.o
	rm -f libmysqlcpp.so
	rm -f examples
