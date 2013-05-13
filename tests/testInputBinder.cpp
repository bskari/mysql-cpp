#include "testInputBinder.hpp"
#include "../InputBinder.hpp"

#include <boost/test/unit_test.hpp>
#include <mysql/mysql.h>
#include <vector>

using std::vector;


void testBind()
{
    vector<MYSQL_BIND> binds;

#ifndef INTEGRAL_TYPE_SPECIALIZATION_CHECK
#define INTEGRAL_TYPE_SPECIALIZATION_CHECK(type, mysqlType, isUnsigned) \
    { \
        InputBinder<0, type> binder; \
        type t = 0; \
        binds.resize(1); \
        binder.bind(&binds, t); \
        BOOST_CHECK(mysqlType == binds.at(0).buffer_type); \
        BOOST_CHECK(0 == binds.at(0).is_null); \
        BOOST_CHECK(isUnsigned == binds.at(0).is_unsigned); \
        BOOST_CHECK(nullptr != binds.at(0).buffer); \
        binds.clear(); \
    }
#endif

    INTEGRAL_TYPE_SPECIALIZATION_CHECK(int,      MYSQL_TYPE_LONG,      0);
    INTEGRAL_TYPE_SPECIALIZATION_CHECK(short,    MYSQL_TYPE_SHORT,     0);
    INTEGRAL_TYPE_SPECIALIZATION_CHECK(int8_t,   MYSQL_TYPE_TINY,      0);
    INTEGRAL_TYPE_SPECIALIZATION_CHECK(uint8_t,  MYSQL_TYPE_TINY,      1);
    INTEGRAL_TYPE_SPECIALIZATION_CHECK(int16_t,  MYSQL_TYPE_SHORT,     0);
    INTEGRAL_TYPE_SPECIALIZATION_CHECK(uint16_t, MYSQL_TYPE_SHORT,     1);
    INTEGRAL_TYPE_SPECIALIZATION_CHECK(int32_t,  MYSQL_TYPE_LONG,      0);
    INTEGRAL_TYPE_SPECIALIZATION_CHECK(uint32_t, MYSQL_TYPE_LONG,      1);
    INTEGRAL_TYPE_SPECIALIZATION_CHECK(int64_t,  MYSQL_TYPE_LONGLONG,  0);
    INTEGRAL_TYPE_SPECIALIZATION_CHECK(uint64_t, MYSQL_TYPE_LONGLONG,  1);

    InputBinder<0, float> floatBinder;
    float Float = 0.0;
    binds.resize(1);
    floatBinder.bind(&binds, Float);
    BOOST_CHECK(MYSQL_TYPE_FLOAT == binds.at(0).buffer_type);
    BOOST_CHECK(0 == binds.at(0).is_null);
    BOOST_CHECK(nullptr != binds.at(0).buffer);
    binds.clear();

    InputBinder<0, double> doubleBinder;
    double Double = 0;
    binds.resize(1);
    doubleBinder.bind(&binds, Double);
    BOOST_CHECK(MYSQL_TYPE_DOUBLE == binds.at(0).buffer_type);
    BOOST_CHECK(0 == binds.at(0).is_null);
    BOOST_CHECK(nullptr != binds.at(0).buffer);
    binds.clear();
}
