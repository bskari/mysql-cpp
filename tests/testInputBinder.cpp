#include <cstring>
#include <mysql/mysql.h>  // NOLINT[build/include_order]

#include <boost/test/unit_test.hpp>  // NOLINT[build/include_order]
#include <vector>
#include <string>

#include "testInputBinder.hpp"
#include "../InputBinder.hpp"

using std::string;
using std::vector;


void testBind() {
#ifndef INTEGRAL_TYPE_SPECIALIZATION_CHECK
#define INTEGRAL_TYPE_SPECIALIZATION_CHECK(type, mysqlType, isUnsigned) \
    { \
        vector<MYSQL_BIND> binds; \
        binds.resize(1); \
        InputBinder<0, type> binder; \
        type t = 0; \
        binder.bind(&binds, t); \
        BOOST_CHECK(mysqlType == binds.at(0).buffer_type); \
        BOOST_CHECK(0 == binds.at(0).is_null); \
        BOOST_CHECK(isUnsigned == binds.at(0).is_unsigned); \
        BOOST_CHECK(nullptr != binds.at(0).buffer); \
    }
#endif

    INTEGRAL_TYPE_SPECIALIZATION_CHECK(int,      MYSQL_TYPE_LONG,      0);
    INTEGRAL_TYPE_SPECIALIZATION_CHECK(short,    MYSQL_TYPE_SHORT,     0);  // NOLINT[runtime/int]
    INTEGRAL_TYPE_SPECIALIZATION_CHECK(int8_t,   MYSQL_TYPE_TINY,      0);
    INTEGRAL_TYPE_SPECIALIZATION_CHECK(uint8_t,  MYSQL_TYPE_TINY,      1);
    INTEGRAL_TYPE_SPECIALIZATION_CHECK(int16_t,  MYSQL_TYPE_SHORT,     0);
    INTEGRAL_TYPE_SPECIALIZATION_CHECK(uint16_t, MYSQL_TYPE_SHORT,     1);
    INTEGRAL_TYPE_SPECIALIZATION_CHECK(int32_t,  MYSQL_TYPE_LONG,      0);
    INTEGRAL_TYPE_SPECIALIZATION_CHECK(uint32_t, MYSQL_TYPE_LONG,      1);
    INTEGRAL_TYPE_SPECIALIZATION_CHECK(int64_t,  MYSQL_TYPE_LONGLONG,  0);
    INTEGRAL_TYPE_SPECIALIZATION_CHECK(uint64_t, MYSQL_TYPE_LONGLONG,  1);

    {
        vector<MYSQL_BIND> binds;
        binds.resize(1);
        InputBinder<0, float> binder;
        float t = 0.0;
        binder.bind(&binds, t);
        BOOST_CHECK(MYSQL_TYPE_FLOAT == binds.at(0).buffer_type);
        BOOST_CHECK(0 == binds.at(0).is_null);
        // MySQL ignores require the is_unsigned field for floting point types,
        // so don't check that attribute
        BOOST_CHECK(nullptr != binds.at(0).buffer);
    }

    {
        vector<MYSQL_BIND> binds;
        binds.resize(1);
        InputBinder<0, double> binder;
        double t = 0;
        binder.bind(&binds, t);
        BOOST_CHECK(MYSQL_TYPE_DOUBLE == binds.at(0).buffer_type);
        BOOST_CHECK(0 == binds.at(0).is_null);
        // MySQL ignores require the is_unsigned field for floting point types,
        // so don't check that attribute
        BOOST_CHECK(nullptr != binds.at(0).buffer);
    }

    {
        vector<MYSQL_BIND> binds;
        binds.resize(1);
        InputBinder<0, char*> binder;
        char t[50];
        strncpy(t, "Hello world", sizeof(t) / sizeof(t[0]));
        binder.bind(&binds, t);
        BOOST_CHECK(MYSQL_TYPE_STRING == binds.at(0).buffer_type);
        BOOST_CHECK(strlen(t) == binds.at(0).buffer_length);
        BOOST_CHECK(0 == binds.at(0).is_null);
        BOOST_CHECK(nullptr != binds.at(0).buffer);
    }

    {
        vector<MYSQL_BIND> binds;
        binds.resize(1);
        InputBinder<0, const char*> binder;
        const char t[50] = "Hello world";
        binder.bind(&binds, t);
        BOOST_CHECK(MYSQL_TYPE_STRING == binds.at(0).buffer_type);
        BOOST_CHECK(strlen(t) == binds.at(0).buffer_length);
        BOOST_CHECK(0 == binds.at(0).is_null);
        BOOST_CHECK(nullptr != binds.at(0).buffer);
    }

    {
        vector<MYSQL_BIND> binds;
        binds.resize(1);
        InputBinder<0, string> binder;
        string t("Hello world");
        binder.bind(&binds, t);
        BOOST_CHECK(MYSQL_TYPE_STRING == binds.at(0).buffer_type);
        BOOST_CHECK(t.size() == binds.at(0).buffer_length);
        BOOST_CHECK(0 == binds.at(0).is_null);
        BOOST_CHECK(nullptr != binds.at(0).buffer);
    }
}
