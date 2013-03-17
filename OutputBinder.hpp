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
        MySqlException mse(statement_);
        mysql_stmt_close(statement_);
        throw mse;
    }

    std::vector<MYSQL_BIND> outputBindParameters;
    outputBindParameters.resize(fieldCount);

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
            setTuple(&rowTuple, outputBindParameters);
        }
        catch (...)
        {
            mysql_stmt_close(statement_);
            throw;
        }

        results->push_back(rowTuple);
        fetchStatus = mysql_stmt_fetch(statement_);
        std::cout << "Fetch status " << fetchStatus << std::endl;
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
    // TODO(bskari|2013-03-17) Set this correctly and do it recursively for
    // each element in the tuple
    std::get<0>(*tuple) = 0;
}


#endif  // OUTPUTBINDER_HPP_
