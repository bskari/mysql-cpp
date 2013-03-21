#ifndef OUTPUTBINDER_HPP_
#define OUTPUTBINDER_HPP_

#include "MySqlException.hpp"

#include <boost/lexical_cast.hpp>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mysql/mysql.h>
#include <string>
#include <tuple>
#include <vector>


static const char NULL_VALUE_ERROR_MESSAGE[] = \
    "Null value encountered with non-shared_ptr output type";


template <typename... Args>
class OutputBinder
{
public:
    OutputBinder(MYSQL_STMT* const statement);

    // Should this just be combined into the constructor? On one hand,
    // creating an instance of this class and then not calling any methods
    // from it seems weird.
    void setResults(std::vector<std::tuple<Args...>>* const results);

#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4)
    // Deleted and defaulted constructors are supported in GCC 4.4
    ~OutputBinder() = default;
    OutputBinder(const OutputBinder& rhs) = delete;
    OutputBinder& operator=(const OutputBinder& rhs) = delete;
#endif

private:
    inline void refetchTruncatedColumns(
        std::vector<MYSQL_BIND>* const parameters,
        std::vector<std::vector<char>>* const buffers,
        std::vector<uint64_t>* const lengths
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
static void setResultTuple(
    Tuple* const tuple,
    const std::vector<MYSQL_BIND>& mysqlBindParameters,
    typename OutputBinderNamespace::int_<I>
);
template<typename Tuple>
static void setResultTuple(
    Tuple* const tuple,
    const std::vector<MYSQL_BIND>& mysqlBindParameters,
    typename OutputBinderNamespace::int_<-1>
);
// Keep this in a templated class instead of just defining function overloads
// so that we don't get unused function warnings
template <typename T>
class OutputBinderResultSetter
{
public:
    /**
     * Default setter for non-specialized types using Boost lexical_cast.
     */
    static void setResult(
        T* const value,
        const MYSQL_BIND& bind
    );
};
template<typename T>
class OutputBinderResultSetter<std::shared_ptr<T>>
{
public:
    static void setResult(
        std::shared_ptr<T>* const value,
        const MYSQL_BIND& bind
    );
};
template<typename T>
class OutputBinderResultSetter<T*>
{
public:
    static void setResult(T** const, const MYSQL_BIND&);
};


template<typename Tuple, int I>
static void bindParameters(
    const Tuple& tuple,  // We only need this so we can access the element types
    std::vector<MYSQL_BIND>* const mysqlBindParameters,
    std::vector<std::vector<char>>* const buffers,
    std::vector<my_bool> const nullFlags,
    typename OutputBinderNamespace::int_<I>
);
template<typename Tuple>
static void bindParameters(
    const Tuple& tuple,  // We only need this so we can access the element types
    std::vector<MYSQL_BIND>* const,
    std::vector<std::vector<char>>* const,
    std::vector<my_bool> const,
    typename OutputBinderNamespace::int_<-1>
);
// Keep this in a templated class instead of just defining function overloads
// so that we don't get unused function warnings
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
    static void setParameter(
        MYSQL_BIND* const bind,
        std::vector<char>* const buffer,
        my_bool* const isNullFlag
    );
};
template<typename T>
class OutputBinderParameterSetter<std::shared_ptr<T>>
{
public:
    static void setParameter(
        MYSQL_BIND* const bind,
        std::vector<char>* const buffer,
        my_bool* const isNullFlag
    );
};
template<typename T>
class OutputBinderParameterSetter<T*>
{
public:
    static void setParameter(
        MYSQL_BIND* const,
        std::vector<char>* const,
        my_bool* const
    );
};


template <typename... Args>
OutputBinder<Args...>::OutputBinder(MYSQL_STMT* const statement)
    : statement_(statement)
{
}


template <typename... Args>
void OutputBinder<Args...>::setResults(
    std::vector<std::tuple<Args...>>* const results
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
        errorMessage += " parameters were provided";
        throw MySqlException(errorMessage);
    }

    std::vector<MYSQL_BIND> parameters;
    parameters.resize(fieldCount);
    // TODO(bskari|2013-03-18) Is this necessary, or will the defaulted C++
    // constructor for MYSQL_BIND handle it?
    memset(&parameters.at(0), 0, sizeof(parameters.at(0)) * parameters.size());
    std::vector<std::vector<char>> buffers;
    buffers.resize(fieldCount);
    std::vector<uint64_t> lengths;
    lengths.resize(fieldCount);
    std::vector<my_bool> nullFlags;
    nullFlags.resize(fieldCount);

    // bindParameters needs to know the type of the tuples, and it does this by
    // taking an example tuple, so just create a dummy
    // TODO(bskari|2013-03-17) There has to be a better way than this
    std::tuple<Args...> unused;
    bindParameters(
        unused,
        &parameters,
        &buffers,
        &nullFlags,
        OutputBinderNamespace::int_<sizeof...(Args) - 1>()
    );

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
            refetchTruncatedColumns(&parameters, &buffers, &lengths);
        }

        std::tuple<Args...> rowTuple;
        try
        {
            setResultTuple(
                &rowTuple,
                parameters,
                OutputBinderNamespace::int_<sizeof...(Args) - 1>()
            );
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
void OutputBinder<Args...>::refetchTruncatedColumns(
    std::vector<MYSQL_BIND>* const parameters,
    std::vector<std::vector<char>>* const buffers,
    std::vector<uint64_t>* const lengths
)
{
    // Find which buffers were too small, expand them and refetch
    std::vector<std::tuple<size_t, size_t>> truncatedColumns;
    for (size_t i = 0; i < lengths->size(); ++i)
    {
        std::vector<char>& buffer = buffers->at(i);
        const size_t untruncatedLength = lengths->at(i);
        if (untruncatedLength > buffer.size())
        {
            // Only refetch the part that we didn't get the first time
            const size_t alreadyRetrieved = buffer.size();
            truncatedColumns.push_back(
                std::tuple<size_t, size_t>(i, alreadyRetrieved)
            );
            buffer.resize(untruncatedLength + 1);
            MYSQL_BIND& bind = parameters->at(i);
            bind.buffer = &buffer.at(alreadyRetrieved);
            bind.buffer_length = buffer.size() - alreadyRetrieved - 1;
        }
    }

    // I'm not sure why, but I occasionally get the truncated status
    // code when nothing was truncated... so just break out?
    if (truncatedColumns.empty())
    {
        // No truncations!
        return;
    }

    // Refetch only the data that were truncated
    const std::vector<std::tuple<size_t, size_t>>::const_iterator end(
        truncatedColumns.end()
    );
    for (
        std::vector<std::tuple<size_t, size_t>>::const_iterator i(
            truncatedColumns.begin()
        );
        i != end;
        ++i
    )
    {
        const size_t column = std::get<0>(*i);
        const size_t offset = std::get<1>(*i);
        MYSQL_BIND& parameter = parameters->at(column);
        const int status = mysql_stmt_fetch_column(
            statement_,
            &parameter,
            column,
            offset
        );
        if (0 != status)
        {
            MySqlException mse(mysql_stmt_error(statement_));
            mysql_stmt_close(statement_);
            throw mse;
        }

        // Now, for subsequent fetches, we need to reset the buffers
        std::vector<char>& buffer = buffers->at(column);
        parameter.buffer = &buffer.at(0);
        parameter.buffer_length = buffer.size();
    }

    // If we've changed the buffers, we need to rebind
    if (0 != mysql_stmt_bind_result(statement_, &parameters->at(0)))
    {
        MySqlException mse(mysql_stmt_error(statement_));
        mysql_stmt_close(statement_);
        throw mse;
    }
}


template<typename Tuple, int I>
void setResultTuple(
    Tuple* const tuple,
    const std::vector<MYSQL_BIND>& outputParameters,
    typename OutputBinderNamespace::int_<I>
)
{
    OutputBinderResultSetter<
        typename std::tuple_element<I, Tuple>::type
    > setter;
    setter.setResult(&(std::get<I>(*tuple)), outputParameters.at(I));
    setResultTuple(
        tuple,
        outputParameters,
        OutputBinderNamespace::int_<I - 1>()
    );
}


template<typename Tuple>
void setResultTuple(
    Tuple* const,
    const std::vector<MYSQL_BIND>&,
    typename OutputBinderNamespace::int_<-1>
)
{
}


template<typename Tuple, int I>
static void bindParameters(
    const Tuple& tuple,  // We only need this so we can access the element types
    std::vector<MYSQL_BIND>* const mysqlBindParameters,
    std::vector<std::vector<char>>* const buffers,
    std::vector<my_bool>* const nullFlags,
    typename OutputBinderNamespace::int_<I>
)
{
    OutputBinderParameterSetter<
        typename std::tuple_element<I, Tuple>::type
    > setter;
    setter.setParameter(
        &mysqlBindParameters->at(I),
        &buffers->at(I),
        &nullFlags->at(I)
    );
    bindParameters(
        tuple,
        mysqlBindParameters,
        buffers,
        nullFlags,
        typename OutputBinderNamespace::int_<I - 1>()
    );
}


template<typename Tuple>
static void bindParameters(
    const Tuple&,  // We only need this so we can access the element types
    std::vector<MYSQL_BIND>* const,
    std::vector<std::vector<char>>* const,
    std::vector<my_bool>* const,
    typename OutputBinderNamespace::int_<-1>
)
{
}


template <typename T>
void OutputBinderResultSetter<T>::setResult(
    T* const value,
    const MYSQL_BIND& bind
)
{
    // Null values only be encountered with shared_ptr output variables, and
    // those are handled in a partial template specialization, so if we see
    // one here, it's unexpected and an error
    if (*bind.is_null)
    {
        throw MySqlException(NULL_VALUE_ERROR_MESSAGE);
    }
    *value = boost::lexical_cast<T>(static_cast<char*>(bind.buffer));
}
// **********************************************************
// Partial specialization for shared_ptr types for setResult
// **********************************************************
template<typename T>
void OutputBinderResultSetter<std::shared_ptr<T>>::setResult(
    std::shared_ptr<T>* const value,
    const MYSQL_BIND& bind
)
{
    if (*bind.is_null)
    {
        // Remove object (if any)
        value->reset();
    }
    else
    {
        // Fall through to the non-shared_ptr version
        // TODO(bskari|2013-03-13) We shouldn't need to allocate a new object,
        // send it to a non-shared_ptr instance of setResult, and then make a
        // copy of it. Refactor this delegation stuff out so that falling
        // through is cleaner.
        T newObject;
        OutputBinderResultSetter<T> setter;
        setter.setResult(&newObject, bind);
        *value = std::make_shared<T>(newObject);
    }
}
// *******************************************************
// Partial specialization for pointer types for setResult
// *******************************************************
template<typename T>
void OutputBinderResultSetter<T*>::setResult(T** const, const MYSQL_BIND&)
{
    static_assert(
        sizeof(T) == 0, // C++ guarantees that the sizeof any valid type != 0
        "Raw pointers are not suppoorted; use std::shared_ptr instead"
    );
}
// ***********************************
// Full specializations for setResult
// ***********************************
#ifndef OUTPUT_BINDER_ELEMENT_SETTER_SPECIALIZATION
#define OUTPUT_BINDER_ELEMENT_SETTER_SPECIALIZATION(type) \
template <> \
class OutputBinderResultSetter<type> \
{ \
public: \
    void setResult( \
        type* const value, \
        const MYSQL_BIND& bind \
    ) \
    { \
        if (*bind.is_null) \
        { \
            throw MySqlException(NULL_VALUE_ERROR_MESSAGE); \
        } \
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
OUTPUT_BINDER_ELEMENT_SETTER_SPECIALIZATION(float)
OUTPUT_BINDER_ELEMENT_SETTER_SPECIALIZATION(double)
template<>
class OutputBinderResultSetter<std::string>
{
public:
    void setResult(
        std::string* const value,
        const MYSQL_BIND& bind
    )
    {
        if (*bind.is_null)
        {
            throw MySqlException(NULL_VALUE_ERROR_MESSAGE);
        }
        // Strings have an operator= for char*, so we can skip the lexical_cast
        // and just call this directly
        *value = static_cast<const char*>(bind.buffer);
    }
};


template <typename T>
void OutputBinderParameterSetter<T>::setParameter(
    MYSQL_BIND* const bind,
    std::vector<char>* const buffer,
    my_bool* const isNullFlag
)
{
    bind->buffer_type = MYSQL_TYPE_STRING;
    if (0 == buffer->size())
    {
        // This seems like a reasonable default. If the buffer is
        // non-empty, then it's probably been expanded to fit accomodate
        // some truncated data, so don't mess with it.
        buffer->resize(20);
    }
    bind->buffer = &(buffer->at(0));
    bind->is_null = isNullFlag;
    bind->buffer_length = buffer->size();
}
// ************************************************************
// Partial specialization for shared_ptr types for setParameter
// ************************************************************
template<typename T>
void OutputBinderParameterSetter<std::shared_ptr<T>>::setParameter(
    MYSQL_BIND* const bind,
    std::vector<char>* const buffer,
    my_bool* const isNullFlag
)
{
    // Just forward to the ful specialization
    OutputBinderParameterSetter<T> setter;
    setter.setParameter(bind, buffer, isNullFlag);
}
// *********************************************************
// Partial specialization for pointer types for setParameter
// *********************************************************
template<typename T>
void OutputBinderParameterSetter<T*>::setParameter(
    MYSQL_BIND* const,
    std::vector<char>* const,
    my_bool* const
)
{
    static_assert(
        sizeof(T) == 0, // C++ guarantees that the sizeof any valid type != 0
        "Raw pointers are not suppoorted; use std::shared_ptr instead"
    );
}
// *************************************
// Full specializations for setParameter
// *************************************
#ifndef OUTPUT_BINDER_PARAMETER_SETTER_SPECIALIZATION
#define OUTPUT_BINDER_PARAMETER_SETTER_SPECIALIZATION(type, mysqlType, isUnsigned) \
template <> \
struct OutputBinderParameterSetter<type> \
{ \
public: \
    static void setParameter( \
        MYSQL_BIND* const bind, \
        std::vector<char>* const buffer, \
        my_bool* const isNullFlag \
    ) \
    { \
        bind->buffer_type = mysqlType; \
        buffer->resize(sizeof(type)); \
        bind->buffer = &(buffer->at(0)); \
        bind->is_null = isNullFlag; \
        bind->is_unsigned = isUnsigned; \
    } \
};
#endif
OUTPUT_BINDER_PARAMETER_SETTER_SPECIALIZATION(int8_t,   MYSQL_TYPE_TINY,     0)
OUTPUT_BINDER_PARAMETER_SETTER_SPECIALIZATION(uint8_t,  MYSQL_TYPE_TINY,     1)
OUTPUT_BINDER_PARAMETER_SETTER_SPECIALIZATION(int16_t,  MYSQL_TYPE_SHORT,    0)
OUTPUT_BINDER_PARAMETER_SETTER_SPECIALIZATION(uint16_t, MYSQL_TYPE_SHORT,    1)
OUTPUT_BINDER_PARAMETER_SETTER_SPECIALIZATION(int32_t,  MYSQL_TYPE_LONG,     0)
OUTPUT_BINDER_PARAMETER_SETTER_SPECIALIZATION(uint32_t, MYSQL_TYPE_LONG,     1)
OUTPUT_BINDER_PARAMETER_SETTER_SPECIALIZATION(int64_t,  MYSQL_TYPE_LONGLONG, 0)
OUTPUT_BINDER_PARAMETER_SETTER_SPECIALIZATION(uint64_t, MYSQL_TYPE_LONGLONG, 1)
OUTPUT_BINDER_PARAMETER_SETTER_SPECIALIZATION(float,    MYSQL_TYPE_FLOAT,    0)
OUTPUT_BINDER_PARAMETER_SETTER_SPECIALIZATION(double,   MYSQL_TYPE_DOUBLE,   0)


#endif  // OUTPUTBINDER_HPP_
