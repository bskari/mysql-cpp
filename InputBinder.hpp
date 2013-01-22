#ifndef INPUTBINDER_HPP_
#define INPUTBINDER_HPP_

#include <cstdint>
#include <cstring>
#include <mysql/mysql.h>

#include <iostream>
#include <string>
#include <vector>

// C++11 doesn't allow for partial template specialization of variadic
// templated functions, but it does of classes
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
        Head,
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
        const char* const value,
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
struct InputBinder<N, char*, Tail...>
{
    static void bind(
        std::vector<MYSQL_BIND>* const bindParameters,
        char* const value,
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
        std::cout << "string bind" << std::endl;
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
        std::cout << "uint32_t bind" << std::endl;
        // Set up the bind parameters
        MYSQL_BIND& bindParameter = bindParameters->at(N);

        bindParameter.buffer_type = MYSQL_TYPE_LONG;
        bindParameter.buffer = const_cast<void*>(
            static_cast<const void*>(&value)
        );
        bindParameter.buffer_length = 0;
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
        std::cout << "uint32_t bind" << std::endl;
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


#endif  // INPUTBINDER_HPP_
