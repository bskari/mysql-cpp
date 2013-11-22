CXX_STANDARD=c++11
CXX=g++
# If you want to change the CXX_STANDARD or CXX, put them in Makefile.custom.
# For example, GCC 4.4.6 only recognizes -std=c++0x, so I changed CXX_STANDARD
# to that in my Makefile.custom.
-include Makefile.custom
WARNING_CXXFLAGS = -Wall -Wextra -Weffc++ -Wfloat-equal -Wshadow\
	-Wpointer-arith -Wcast-align -Wstrict-overflow=5\
	-Wwrite-strings -Wswitch-default -Wswitch-enum -Wparentheses\
	-Woverloaded-virtual -Wconversion -pedantic
CXXFLAGS=-std=$(CXX_STANDARD) $(WARNING_CXXFLAGS) -g --coverage
STATICFLAGS=$(CXXFLAGS) -c -fPIC
SHAREDFLAGS=$(CXXFLAGS) -shared

all: examples test

examples: examples.o libmysqlcpp.so
	$(CXX) $(CXXFLAGS) examples.o libmysqlcpp.so -lmysqlclient_r -o examples

examples.o: examples.cpp MySql.hpp MySqlException.hpp InputBinder.hpp \
	OutputBinder.hpp

MySql.o: MySql.cpp MySql.hpp InputBinder.hpp OutputBinder.hpp \
	MySqlException.o MySqlException.hpp MySqlPreparedStatement.hpp
	$(CXX) $(CXXFLAGS) $(STATICFLAGS) MySql.cpp -o MySql.o

MySqlException.o: MySqlException.cpp MySqlException.hpp
	$(CXX) $(CXXFLAGS) $(STATICFLAGS) MySqlException.cpp -o MySqlException.o

MySqlPreparedStatement.o: MySqlPreparedStatement.cpp MySqlPreparedStatement.hpp
	$(CXX) $(CXXFLAGS) $(STATICFLAGS) MySqlPreparedStatement.cpp \
		-o MySqlPreparedStatement.o

OutputBinder.o: OutputBinder.hpp OutputBinder.cpp MySqlPreparedStatement.hpp
	$(CXX) $(CXXFLAGS) $(STATICFLAGS) OutputBinder.cpp -o OutputBinder.o

libmysqlcpp.so: MySql.o MySql.hpp MySqlException.o MySqlException.hpp \
	MySqlPreparedStatement.o InputBinder.hpp OutputBinder.o OutputBinder.hpp
	$(CXX) $(CXXFLAGS) $(SHAREDFLAGS) -Wl,-soname,libmysqlcpp.so \
		MySql.o MySqlException.o MySqlPreparedStatement.o OutputBinder.o \
		-o libmysqlcpp.so

test: tests/test.o tests/testInputBinder.o tests/testInputBinder.hpp \
	tests/testOutputBinder.o tests/testOutputBinder.hpp \
	tests/testMySql.hpp tests/testMySql.o MySqlException.o MySql.o \
	MySqlPreparedStatement.o OutputBinder.o
	$(CXX) $(CXXFLAGS) tests/test.o tests/testInputBinder.o \
		tests/testOutputBinder.o tests/testMySql.o MySqlException.o MySql.o \
		MySqlPreparedStatement.o OutputBinder.o \
		-lboost_unit_test_framework -lmysqlclient_r -o test

tests/testInputBinder.o: tests/testInputBinder.cpp tests/testInputBinder.hpp \
	InputBinder.hpp

tests/testOutputBinder.o: tests/testOutputBinder.cpp \
	tests/testOutputBinder.hpp OutputBinder.hpp

tests/testMySql.o: tests/testMySql.cpp tests/testMySql.hpp MySql.hpp \
	MySqlPreparedStatement.hpp

.PHONY: clean
clean: clean-coverage
	rm -f *.o tests/*.o
	rm -f libmysqlcpp.so
	rm -f examples
	rm -f test

.PHONY: clean-coverage
clean-coverage:
	rm -f *.gcno *.gcov *.gcna *.gcda tests/*.gcno tests/*.gcov tests/*.gcna tests/*.gcda
	rm -f coverage.info
	rm -rf coverage

.PHONY: coverage
coverage:
	lcov --capture --directory . --base-directory . --output-file coverage.info
	genhtml coverage.info --output-directory coverage
