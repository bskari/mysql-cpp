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
    void setTuple(
        std::tuple<Args...>* tuple,
        const std::vector<MYSQL_BIND>& bindParameters
    );

    MYSQL_STMT* const statement_;

#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 4)
    // Hidden methods
    OutputBinder(const OutputBinder& rhs);
    OutputBinder& operator=(const OutputBinder& rhs);
#endif
};


namespace OutputBinderNamespace {
    template<std::size_t I> struct int_{};  // Compile-time counter
}
template<typename Tuple, size_t I>
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
    std::vector<std::vector<char>> buffers(
        fieldCount,
        std::vector<char>(' ', 20)
    );
    std::vector<uint64_t> lengths;
    lengths.resize(fieldCount);

    // So, normally you would just bind each type to a buffer specific to that
    // type, e.g. &int for INTs and char* for VARCHAR, but if someone gives me
    // a tuple with a std::string, I don't really know how to use that as a
    // buffer, so for now I'll just use char* buffers and save everything and
    // boost::lexical_cast to set the values.
    for (size_t i = 0; i < buffers.size(); ++i)
    {
        MYSQL_BIND& bind = parameters.at(i);
        bind.buffer_type = MYSQL_TYPE_STRING;
        bind.buffer = &(buffers.at(i).at(0));
        bind.is_null = 0;
        bind.buffer_length = buffers.at(i).size();
        bind.length = &lengths.at(i);
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
    const std::vector<MYSQL_BIND>& bindParameters
)
{
    setTupleElements(
        tuple,
        bindParameters,
        OutputBinderNamespace::int_<sizeof...(Args) - 1>()
    );
}


template<typename Tuple, size_t I>
void setTupleElements(
    Tuple* const tuple,
    const std::vector<MYSQL_BIND>& bindParameters,
    typename OutputBinderNamespace::int_<I>
)
{
    std::get<I>(*tuple) =
        boost::lexical_cast<typename std::tuple_element<I, Tuple>::type>(
            static_cast<char*>(bindParameters.at(I).buffer)
        );
    setTupleElements(
        tuple,
        bindParameters,
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


#endif  // OUTPUTBINDER_HPP_
