#ifndef INPUTBINDER_HPP_
#define INPUTBINDER_HPP_

#include <cstdint>
#include <cstring>
#include <mysql/mysql.h>

#include <string>
#include <vector>

// C++11 doesn't allow for partial template specialization of variadic
// templated functions, but it does of classes, so wrap this function in a
// class
template <size_t N, typename... Args>
struct InputBinder
{
    static void bind(std::vector<MYSQL_BIND>* const) {}
};


template <size_t N, typename Head, typename... Tail>
struct InputBinder<N, Head, Tail...>
{
    // This is purposely left undefined; individual types should have partial
    // template specialized instances defined for them instead.
    static void bind(
        std::vector<MYSQL_BIND>* const bindParameters,
        Head value,
        Tail... tail
    );
};


// ************************************************
// Partial template specialization for char pointer
// ************************************************
template <size_t N, typename... Tail>
struct InputBinder<N, const char*, Tail...>
{
    static void bind(
        std::vector<MYSQL_BIND>* const bindParameters,
        const char* const& value,
        Tail... tail
    )
    {
        // Set up the bind parameters
        MYSQL_BIND& bindParameter = bindParameters->at(N);

        bindParameter.buffer_type = MYSQL_TYPE_STRING;
        bindParameter.buffer = const_cast<void*>(
            static_cast<const void*>(value)
        );
        bindParameter.buffer_length = strlen(value);
        bindParameter.length = &bindParameter.buffer_length;
        bindParameter.is_unsigned = 0;
        bindParameter.is_null = 0;

        InputBinder<N + 1, Tail...> b;
        b.bind(bindParameters, tail...);
    }
};
template <size_t N, typename... Tail>
struct InputBinder<N, char*&, Tail...>
{
    static void bind(
        std::vector<MYSQL_BIND>* const bindParameters,
        const char* const& value,
        Tail... tail
    )
    {
        bind(bindParameters, value, tail...);
    }
};


// ******************************************
// Partial template specialization for string
// ******************************************
template <size_t N, typename... Tail>
struct InputBinder<N, std::string, Tail...>
{
    static void bind(
        std::vector<MYSQL_BIND>* const bindParameters,
        const std::string& value,
        Tail... tail
    )
    {
        // Set up the bind parameters
        MYSQL_BIND& bindParameter = bindParameters->at(N);

        bindParameter.buffer_type = MYSQL_TYPE_STRING;
        bindParameter.buffer = const_cast<void*>(
            static_cast<const void*>(value.c_str())
        );
        bindParameter.buffer_length = value.length();
        bindParameter.length = &bindParameter.buffer_length;
        bindParameter.is_unsigned = 0;
        bindParameter.is_null = 0;

        InputBinder<N + 1, Tail...> b;
        b.bind(bindParameters, tail...);
    }
};


// ********************************************
// Partial template specialization for int32_t
// ********************************************
template <size_t N, typename... Tail>
struct InputBinder<N, int32_t, Tail...>
{
    static void bind(
        std::vector<MYSQL_BIND>* const bindParameters,
        const int32_t& value,
        Tail... tail
    )
    {
        // Set up the bind parameters
        MYSQL_BIND& bindParameter = bindParameters->at(N);

        bindParameter.buffer_type = MYSQL_TYPE_LONG;
        bindParameter.buffer = const_cast<void*>(
            static_cast<const void*>(&value)
        );
        bindParameter.is_unsigned = 0;
        bindParameter.is_null = 0;

        InputBinder<N + 1, Tail...> b;
        b.bind(bindParameters, tail...);
    }
};


// ********************************************
// Partial template specialization for uint32_t
// ********************************************
template <size_t N, typename... Tail>
struct InputBinder<N, uint32_t, Tail...>
{
    static void bind(
        std::vector<MYSQL_BIND>* const bindParameters,
        const uint32_t& value,
        Tail... tail
    )
    {
        // Set up the bind parameters
        MYSQL_BIND& bindParameter = bindParameters->at(N);

        bindParameter.buffer_type = MYSQL_TYPE_LONG;
        bindParameter.buffer = const_cast<void*>(
            static_cast<const void*>(&value)
        );
        bindParameter.is_unsigned = 1;
        bindParameter.is_null = 0;

        InputBinder<N + 1, Tail...> b;
        b.bind(bindParameters, tail...);
    }
};


// ********************************************
// Partial template specialization for int16_t
// ********************************************
template <size_t N, typename... Tail>
struct InputBinder<N, int16_t, Tail...>
{
    static void bind(
        std::vector<MYSQL_BIND>* const bindParameters,
        const int16_t& value,
        Tail... tail
    )
    {
        // Set up the bind parameters
        MYSQL_BIND& bindParameter = bindParameters->at(N);

        bindParameter.buffer_type = MYSQL_TYPE_SHORT;
        bindParameter.buffer = const_cast<void*>(
            static_cast<const void*>(&value)
        );
        bindParameter.is_unsigned = 0;
        bindParameter.is_null = 0;

        InputBinder<N + 1, Tail...> b;
        b.bind(bindParameters, tail...);
    }
};


// ********************************************
// Partial template specialization for uint16_t
// ********************************************
template <size_t N, typename... Tail>
struct InputBinder<N, uint16_t, Tail...>
{
    static void bind(
        std::vector<MYSQL_BIND>* const bindParameters,
        const uint16_t& value,
        Tail... tail
    )
    {
        // Set up the bind parameters
        MYSQL_BIND& bindParameter = bindParameters->at(N);

        bindParameter.buffer_type = MYSQL_TYPE_SHORT;
        bindParameter.buffer = const_cast<void*>(
            static_cast<const void*>(&value)
        );
        bindParameter.is_unsigned = 1;
        bindParameter.is_null = 0;

        InputBinder<N + 1, Tail...> b;
        b.bind(bindParameters, tail...);
    }
};


// ********************************************
// Partial template specialization for int8_t
// ********************************************
template <size_t N, typename... Tail>
struct InputBinder<N, int8_t, Tail...>
{
    static void bind(
        std::vector<MYSQL_BIND>* const bindParameters,
        const int8_t& value,
        Tail... tail
    )
    {
        // Set up the bind parameters
        MYSQL_BIND& bindParameter = bindParameters->at(N);

        bindParameter.buffer_type = MYSQL_TYPE_TINY;
        bindParameter.buffer = const_cast<void*>(
            static_cast<const void*>(&value)
        );
        bindParameter.is_unsigned = 0;
        bindParameter.is_null = 0;

        InputBinder<N + 1, Tail...> b;
        b.bind(bindParameters, tail...);
    }
};


// ********************************************
// Partial template specialization for uint8_t
// ********************************************
template <size_t N, typename... Tail>
struct InputBinder<N, uint8_t, Tail...>
{
    static void bind(
        std::vector<MYSQL_BIND>* const bindParameters,
        const uint8_t& value,
        Tail... tail
    )
    {
        // Set up the bind parameters
        MYSQL_BIND& bindParameter = bindParameters->at(N);

        bindParameter.buffer_type = MYSQL_TYPE_TINY;
        bindParameter.buffer = const_cast<void*>(
            static_cast<const void*>(&value)
        );
        bindParameter.is_unsigned = 1;
        bindParameter.is_null = 0;

        InputBinder<N + 1, Tail...> b;
        b.bind(bindParameters, tail...);
    }
};


// ********************************************
// Partial template specialization for float
// ********************************************
template <size_t N, typename... Tail>
struct InputBinder<N, float, Tail...>
{
    static void bind(
        std::vector<MYSQL_BIND>* const bindParameters,
        const float& value,
        Tail... tail
    )
    {
        static_assert(4 == sizeof(float), "Unexpected float size");

        // Set up the bind parameters
        MYSQL_BIND& bindParameter = bindParameters->at(N);

        bindParameter.buffer_type = MYSQL_TYPE_FLOAT;
        bindParameter.buffer = const_cast<void*>(
            static_cast<const void*>(&value)
        );
        bindParameter.is_null = 0;

        InputBinder<N + 1, Tail...> b;
        b.bind(bindParameters, tail...);
    }
};


// ********************************************
// Partial template specialization for double
// ********************************************
template <size_t N, typename... Tail>
struct InputBinder<N, double, Tail...>
{
    static void bind(
        std::vector<MYSQL_BIND>* const bindParameters,
        const double& value,
        Tail... tail
    )
    {
        static_assert(8 == sizeof(double), "Unexpected double size");

        // Set up the bind parameters
        MYSQL_BIND& bindParameter = bindParameters->at(N);

        bindParameter.buffer_type = MYSQL_TYPE_DOUBLE;
        bindParameter.buffer = const_cast<void*>(
            static_cast<const void*>(&value)
        );
        bindParameter.is_null = 0;

        InputBinder<N + 1, Tail...> b;
        b.bind(bindParameters, tail...);
    }
};


#endif  // INPUTBINDER_HPP_
