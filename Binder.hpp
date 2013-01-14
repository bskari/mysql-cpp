#ifndef BINDER_HPP_
#define BINDER_HPP_

#include <cstdint>
#include <cstring>
#include <mysql/mysql.h>

#include <iostream>
#include <string>
#include <vector>

// C++11 doesn't allow for partial template specialization of variadic
// templated functions, but it does of classes
template <size_t N, typename... Args>
struct Binder
{
    static void bind(std::vector<MYSQL_BIND>* const) {}
};

template <size_t N, typename Head, typename... Tail>
struct Binder<N, Head, Tail...>
{
    // This is purposely left undefined; individual types should have partial
    // template specialized instances defined for them instead.
    static void bind(
        std::vector<MYSQL_BIND>* const bindParameters,
        Head,
        Tail... tail
    );
};


// **********************************************
// Partial template specialization for char array
// **********************************************
template <size_t N, typename... Tail>
struct Binder<N, const char[6], Tail...>
{
    static void bind(
        std::vector<MYSQL_BIND>* const bindParameters,
        const char value[6],
        Tail... tail
    )
    {
        std::cout << "char array bind" << std::endl;
        // Set up the bind parameters
        MYSQL_BIND& bindParameter = bindParameters->at(N);

        bindParameter.buffer_type = MYSQL_TYPE_STRING;
        bindParameter.buffer = const_cast<void*>(
            static_cast<const void*>(&value)
        );
        bindParameter.buffer_length = 6 - 1;
        bindParameter.length = &bindParameter.buffer_length;
        bindParameter.is_unsigned = 0;
        bindParameter.is_null = 0;

        Binder<N + 1, Tail...> b;
        b.bind(bindParameters, tail...);
    }
    void foo() {}
};


// ************************************************
// Partial template specialization for char pointer
// ************************************************
template <size_t N, typename... Tail>
struct Binder<N, const char*, Tail...>
{
    static void bind(
        std::vector<MYSQL_BIND>* const bindParameters,
        const char* const value,
        Tail... tail
    )
    {
        std::cout << "C-string bind" << std::endl;
        // Set up the bind parameters
        MYSQL_BIND& bindParameter = bindParameters->at(N);

        bindParameter.buffer_type = MYSQL_TYPE_STRING;
        bindParameter.buffer = value;
        bindParameter.buffer_length = strlen(value);
        bindParameter.length = &bindParameter.buffer_length;
        bindParameter.is_unsigned = 0;
        bindParameter.is_null = 0;

        Binder<N + 1, Tail...> b;
        b.bind(bindParameters, tail...);
    }
};


// ******************************************
// Partial template specialization for string
// ******************************************
template <size_t N, typename... Tail>
struct Binder<N, const std::string&, Tail...>
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

        Binder<N + 1, Tail...> b;
        b.bind(bindParameters, tail...);
    }
};


// ********************************************
// Partial template specialization for int32_t
// ********************************************
template <size_t N, typename... Tail>
struct Binder<N, const int32_t&, Tail...>
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

        Binder<N + 1, Tail...> b;
        b.bind(bindParameters, tail...);
    }
};


// ********************************************
// Partial template specialization for uint32_t
// ********************************************
template <size_t N, typename... Tail>
struct Binder<N, const uint32_t, Tail...>
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

        Binder<N + 1, Tail...> b;
        b.bind(bindParameters, tail...);
    }
};


#endif  // BINDER_HPP_
