#ifndef INPUTBINDER_HPP_
#define INPUTBINDER_HPP_

#include <cstdint>
#include <cstring>
#include <mysql/mysql.h>

#include <string>
#include <vector>

/**
 * Binds the input parameters to the query.
 */
template <typename... Args>
void bindInputs(
    std::vector<MYSQL_BIND>* inputBindParameters,
    const Args&... args);

namespace InputBinderPrivate {

// C++11 doesn't allow for partial template specialization of variadic
// templated functions, but it does of classes, so wrap this function in a
// class
template <size_t N, typename... Args>
struct InputBinder {
    static void bind(std::vector<MYSQL_BIND>* const) {}
};


template <size_t N, typename Head, typename... Tail>
struct InputBinder<N, Head, Tail...> {
    static void bind(
        std::vector<MYSQL_BIND>* const,
        const Head&,
        const Tail&...
    ) {
        // C++ guarantees that the sizeof any parameter pack >= 0, so this will
        // always give a compile time error.
        static_assert(
            sizeof...(Tail) < 0,
            "All types need to have partial template specialized instances"
            " defined for them, but one is missing for type Head.");
    }
};


// ************************************************
// Partial template specialization for char pointer
// ************************************************
template <size_t N, typename... Tail>
struct InputBinder<N, char*, Tail...> {
    static void bind(
        std::vector<MYSQL_BIND>* const bindParameters,
        const char* const& value,
        const Tail&... tail
    ) {
        // Set up the bind parameters
        MYSQL_BIND& bindParameter = bindParameters->at(N);

        bindParameter.buffer_type = MYSQL_TYPE_STRING;
        bindParameter.buffer = const_cast<void*>(
            static_cast<const void*>(value));
        bindParameter.buffer_length = std::strlen(value);
        bindParameter.length = &bindParameter.buffer_length;
        bindParameter.is_unsigned = 0;
        bindParameter.is_null = 0;

        InputBinder<N + 1, Tail...>::bind(bindParameters, tail...);
    }
};
template <size_t N, typename... Tail>
struct InputBinder<N, const char*, Tail...> {
    static void bind(
        std::vector<MYSQL_BIND>* const bindParameters,
        const char* const& value,
        const Tail&... tail
    ) {
        InputBinder<N, char*, Tail...>::bind(
            bindParameters,
            const_cast<char*>(value),
            tail...);
    }
};


// ******************************************
// Partial template specialization for string
// ******************************************
template <size_t N, typename... Tail>
struct InputBinder<N, std::string, Tail...> {
    static void bind(
        std::vector<MYSQL_BIND>* const bindParameters,
        const std::string& value,
        const Tail&... tail
    ) {
        // Set up the bind parameters
        MYSQL_BIND& bindParameter = bindParameters->at(N);

        bindParameter.buffer_type = MYSQL_TYPE_STRING;
        bindParameter.buffer = const_cast<void*>(
            static_cast<const void*>(value.c_str()));
        bindParameter.buffer_length = value.length();
        bindParameter.length = &bindParameter.buffer_length;
        bindParameter.is_unsigned = 0;
        bindParameter.is_null = 0;

        InputBinder<N + 1, Tail...>::bind(bindParameters, tail...);
    }
};


#ifndef INPUT_BINDER_INTEGRAL_TYPE_SPECIALIZATION
#define INPUT_BINDER_INTEGRAL_TYPE_SPECIALIZATION(type, mysqlType, isUnsigned) \
template <size_t N, typename... Tail> \
struct InputBinder<N, type, Tail...> { \
    static void bind( \
        std::vector<MYSQL_BIND>* const bindParameters, \
        const type& value, \
        const Tail&... tail \
    ) { \
        /* Set up the bind parameters */ \
        MYSQL_BIND& bindParameter = bindParameters->at(N); \
        bindParameter.buffer_type = mysqlType; \
        bindParameter.buffer = const_cast<void*>( \
            static_cast<const void*>(&value)); \
        bindParameter.is_unsigned = isUnsigned; \
        bindParameter.is_null = 0; \
        InputBinder<N + 1, Tail...>::bind(bindParameters, tail...); \
    } \
};
#endif

INPUT_BINDER_INTEGRAL_TYPE_SPECIALIZATION(int8_t,   MYSQL_TYPE_TINY,     0)
INPUT_BINDER_INTEGRAL_TYPE_SPECIALIZATION(uint8_t,  MYSQL_TYPE_TINY,     1)
INPUT_BINDER_INTEGRAL_TYPE_SPECIALIZATION(int16_t,  MYSQL_TYPE_SHORT,    0)
INPUT_BINDER_INTEGRAL_TYPE_SPECIALIZATION(uint16_t, MYSQL_TYPE_SHORT,    1)
INPUT_BINDER_INTEGRAL_TYPE_SPECIALIZATION(int32_t,  MYSQL_TYPE_LONG,     0)
INPUT_BINDER_INTEGRAL_TYPE_SPECIALIZATION(uint32_t, MYSQL_TYPE_LONG,     1)
INPUT_BINDER_INTEGRAL_TYPE_SPECIALIZATION(int64_t,  MYSQL_TYPE_LONGLONG, 0)
INPUT_BINDER_INTEGRAL_TYPE_SPECIALIZATION(uint64_t, MYSQL_TYPE_LONGLONG, 1)


#ifndef INPUT_BINDER_FLOATING_TYPE_SPECIALIZATION
#define INPUT_BINDER_FLOATING_TYPE_SPECIALIZATION(type, mysqlType, size) \
template <size_t N, typename... Tail> \
struct InputBinder<N, type, Tail...> { \
    static void bind( \
        std::vector<MYSQL_BIND>* const bindParameters, \
        const type& value, \
        const Tail&... tail \
    ) { \
        /* MySQL expects specific sizes for floating point types */ \
        static_assert(size == sizeof(type), "Unexpected floating point size"); \
        /* Set up the bind parameters */ \
        MYSQL_BIND& bindParameter = bindParameters->at(N); \
        bindParameter.buffer_type = mysqlType; \
        bindParameter.buffer = const_cast<void*>( \
            static_cast<const void*>(&value)); \
        bindParameter.is_null = 0; \
        InputBinder<N + 1, Tail...>::bind(bindParameters, tail...); \
    } \
};
#endif

INPUT_BINDER_FLOATING_TYPE_SPECIALIZATION(float, MYSQL_TYPE_FLOAT, 4)
INPUT_BINDER_FLOATING_TYPE_SPECIALIZATION(double, MYSQL_TYPE_DOUBLE, 8)

}  // namespace InputBinderPrivate


template <typename... Args>
void bindInputs(
    std::vector<MYSQL_BIND>* const inputBindParameters,
    const Args&... args
) {
    InputBinderPrivate::InputBinder<0, Args...>::bind(
        inputBindParameters,
        args...);
}

#endif  // INPUTBINDER_HPP_
