#include <cstdint>
#include <cstdlib>
#include <mysql/mysql.h>

#include <boost/test/unit_test.hpp>
#include <memory>
#include <string>
#include <sstream>
#include <vector>

#include "testOutputBinder.hpp"
#include "../MySqlException.hpp"
#include "../OutputBinder.hpp"

using std::shared_ptr;
using std::string;
using std::stringstream;
using std::unique_ptr;
using std::vector;

using namespace OutputBinderPrivate;

/**
 * Use this instead of boost::weakLexicalCast because we don't care about
 * truncation errors.
 */
template <typename Target, typename Source>
static Target weakLexicalCast(const Source& source) {
    Target returnValue;
    stringstream stream;
    stream << source;
    stream >> returnValue;
    return returnValue;
}


void testSetResult() {
    unsigned int randSeed = 0;
    MYSQL_BIND bind;
    my_bool nullFlag = false;

    // Put each test in {} to create a local scope so that I don't have to do
    // any weird name mangling
#ifndef TYPE_TEST_SET_RESULT
#define TYPE_TEST_SET_RESULT(type) \
    { \
    type result = weakLexicalCast<type>(rand_r(&randSeed)); \
    bind.buffer = &result; \
    bind.is_null = &nullFlag; \
    type output; \
    OutputBinderResultSetter<type>::setResult(&output, bind); \
    BOOST_CHECK(result == output); \
    }
#endif
    TYPE_TEST_SET_RESULT(int)    // NOLINT[readability/casting]
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
    TYPE_TEST_SET_RESULT(float)  // NOLINT[readability/casting]
    TYPE_TEST_SET_RESULT(double) // NOLINT[readability/casting]
#pragma GCC diagnostic pop
    TYPE_TEST_SET_RESULT(char)   // NOLINT[readability/casting]
    TYPE_TEST_SET_RESULT(int8_t)
    TYPE_TEST_SET_RESULT(uint8_t)
    TYPE_TEST_SET_RESULT(int16_t)
    TYPE_TEST_SET_RESULT(uint16_t)
    TYPE_TEST_SET_RESULT(int32_t)
    TYPE_TEST_SET_RESULT(uint32_t)
    TYPE_TEST_SET_RESULT(int64_t)
    TYPE_TEST_SET_RESULT(uint64_t)
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
        OutputBinderResultSetter<string>::setResult(&output, bind);
        BOOST_CHECK(result.size() == output.size());
        BOOST_CHECK(result == output);
    }

    // std::shared_ptr with NULL test
#ifndef NULL_SHARED_PTR_TYPE_TEST_SET_RESULT
#define NULL_SHARED_PTR_TYPE_TEST_SET_RESULT(type) \
    { \
        type output = weakLexicalCast<type>(rand_r(&randSeed)); \
        shared_ptr<type> result; \
        nullFlag = true; \
        bind.buffer = &output; \
        bind.is_null = &nullFlag; \
        OutputBinderResultSetter<decltype(result)>::setResult(&result, bind); \
        BOOST_CHECK(0 == result.get()); \
    }
#endif
    NULL_SHARED_PTR_TYPE_TEST_SET_RESULT(int)    // NOLINT[readability/casting]
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
    NULL_SHARED_PTR_TYPE_TEST_SET_RESULT(float)  // NOLINT[readability/casting]
    NULL_SHARED_PTR_TYPE_TEST_SET_RESULT(double) // NOLINT[readability/casting]
#pragma GCC diagnostic pop
    NULL_SHARED_PTR_TYPE_TEST_SET_RESULT(char)   // NOLINT[readability/casting]
    NULL_SHARED_PTR_TYPE_TEST_SET_RESULT(int8_t)
    NULL_SHARED_PTR_TYPE_TEST_SET_RESULT(uint8_t)
    NULL_SHARED_PTR_TYPE_TEST_SET_RESULT(int16_t)
    NULL_SHARED_PTR_TYPE_TEST_SET_RESULT(uint16_t)
    NULL_SHARED_PTR_TYPE_TEST_SET_RESULT(int32_t)
    NULL_SHARED_PTR_TYPE_TEST_SET_RESULT(uint32_t)
    NULL_SHARED_PTR_TYPE_TEST_SET_RESULT(int64_t)
    NULL_SHARED_PTR_TYPE_TEST_SET_RESULT(uint64_t)
    NULL_SHARED_PTR_TYPE_TEST_SET_RESULT(string)

    // std::shared_ptr with a value test
#ifndef SHARED_PTR_TYPE_TEST_SET_RESULT
#define SHARED_PTR_TYPE_TEST_SET_RESULT(type) \
    { \
        type output = weakLexicalCast<type>(rand_r(&randSeed)); \
        shared_ptr<decltype(output)> ptr; \
        bind.buffer = &output; \
        nullFlag = false; \
        bind.is_null = &nullFlag; \
        OutputBinderResultSetter<decltype(ptr)>::setResult(&ptr, bind); \
        BOOST_CHECK(nullptr != ptr.get()); \
        if (nullptr != ptr.get()) { \
            BOOST_CHECK(output == *ptr); \
        } \
    }
#endif
    SHARED_PTR_TYPE_TEST_SET_RESULT(int)    // NOLINT[readability/casting]
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
    SHARED_PTR_TYPE_TEST_SET_RESULT(float)  // NOLINT[readability/casting]
    SHARED_PTR_TYPE_TEST_SET_RESULT(double) // NOLINT[readability/casting]
#pragma GCC diagnostic pop
    SHARED_PTR_TYPE_TEST_SET_RESULT(int8_t)
    SHARED_PTR_TYPE_TEST_SET_RESULT(uint8_t)
    SHARED_PTR_TYPE_TEST_SET_RESULT(int16_t)
    SHARED_PTR_TYPE_TEST_SET_RESULT(uint16_t)
    SHARED_PTR_TYPE_TEST_SET_RESULT(int32_t)
    SHARED_PTR_TYPE_TEST_SET_RESULT(uint32_t)
    SHARED_PTR_TYPE_TEST_SET_RESULT(int64_t)
    SHARED_PTR_TYPE_TEST_SET_RESULT(uint64_t)
    {  // NOLINT[whitespace/parens]
        string result(" ", 5);
        shared_ptr<decltype(result)> ptr;
        vector<char> buffer(result.size());
        for (size_t i = 0; i < result.size(); ++i) {
            // Let's avoid \0
            buffer.at(i) = result.at(i) = static_cast<char>(
                (rand_r(&randSeed) % 10 + 'a'));
        }
        buffer.push_back('\0');
        bind.buffer = &buffer.at(0);
        nullFlag = false;
        bind.is_null = &nullFlag;
        OutputBinderResultSetter<decltype(ptr)>::setResult(&ptr, bind);
        BOOST_CHECK(nullptr != ptr.get());
        if (nullptr != ptr.get()) {
            BOOST_CHECK(result.size() == ptr->size());
            BOOST_CHECK(result == *ptr);
        }
    }

    // Trying to set a NULL value with a non-std::shared_ptr parameter should
    // throw
#ifndef NULL_NON_SHARED_PTR_TYPE_TEST_SET_RESULT
#define NULL_NON_SHARED_PTR_TYPE_TEST_SET_RESULT(type) \
    { \
        type output = weakLexicalCast<type>(rand_r(&randSeed)); \
        nullFlag = true; \
        bind.buffer = &output; \
        bind.is_null = &nullFlag; \
        BOOST_CHECK_THROW( \
            OutputBinderResultSetter<decltype(output)>::setResult(&output, bind), \
            MySqlException); \
    }
#endif
    NULL_NON_SHARED_PTR_TYPE_TEST_SET_RESULT(int)    // NOLINT[readability/casting]
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
    NULL_NON_SHARED_PTR_TYPE_TEST_SET_RESULT(float)  // NOLINT[readability/casting]
    NULL_NON_SHARED_PTR_TYPE_TEST_SET_RESULT(double) // NOLINT[readability/casting]
#pragma GCC diagnostic pop
    NULL_NON_SHARED_PTR_TYPE_TEST_SET_RESULT(char)   // NOLINT[readability/casting]
    NULL_NON_SHARED_PTR_TYPE_TEST_SET_RESULT(int8_t)
    NULL_NON_SHARED_PTR_TYPE_TEST_SET_RESULT(uint8_t)
    NULL_NON_SHARED_PTR_TYPE_TEST_SET_RESULT(int16_t)
    NULL_NON_SHARED_PTR_TYPE_TEST_SET_RESULT(uint16_t)
    NULL_NON_SHARED_PTR_TYPE_TEST_SET_RESULT(int32_t)
    NULL_NON_SHARED_PTR_TYPE_TEST_SET_RESULT(uint32_t)
    NULL_NON_SHARED_PTR_TYPE_TEST_SET_RESULT(int64_t)
    NULL_NON_SHARED_PTR_TYPE_TEST_SET_RESULT(uint64_t)
    NULL_NON_SHARED_PTR_TYPE_TEST_SET_RESULT(string)

    // std::unique_ptr with NULL test
#ifndef NULL_UNIQUE_PTR_TYPE_TEST_SET_RESULT
#define NULL_UNIQUE_PTR_TYPE_TEST_SET_RESULT(type) \
    { \
        type output = weakLexicalCast<type>(rand_r(&randSeed)); \
        unique_ptr<type> result; \
        nullFlag = true; \
        bind.buffer = &output; \
        bind.is_null = &nullFlag; \
        OutputBinderResultSetter<decltype(result)>::setResult(&result, bind); \
        BOOST_CHECK(0 == result.get()); \
    }
#endif
    NULL_UNIQUE_PTR_TYPE_TEST_SET_RESULT(int)    // NOLINT[readability/casting]
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
    NULL_UNIQUE_PTR_TYPE_TEST_SET_RESULT(float)  // NOLINT[readability/casting]
    NULL_UNIQUE_PTR_TYPE_TEST_SET_RESULT(double) // NOLINT[readability/casting]
#pragma GCC diagnostic pop
    NULL_UNIQUE_PTR_TYPE_TEST_SET_RESULT(char)   // NOLINT[readability/casting]
    NULL_UNIQUE_PTR_TYPE_TEST_SET_RESULT(int8_t)
    NULL_UNIQUE_PTR_TYPE_TEST_SET_RESULT(uint8_t)
    NULL_UNIQUE_PTR_TYPE_TEST_SET_RESULT(int16_t)
    NULL_UNIQUE_PTR_TYPE_TEST_SET_RESULT(uint16_t)
    NULL_UNIQUE_PTR_TYPE_TEST_SET_RESULT(int32_t)
    NULL_UNIQUE_PTR_TYPE_TEST_SET_RESULT(uint32_t)
    NULL_UNIQUE_PTR_TYPE_TEST_SET_RESULT(int64_t)
    NULL_UNIQUE_PTR_TYPE_TEST_SET_RESULT(uint64_t)
    NULL_UNIQUE_PTR_TYPE_TEST_SET_RESULT(string)

    // std::unique_ptr with a value test
#ifndef UNIQUE_PTR_TYPE_TEST_SET_RESULT
#define UNIQUE_PTR_TYPE_TEST_SET_RESULT(type) \
    { \
        type output = weakLexicalCast<type>(rand_r(&randSeed)); \
        unique_ptr<decltype(output)> ptr; \
        bind.buffer = &output; \
        nullFlag = false; \
        bind.is_null = &nullFlag; \
        OutputBinderResultSetter<decltype(ptr)>::setResult(&ptr, bind); \
        BOOST_CHECK(nullptr != ptr.get()); \
        if (nullptr != ptr.get()) { \
            BOOST_CHECK(output == *ptr); \
        } \
    }
#endif
    UNIQUE_PTR_TYPE_TEST_SET_RESULT(int)    // NOLINT[readability/casting]
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
    UNIQUE_PTR_TYPE_TEST_SET_RESULT(float)  // NOLINT[readability/casting]
    UNIQUE_PTR_TYPE_TEST_SET_RESULT(double) // NOLINT[readability/casting]
#pragma GCC diagnostic pop
    UNIQUE_PTR_TYPE_TEST_SET_RESULT(int8_t)
    UNIQUE_PTR_TYPE_TEST_SET_RESULT(uint8_t)
    UNIQUE_PTR_TYPE_TEST_SET_RESULT(int16_t)
    UNIQUE_PTR_TYPE_TEST_SET_RESULT(uint16_t)
    UNIQUE_PTR_TYPE_TEST_SET_RESULT(int32_t)
    UNIQUE_PTR_TYPE_TEST_SET_RESULT(uint32_t)
    UNIQUE_PTR_TYPE_TEST_SET_RESULT(int64_t)
    UNIQUE_PTR_TYPE_TEST_SET_RESULT(uint64_t)
    {  // NOLINT[whitespace/parens]
        string result(" ", 5);
        unique_ptr<decltype(result)> ptr;
        vector<char> buffer(result.size());
        for (size_t i = 0; i < result.size(); ++i) {
            // Let's avoid \0
            buffer.at(i) = result.at(i) = static_cast<char>(
                (rand_r(&randSeed) % 10 + 'a'));
        }
        buffer.push_back('\0');
        bind.buffer = &buffer.at(0);
        nullFlag = false;
        bind.is_null = &nullFlag;
        OutputBinderResultSetter<decltype(ptr)>::setResult(&ptr, bind);
        BOOST_CHECK(nullptr != ptr.get());
        if (nullptr != ptr.get()) {
            BOOST_CHECK(result.size() == ptr->size());
            BOOST_CHECK(result == *ptr);
        }
    }

    // Trying to set a NULL value with a non-std::unique_ptr parameter should
    // throw
#ifndef NULL_NON_UNIQUE_PTR_TYPE_TEST_SET_RESULT
#define NULL_NON_UNIQUE_PTR_TYPE_TEST_SET_RESULT(type) \
    { \
        type output = weakLexicalCast<type>(rand_r(&randSeed)); \
        nullFlag = true; \
        bind.buffer = &output; \
        bind.is_null = &nullFlag; \
        BOOST_CHECK_THROW( \
            OutputBinderResultSetter<decltype(output)>::setResult(&output, bind), \
            MySqlException); \
    }
#endif
    NULL_NON_UNIQUE_PTR_TYPE_TEST_SET_RESULT(int)    // NOLINT[readability/casting]
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
    NULL_NON_UNIQUE_PTR_TYPE_TEST_SET_RESULT(float)  // NOLINT[readability/casting]
    NULL_NON_UNIQUE_PTR_TYPE_TEST_SET_RESULT(double) // NOLINT[readability/casting]
#pragma GCC diagnostic pop
    NULL_NON_UNIQUE_PTR_TYPE_TEST_SET_RESULT(char)   // NOLINT[readability/casting]
    NULL_NON_UNIQUE_PTR_TYPE_TEST_SET_RESULT(int8_t)
    NULL_NON_UNIQUE_PTR_TYPE_TEST_SET_RESULT(uint8_t)
    NULL_NON_UNIQUE_PTR_TYPE_TEST_SET_RESULT(int16_t)
    NULL_NON_UNIQUE_PTR_TYPE_TEST_SET_RESULT(uint16_t)
    NULL_NON_UNIQUE_PTR_TYPE_TEST_SET_RESULT(int32_t)
    NULL_NON_UNIQUE_PTR_TYPE_TEST_SET_RESULT(uint32_t)
    NULL_NON_UNIQUE_PTR_TYPE_TEST_SET_RESULT(int64_t)
    NULL_NON_UNIQUE_PTR_TYPE_TEST_SET_RESULT(uint64_t)
    NULL_NON_UNIQUE_PTR_TYPE_TEST_SET_RESULT(string)
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
    OutputBinderParameterSetter<type>::setParameter(&bind, &buffer, &nullFlag); \
    BOOST_CHECK(sizeof(type) == buffer.size()); \
    BOOST_CHECK(bind.buffer == &buffer.at(0)); \
    BOOST_CHECK( \
        static_cast<bool>(bind.is_unsigned) \
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
        OutputBinderParameterSetter<UserType>::setParameter(&bind, &buffer, &nullFlag);
        BOOST_CHECK(bind.buffer == &buffer.at(0));
        BOOST_CHECK(bind.is_null == &nullFlag);
    }

    // std::shared_ptr should just forward to the default initialization
#ifndef SHARED_PTR_TYPE_TEST_SET_PARAMETER
#define SHARED_PTR_TYPE_TEST_SET_PARAMETER(type, mysqlType, isUnsigned) \
    { \
    vector<char> buffer; \
    OutputBinderParameterSetter<shared_ptr<type>>::setParameter(&bind, &buffer, &nullFlag); \
    BOOST_CHECK(sizeof(type) == buffer.size()); \
    BOOST_CHECK(bind.buffer == &buffer.at(0)); \
    BOOST_CHECK( \
        static_cast<bool>(bind.is_unsigned) \
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
