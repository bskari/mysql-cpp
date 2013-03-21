#include "testOutputBinder.hpp"
#include "../OutputBinder.hpp"

#include <cstdint>
#include <boost/test/unit_test.hpp>
#include <mysql/mysql.h>
#include <string>
#include <vector>

using std::string;
using std::vector;


void testSetResult()
{
    MYSQL_BIND bind;
    my_bool nullFlag = false;

    // Put each test in {} to create a local scope so that I don't have to do
    // any weird name mangling
#ifndef TYPE_TEST
#define TYPE_TEST(type) \
    { \
    type result = rand(); \
    bind.buffer = &result; \
    bind.is_null = &nullFlag; \
    type output; \
    OutputBinderResultSetter<type> setter; \
    setter.setResult(&output, bind); \
    BOOST_CHECK(result == output); \
    }
#endif

    TYPE_TEST(int)
    TYPE_TEST(float)
    TYPE_TEST(double)
    TYPE_TEST(char)
    TYPE_TEST(int8_t)
    TYPE_TEST(uint8_t)
    TYPE_TEST(int16_t)
    TYPE_TEST(uint16_t)
    TYPE_TEST(int32_t)
    TYPE_TEST(uint32_t)
    TYPE_TEST(int64_t)
    TYPE_TEST(uint64_t)

    {
    string result(" ", 5);
    vector<char> buffer(result.size());
    for (size_t i = 0; i < result.size(); ++i)
    {
        // Let's avoid \0
        buffer.at(i) = result.at(i) = (char)(rand() % 10 + 'a');
    }
    bind.buffer = &buffer.at(0);
    bind.is_null = &nullFlag;
    string output;
    OutputBinderResultSetter<string> setter;
    setter.setResult(&output, bind);
    BOOST_CHECK(result.size() == output.size());
    BOOST_CHECK(result == output);
    }
}


void testSetParameter()
{
}
