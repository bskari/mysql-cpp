#include "testInputBinder.hpp"
#include "../InputBinder.hpp"

#include <boost/test/unit_test.hpp>
#include <mysql/mysql.h>
#include <vector>

using std::vector;

#include <iostream>
using std::cout;
using std::endl;


void testBind()
{
    vector<MYSQL_BIND> binds;

    InputBinder<0, int> intBinder;
    int Int = 0;
    binds.resize(1);
    intBinder.bind(&binds, Int);
    BOOST_CHECK(MYSQL_TYPE_LONG == binds.at(0).buffer_type);
    BOOST_CHECK(0 == binds.at(0).is_null);
    BOOST_CHECK(0 == binds.at(0).is_unsigned);
    BOOST_CHECK(nullptr != binds.at(0).buffer);
    binds.clear();

    InputBinder<0, short> shortBinder;
    short Short = 0;
    binds.resize(1);
    shortBinder.bind(&binds, Short);
    BOOST_CHECK(MYSQL_TYPE_SHORT == binds.at(0).buffer_type);
    BOOST_CHECK(0 == binds.at(0).is_null);
    BOOST_CHECK(0 == binds.at(0).is_unsigned);
    BOOST_CHECK(nullptr != binds.at(0).buffer);
    binds.clear();

    InputBinder<0, int32_t> int32_tBinder;
    int32_t Int32_t = 0;
    binds.resize(1);
    int32_tBinder.bind(&binds, Int32_t);
    BOOST_CHECK(MYSQL_TYPE_LONG == binds.at(0).buffer_type);
    BOOST_CHECK(0 == binds.at(0).is_null);
    BOOST_CHECK(0 == binds.at(0).is_unsigned);
    BOOST_CHECK(nullptr != binds.at(0).buffer);
    binds.clear();

    InputBinder<0, int16_t> int16_tBinder;
    int16_t Int16_t = 0;
    binds.resize(1);
    int16_tBinder.bind(&binds, Int16_t);
    BOOST_CHECK(MYSQL_TYPE_SHORT == binds.at(0).buffer_type);
    BOOST_CHECK(0 == binds.at(0).is_null);
    BOOST_CHECK(0 == binds.at(0).is_unsigned);
    BOOST_CHECK(nullptr != binds.at(0).buffer);
    binds.clear();

    InputBinder<0, int8_t> int8_tBinder;
    int8_t Int8_t = 0;
    binds.resize(1);
    int8_tBinder.bind(&binds, Int8_t);
    BOOST_CHECK(MYSQL_TYPE_TINY == binds.at(0).buffer_type);
    BOOST_CHECK(0 == binds.at(0).is_null);
    BOOST_CHECK(0 == binds.at(0).is_unsigned);
    BOOST_CHECK(nullptr != binds.at(0).buffer);
    binds.clear();

    InputBinder<0, uint32_t> uint32_tBinder;
    uint32_t Uint32_t = 0;
    binds.resize(1);
    uint32_tBinder.bind(&binds, Uint32_t);
    BOOST_CHECK(MYSQL_TYPE_LONG == binds.at(0).buffer_type);
    BOOST_CHECK(0 == binds.at(0).is_null);
    BOOST_CHECK(1 == binds.at(0).is_unsigned);
    BOOST_CHECK(nullptr != binds.at(0).buffer);
    binds.clear();

    InputBinder<0, uint16_t> uint16_tBinder;
    uint16_t Uint16_t = 0;
    binds.resize(1);
    uint16_tBinder.bind(&binds, Uint16_t);
    BOOST_CHECK(MYSQL_TYPE_SHORT == binds.at(0).buffer_type);
    BOOST_CHECK(0 == binds.at(0).is_null);
    BOOST_CHECK(1 == binds.at(0).is_unsigned);
    BOOST_CHECK(nullptr != binds.at(0).buffer);
    binds.clear();

    InputBinder<0, uint8_t> uint8_tBinder;
    uint8_t Uint8_t = 0;
    binds.resize(1);
    uint8_tBinder.bind(&binds, Uint8_t);
    BOOST_CHECK(MYSQL_TYPE_TINY == binds.at(0).buffer_type);
    BOOST_CHECK(0 == binds.at(0).is_null);
    BOOST_CHECK(1 == binds.at(0).is_unsigned);
    BOOST_CHECK(nullptr != binds.at(0).buffer);
    binds.clear();

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
