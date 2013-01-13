#ifndef BINDER_HPP_
#define BINDER_HPP_

#include <cstdint>
#include <mysql/mysql.h>

#include <vector>

// C++11 doesn't allow for partial template specialization of variadic
// templated functions, but it does of classes
template <size_t N, typename... Args>
struct Binder
{
    static void bind(
        std::vector<MYSQL_BIND>* const bindParameters,
        Args... parameters
    )
    {
    }
};

template <size_t N, typename Head, typename... Tail>
struct Binder<N, Head, Tail...>
{
    static void bind(
        std::vector<MYSQL_BIND>* const bindParameters,
        const Head& head,
        Tail... tail
    );
};


// ******************************************
// Partial template specialization for string
// ******************************************
template <size_t N, typename... Tail>
struct Binder<N, const std::string& value, Tail...>
{
    static void bind(
        std::vector<MYSQL_BIND>* const bindParameters,
        const uint32_t& value,
        const Tail&... tail
    )
    {
        // Set up the bind parameters
        MYSQL_BIND& bindParameter = bindParameters->at(N);

        bindParameter.buffer_type = MYSQL_TYPE_STRING;
        bindParameter.buffer = value.c_str();
        bindParameter.is_unsigned = 0;
        bindParameter.is_null = 0;
        bindParameter.length = value.length();

        Binder<N + 1, Tail...> b;
        b.bind(bindParameters);
    }
};


// ********************************************
// Partial template specialization for uint32_t
// ********************************************
template <size_t N, typename... Tail>
struct Binder<N, uint32_t, Tail...>
{
    static void bind(
        std::vector<MYSQL_BIND>* const bindParameters,
        const uint32_t& value,
        const Tail&... tail
    )
    {
        // Set up the bind parameters
        MYSQL_BIND& bindParameter = bindParameters->at(N);

        bindParameter.buffer_type = MYSQL_TYPE_LONG;
        bindParameter.buffer = &value;
        bindParameter.is_unsigned = 0;
        bindParameter.is_null = 0;
        bindParameter.length = 0;

        Binder<N + 1, Tail...> b;
        b.bind(bindParameters);
    }
};


#endif  // BINDER_HPP_
