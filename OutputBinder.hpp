#ifndef OUTPUTBINDER_HPP_
#define OUTPUTBINDER_HPP_

#include <cstdint>
#include <cstring>
#include <mysql/mysql.h>

#include <string>
#include <tuple>
#include <vector>

#include "MySqlException.hpp"


template <typename... Args>
class OutputBinder
{
public:
    OutputBinder(MYSQL_STMT* const statement);

    // This should just be combined into the constructor
    void setResults(std::vector<std::tuple<Args...>>* results);

#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4)
    // Deleted and defaulted constructors are supported in GCC 4.4
    ~OutputBinder() = default;
    OutputBinder(const OutputBinder& rhs) = delete;
    OutputBinder& operator=(const OutputBinder& rhs) = delete;
#endif

private:
    static void setTuple(
        std::tuple<Args...>* tuple,
        const std::vector<MYSQL_BIND>& bindParameters
    );
    static void setBind(
        const std::tuple<Args...>& tuple,
        std::vector<MYSQL_BIND>* const outputParameters,
        std::vector<std::vector<char>>* const buffers
    );

    MYSQL_STMT* const statement_;

#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 4)
    // Hidden methods
    OutputBinder(const OutputBinder& rhs);
    OutputBinder& operator=(const OutputBinder& rhs);
#endif
};


namespace OutputBinderNamespace {
    template<int I> struct int_{};  // Compile-time counter
}
template<typename Tuple, int I>
static void setTupleElements(
    Tuple* const tuple,
    const std::vector<MYSQL_BIND>& bindParameters,
    typename OutputBinderNamespace::int_<I>
);
template<typename Tuple>
static void setTupleElements(
    Tuple* const tuple,
    const std::vector<MYSQL_BIND>& bindParameters,
    typename OutputBinderNamespace::int_<-1>
);
// C++11 doesn't allow for partial template specialization of variadic
// templated functions, but it does of classes, so wrap this function in a
// class
template <typename T>
class OutputBinderElementSetter
{
public:
    /**
     * Default setter for non-specialized types using Boost lexical_cast.
     */
    static void setElement(T* const value, const MYSQL_BIND& bind)
    {
        *value = boost::lexical_cast<T>(static_cast<char*>(bind.buffer));
    }
};


template<typename Tuple, int I>
static void setBindElements(
    const Tuple& tuple,  // We only need this so we can access the element types
    std::vector<MYSQL_BIND>* const bindParameters,
    std::vector<std::vector<char>>* const buffers,
    typename OutputBinderNamespace::int_<I>
);
template<typename Tuple>
static void setBindElements(
    const Tuple& tuple,  // We only need this so we can access the element types
    std::vector<MYSQL_BIND>* const,
    std::vector<std::vector<char>>* const,
    typename OutputBinderNamespace::int_<-1>
);
// C++11 doesn't allow for partial template specialization of variadic
// templated functions, but it does of classes, so wrap this function in a
// class
template <typename T>
class OutputBinderParameterSetter
{
public:
    /**
     * Default setter for non-specialized types using Boost lexical_cast. If
     * the type doesn't have a specialization, just set it to the string type.
     * MySQL will convert the value to a string and we'll use Boost
     * lexical_cast to convert it back later.
     */
    static void setParameter(MYSQL_BIND* bind, std::vector<char>* buffer)
    {
        bind->buffer_type = MYSQL_TYPE_STRING;
        buffer->resize(20);  // This seems like a reasonable default
        bind->buffer = &(buffer->at(0));
        bind->is_null = 0;
        bind->buffer_length = buffer->size();
    }
};


template <typename... Args>
OutputBinder<Args...>::OutputBinder(MYSQL_STMT* const statement)
    : statement_(statement)
{
}


template <typename... Args>
void OutputBinder<Args...>::setResults(
    std::vector<std::tuple<Args...>>* results
)
{
    // Bind the output parameters
    // Check that the sizes match
    const size_t fieldCount = mysql_stmt_field_count(statement_);
    if (fieldCount != sizeof...(Args))
    {
        mysql_stmt_close(statement_);
        std::string errorMessage(
            "Incorrect number of output parameters; query required "
        );
        errorMessage += boost::lexical_cast<std::string>(fieldCount);
        errorMessage += " but ";
        errorMessage += boost::lexical_cast<std::string>(sizeof...(Args));
        errorMessage += " parameters were provided.";
        throw MySqlException(errorMessage);
    }

    std::vector<MYSQL_BIND> parameters;
    parameters.resize(fieldCount);
    std::vector<std::vector<char>> buffers;
    buffers.resize(fieldCount);
    std::vector<uint64_t> lengths;
    lengths.resize(fieldCount);

    // setBind needs to know the type of the tuples, and it does this by
    // taking an example tuple, so just create a dummy
    // TODO(bskari|2013-03-17) There has to be a better way than this
    std::tuple<Args...> unused;
    setBind(unused, &parameters, &buffers);

    for (size_t i = 0; i < buffers.size(); ++i)
    {
        // This doesn't need to be set on every type, but it won't hurt
        // anything, and it will make the OutputBinderParameterSetter
        // specializations simpler
        parameters.at(i).length = &lengths.at(i);
    }
    if (0 != mysql_stmt_bind_result(statement_, &parameters.at(0)))
    {
        MySqlException mse(mysql_stmt_error(statement_));
        mysql_stmt_close(statement_);
        throw mse;
    }

    if (0 != mysql_stmt_execute(statement_))
    {
        MySqlException mse(mysql_stmt_error(statement_));
        mysql_stmt_close(statement_);
        throw mse;
    }

    int fetchStatus = mysql_stmt_fetch(statement_);
    while (0 == fetchStatus || MYSQL_DATA_TRUNCATED == fetchStatus)
    {
        if (MYSQL_DATA_TRUNCATED == fetchStatus)
        {
            // TODO(bskari|2013-03-17) Rebind to a larger buffer and refetch
            fetchStatus = mysql_stmt_fetch(statement_);
            continue;
        }

        std::tuple<Args...> rowTuple;
        try
        {
            setTuple(&rowTuple, parameters);
        }
        catch (...)
        {
            mysql_stmt_close(statement_);
            throw;
        }

        results->push_back(rowTuple);
        fetchStatus = mysql_stmt_fetch(statement_);
    }

    switch (fetchStatus)
    {
        case MYSQL_NO_DATA:
            // No problem! All rows fetched.
            break;
        case 1: // Error occurred
        {
            MySqlException mse(mysql_stmt_error(statement_));
            mysql_stmt_close(statement_);
            throw mse;
        }
        default:
        {
            assert(false && "Unknown error code from mysql_stmt_fetch");
            MySqlException mse(mysql_stmt_error(statement_));
            mysql_stmt_close(statement_);
            throw mse;
        }
    }
}


template <typename... Args>
void OutputBinder<Args...>::setTuple(
    std::tuple<Args...>* tuple,
    const std::vector<MYSQL_BIND>& outputParameters
)
{
    setTupleElements(
        tuple,
        outputParameters,
        OutputBinderNamespace::int_<sizeof...(Args) - 1>()
    );
}


template <typename... Args>
void OutputBinder<Args...>::setBind(
    const std::tuple<Args...>& tuple,
    std::vector<MYSQL_BIND>* const outputParameters,
    std::vector<std::vector<char>>* const buffers
)
{
    setBindElements(
        tuple,
        outputParameters,
        buffers,
        OutputBinderNamespace::int_<sizeof...(Args) - 1>()
    );
}


template<typename Tuple, int I>
void setTupleElements(
    Tuple* const tuple,
    const std::vector<MYSQL_BIND>& outputParameters,
    typename OutputBinderNamespace::int_<I>
)
{
    OutputBinderElementSetter<
        typename std::tuple_element<I, Tuple>::type
    > setter;
    setter.setElement(&(std::get<I>(*tuple)), outputParameters.at(I));
    setTupleElements(
        tuple,
        outputParameters,
        OutputBinderNamespace::int_<I - 1>()
    );
}


template<typename Tuple>
void setTupleElements(
    Tuple* const,
    const std::vector<MYSQL_BIND>&,
    typename OutputBinderNamespace::int_<-1>
)
{
}


template<typename Tuple, int I>
static void setBindElements(
    const Tuple& tuple,  // We only need this so we can access the element types
    std::vector<MYSQL_BIND>* const bindParameters,
    std::vector<std::vector<char>>* const buffers,
    typename OutputBinderNamespace::int_<I>
)
{
    OutputBinderParameterSetter<
        typename std::tuple_element<I, Tuple>::type
    > setter;
    setter.setParameter(&bindParameters->at(I), &buffers->at(I));
    setBindElements(
        tuple,
        bindParameters,
        buffers,
        typename OutputBinderNamespace::int_<I - 1>()
    );
}


template<typename Tuple>
static void setBindElements(
    const Tuple&,  // We only need this so we can access the element types
    std::vector<MYSQL_BIND>* const,
    std::vector<std::vector<char>>* const,
    typename OutputBinderNamespace::int_<-1>
)
{
}


// ****************************************************
// Partial template specializations for element setters
// ****************************************************
#ifndef OUTPUT_BINDER_ELEMENT_SETTER_SPECIALIZATION
#define OUTPUT_BINDER_ELEMENT_SETTER_SPECIALIZATION(type) \
template <> \
class OutputBinderElementSetter<type> \
{ \
public: \
    void setElement( \
        type* const value, \
        const MYSQL_BIND& bind \
    ) \
    { \
        *value = *static_cast<const decltype(value)>(bind.buffer); \
    } \
};
#endif
OUTPUT_BINDER_ELEMENT_SETTER_SPECIALIZATION(int8_t)
OUTPUT_BINDER_ELEMENT_SETTER_SPECIALIZATION(uint8_t)
OUTPUT_BINDER_ELEMENT_SETTER_SPECIALIZATION(int16_t)
OUTPUT_BINDER_ELEMENT_SETTER_SPECIALIZATION(uint16_t)
OUTPUT_BINDER_ELEMENT_SETTER_SPECIALIZATION(int32_t)
OUTPUT_BINDER_ELEMENT_SETTER_SPECIALIZATION(uint32_t)
OUTPUT_BINDER_ELEMENT_SETTER_SPECIALIZATION(int64_t)
OUTPUT_BINDER_ELEMENT_SETTER_SPECIALIZATION(uint64_t)


// *************************************************
// Partial template specializations for bind setters
// *************************************************
#ifndef OUTPUT_BINDER_PARAMETER_SETTER_SPECIALIZATION
#define OUTPUT_BINDER_PARAMETER_SETTER_SPECIALIZATION(type, mysqlType, isUnsigned) \
template <> \
struct OutputBinderParameterSetter<type> \
{ \
public: \
    static void setParameter(MYSQL_BIND* bind, std::vector<char>* buffer) \
    { \
        bind->buffer_type = mysqlType; \
        buffer->resize(sizeof(type)); \
        bind->buffer = &(buffer->at(0)); \
        bind->is_null = 0; \
        bind->is_unsigned = isUnsigned; \
    } \
};
#endif
OUTPUT_BINDER_PARAMETER_SETTER_SPECIALIZATION(int8_t, MYSQL_TYPE_TINY, 0)
OUTPUT_BINDER_PARAMETER_SETTER_SPECIALIZATION(uint8_t, MYSQL_TYPE_TINY, 1)
OUTPUT_BINDER_PARAMETER_SETTER_SPECIALIZATION(int16_t, MYSQL_TYPE_TINY, 0)
OUTPUT_BINDER_PARAMETER_SETTER_SPECIALIZATION(uint16_t, MYSQL_TYPE_TINY, 1)
OUTPUT_BINDER_PARAMETER_SETTER_SPECIALIZATION(int32_t, MYSQL_TYPE_TINY, 0)
OUTPUT_BINDER_PARAMETER_SETTER_SPECIALIZATION(uint32_t, MYSQL_TYPE_TINY, 1)
OUTPUT_BINDER_PARAMETER_SETTER_SPECIALIZATION(int64_t, MYSQL_TYPE_TINY, 0)
OUTPUT_BINDER_PARAMETER_SETTER_SPECIALIZATION(uint64_t, MYSQL_TYPE_TINY, 1)


#endif  // OUTPUTBINDER_HPP_
