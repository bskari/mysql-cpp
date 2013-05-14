#include <cstdint>
#include <mysql/mysql.h>

#include <boost/test/unit_test.hpp>
#include <memory>
#include <string>
#include <vector>

#include "testOutputBinder.hpp"
#include "../MySqlException.hpp"
#include "../OutputBinder.hpp"

using std::shared_ptr;
using std::string;
using std::vector;


void testSetResult() {
    unsigned int randSeed = 0;
    MYSQL_BIND bind;
    my_bool nullFlag = false;

    // Put each test in {} to create a local scope so that I don't have to do
    // any weird name mangling
#ifndef TYPE_TEST_SET_RESULT
#define TYPE_TEST_SET_RESULT(type) \
    { \
    type result = rand_r(&randSeed); \
    bind.buffer = &result; \
    bind.is_null = &nullFlag; \
    type output; \
    OutputBinderResultSetter<type> setter; \
    setter.setResult(&output, bind); \
    BOOST_CHECK(result == output); \
    }
#endif

    TYPE_TEST_SET_RESULT(int)    // NOLINT[readability/casting]
    TYPE_TEST_SET_RESULT(float)  // NOLINT[readability/casting]
    TYPE_TEST_SET_RESULT(double) // NOLINT[readability/casting]
    TYPE_TEST_SET_RESULT(char)   // NOLINT[readability/casting]
    TYPE_TEST_SET_RESULT(int8_t)
    TYPE_TEST_SET_RESULT(uint8_t)
    TYPE_TEST_SET_RESULT(int16_t)
    TYPE_TEST_SET_RESULT(uint16_t)
    TYPE_TEST_SET_RESULT(int32_t)
    TYPE_TEST_SET_RESULT(uint32_t)
    TYPE_TEST_SET_RESULT(int64_t)
    TYPE_TEST_SET_RESULT(uint64_t)

    // std::string test
    {  // NOLINT[whitespace/parens]
        string result(" ", 5);
        vector<char> buffer(result.size());
        for (size_t i = 0; i < result.size(); ++i) {
            // Let's avoid \0
            buffer.at(i) = result.at(i) = static_cast<char>(
                (rand_r(&randSeed) % 10 + 'a'));
        }
        buffer.push_back('\0');
        bind.buffer = &buffer.at(0);
        bind.is_null = &nullFlag;
        string output;
        OutputBinderResultSetter<string> setter;
        setter.setResult(&output, bind);
        BOOST_CHECK(result.size() == output.size());
        BOOST_CHECK(result == output);
    }
    return;

    // std::shared_ptr with NULL test
    {  // NOLINT[whitespace/parens]
        float output;
        shared_ptr<decltype(output)> ptr;
        nullFlag = true;
        bind.buffer = &output;
        bind.is_null = &nullFlag;
        OutputBinderResultSetter<decltype(output)> setter;
        setter.setResult(&output, bind);
        BOOST_CHECK(0 == ptr.get());
    }

    // std::shared_ptr with a value test
    {  // NOLINT[whitespace/parens]
        float output = rand_r(&randSeed);
        shared_ptr<decltype(output)> ptr;
        nullFlag = false;
        bind.buffer = &output;
        bind.is_null = &nullFlag;
        OutputBinderResultSetter<decltype(output)> setter;
        setter.setResult(&output, bind);
        BOOST_CHECK(0 != ptr.get());
        if (0 != ptr.get()) {
            BOOST_CHECK(output == *ptr);
        }
    }

    // Trying to set a NULL value with a non-std::shared_ptr parameter should
    // throw
    {  // NOLINT[whitespace/parens]
        float output = rand_r(&randSeed);
        nullFlag = true;
        bind.buffer = &output;
        bind.is_null = &nullFlag;
        OutputBinderResultSetter<decltype(output)> setter;
        BOOST_CHECK_THROW(setter.setResult(&output, bind), MySqlException);
    }
}


void testSetParameter() {
    MYSQL_BIND bind;
    my_bool nullFlag;

    // Put each test in {} to create a local scope so that I don't have to do
    // any weird name mangling
#ifndef TYPE_TEST_SET_PARAMETER
#define TYPE_TEST_SET_PARAMETER(type, mysqlType, isUnsigned) \
    { \
    vector<char> buffer; \
    OutputBinderParameterSetter<type> setter; \
    setter.setParameter(&bind, &buffer, &nullFlag); \
    BOOST_CHECK(sizeof(type) == buffer.size()); \
    BOOST_CHECK(bind.buffer == &buffer.at(0)); \
    BOOST_CHECK(static_cast<bool>(bind.is_unsigned) \
        == static_cast<bool>(isUnsigned)); \
    BOOST_CHECK(bind.is_null == &nullFlag); \
    }
#endif

    TYPE_TEST_SET_PARAMETER(float,    MYSQL_TYPE_FLOAT,    0)
    TYPE_TEST_SET_PARAMETER(double,   MYSQL_TYPE_DOUBLE,   0)
    TYPE_TEST_SET_PARAMETER(int8_t,   MYSQL_TYPE_TINY,     0)
    TYPE_TEST_SET_PARAMETER(uint8_t,  MYSQL_TYPE_TINY,     1)
    TYPE_TEST_SET_PARAMETER(int16_t,  MYSQL_TYPE_SHORT,    0)
    TYPE_TEST_SET_PARAMETER(uint16_t, MYSQL_TYPE_SHORT,    1)
    TYPE_TEST_SET_PARAMETER(int32_t,  MYSQL_TYPE_LONG,     0)
    TYPE_TEST_SET_PARAMETER(uint32_t, MYSQL_TYPE_LONG,     1)
    TYPE_TEST_SET_PARAMETER(int64_t,  MYSQL_TYPE_LONGLONG, 0)
    TYPE_TEST_SET_PARAMETER(uint64_t, MYSQL_TYPE_LONGLONG, 1)

    // User defined types should default to a string that boost::lexical_cast
    // will convert
    {  // NOLINT[whitespace/parens]
        class UserType {};
        vector<char> buffer;
        OutputBinderParameterSetter<UserType> setter;
        setter.setParameter(&bind, &buffer, &nullFlag);
        BOOST_CHECK(bind.buffer == &buffer.at(0));
        BOOST_CHECK(bind.is_null == &nullFlag);
    }

    // std::shared_ptr should just forward to the default initialization
#ifndef SHARED_PTR_TYPE_TEST_SET_PARAMETER
#define SHARED_PTR_TYPE_TEST_SET_PARAMETER(type, mysqlType, isUnsigned) \
    { \
    vector<char> buffer; \
    OutputBinderParameterSetter<shared_ptr<type>> setter; \
    setter.setParameter(&bind, &buffer, &nullFlag); \
    BOOST_CHECK(sizeof(type) == buffer.size()); \
    BOOST_CHECK(bind.buffer == &buffer.at(0)); \
    BOOST_CHECK(static_cast<bool>(bind.is_unsigned) \
        == static_cast<bool>(isUnsigned)); \
    BOOST_CHECK(bind.is_null == &nullFlag); \
    }
#endif
    SHARED_PTR_TYPE_TEST_SET_PARAMETER(float,    MYSQL_TYPE_FLOAT,    0)
    SHARED_PTR_TYPE_TEST_SET_PARAMETER(double,   MYSQL_TYPE_DOUBLE,   0)
    SHARED_PTR_TYPE_TEST_SET_PARAMETER(int8_t,   MYSQL_TYPE_TINY,     0)
    SHARED_PTR_TYPE_TEST_SET_PARAMETER(uint8_t,  MYSQL_TYPE_TINY,     1)
    SHARED_PTR_TYPE_TEST_SET_PARAMETER(int16_t,  MYSQL_TYPE_SHORT,    0)
    SHARED_PTR_TYPE_TEST_SET_PARAMETER(uint16_t, MYSQL_TYPE_SHORT,    1)
    SHARED_PTR_TYPE_TEST_SET_PARAMETER(int32_t,  MYSQL_TYPE_LONG,     0)
    SHARED_PTR_TYPE_TEST_SET_PARAMETER(uint32_t, MYSQL_TYPE_LONG,     1)
    SHARED_PTR_TYPE_TEST_SET_PARAMETER(int64_t,  MYSQL_TYPE_LONGLONG, 0)
    SHARED_PTR_TYPE_TEST_SET_PARAMETER(uint64_t, MYSQL_TYPE_LONGLONG, 1)
}
