#ifndef MYSQL_HPP_
#define MYSQL_HPP_

#include <cassert>
#include <cstdint>
#include <cstring>
#include <mysql/mysql.h>

#include <boost/lexical_cast.hpp>
#include <string>
#include <tuple>
#include <typeinfo>
#include <utility>
#include <vector>

#include "InputBinder.hpp"
#include "MySqlException.hpp"

#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 6)
#define nullptr 0
#endif


class MySql
{
public:
    MySql(
        const char* const hostname,
        const char* const username,
        const char* const password,
        const char* const database,
        const uint16_t port = 3306
    );

    // Delegating constructors are supported in GCC 4.7
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7)
    MySql(
        const char* hostname,
        const char* username,
        const char* password,
        const uint16_t port = 3306
    );
#endif
    ~MySql();

#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4)
    MySql(const MySql& rhs) = delete;
    MySql& operator=(const MySql& rhs) = delete;
#endif

    /**
     * Normal query. Results are stored in the given vector.
     * @param query The query to run.
     * @param results A vector of tuples to store the results in.
     * @param args Arguments to bind to the query.
     */
    template<typename... Args>
    void runQuery(
        std::vector<std::tuple<Args...> >* const results,
        const char* const query,
        Args... args
    );

    /**
     * Command that doesn't return results, like "USE yelp" or
     * "INSERT INTO user VALUES ('Brandon', 28)".
     * @param query The query to run.
     * @param args Arguments to bind to the query.
     * @return The number of affected rows.
     */
    template<typename... Args>
    my_ulonglong runCommand(
        const char* const command,
        Args... args
    );
    my_ulonglong runCommand(const char* const command);

private:
    template<typename... Args>
    void extractResults(MYSQL_RES* const result, Args&&... args) const;

    template<typename T, typename... Args>
    void extractRow(
        MYSQL_ROW rowValues,
        const size_t currentField,
        const size_t totalFields,
        T* const value,
        Args... args
    ) const;
    void extractRow(
        MYSQL_ROW rowValues,
        const size_t currentField,
        const size_t totalFields
    ) const;

    template<std::size_t I> struct int_{};  // Compile-time counter
    template<typename Tuple, size_t I>
    static void setTupleElements(MYSQL_ROW row, Tuple* const returnValue, int_<I>);
    template<typename Tuple>
    static void setTupleElements(MYSQL_ROW, Tuple* const returnValue, int_<0>);
    template<typename... Args>
    void setTuple(MYSQL_ROW, std::tuple<Args...>* const t);

    MYSQL* connection_;
    std::vector<MYSQL_BIND> bindParameters_;

#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 4)
    // Hidden methods
    MySql(const MySql& rhs);
    MySql& operator=(const MySql& rhs);
#endif
};


template<typename... Args>
my_ulonglong MySql::runCommand(
    const char* const command,
    Args... args
)
{
    MYSQL_STMT* const statement = mysql_stmt_init(connection_);
    if (nullptr == statement)
    {
        throw MySqlException(connection_);
    }

    const size_t length = ::strlen(command);
    const int status = mysql_stmt_prepare(statement, command, length);
    if (0 != status)
    {
        MySqlException mse(statement);
        mysql_stmt_close(statement);
        throw mse;
    }

    const size_t parameterCount = mysql_stmt_param_count(statement);
    if (sizeof...(args) != parameterCount)
    {
        mysql_stmt_close(statement);
        std::string errorMessage("Incorrect number of parameters; query required ");
        errorMessage += boost::lexical_cast<std::string>(parameterCount);
        errorMessage += " but ";
        errorMessage += boost::lexical_cast<std::string>(sizeof...(args));
        errorMessage += " parameters were provided.";
        throw MySqlException(errorMessage);
    }

    bindParameters_.resize(parameterCount);
    InputBinder<0, Args...> binder;
    binder.bind(&bindParameters_, args...);
    if (0 != mysql_stmt_bind_param(statement, &bindParameters_[0]))
    {
        MySqlException mse(statement);
        mysql_stmt_close(statement);
        throw mse;
    }

    if (0 != mysql_stmt_execute(statement))
    {
        MySqlException mse(statement);
        mysql_stmt_close(statement);
        throw mse;
    }

    // If the user ran a SELECT statement or something else, at least warn them
    const my_ulonglong affectedRows = mysql_stmt_affected_rows(statement);
    if (((my_ulonglong)(-1)) == affectedRows)
    {
        MySqlException mse(statement);
        mysql_stmt_close(statement);
        throw mse;
    }

    mysql_stmt_close(statement);
    return affectedRows;
}


template<typename... Args>
void MySql::runQuery(
    std::vector<std::tuple<Args...> >* const results,
    const char* const query,
    Args... args
)
{
    assert(nullptr != results);
    MYSQL_STMT* const statement = mysql_stmt_init(connection_);

    const size_t length = ::strlen(query);
    if (0 != mysql_stmt_prepare(statement, query, length))
    {
        throw MySqlException(connection_);
    }

    const size_t parameterCount = mysql_stmt_param_count(statement);
    if (sizeof...(args) != parameterCount)
    {
        mysql_stmt_close(statement);

        std::string errorMessage("Incorrect number of parameters; query required ");
        errorMessage += boost::lexical_cast<std::string>(parameterCount);
        errorMessage += " but ";
        errorMessage += boost::lexical_cast<std::string>(sizeof...(args));
        errorMessage += " parameters were provided.";
        throw MySqlException(errorMessage);
    }

    bindParameters_.resize(parameterCount);
    InputBinder<0, Args...> binder;
    binder.bind(&bindParameters_, args...);
    if (0 != mysql_stmt_bind_param(statement, &bindParameters_[0]))
    {
        mysql_stmt_close(statement);
        throw MySqlException(connection_);
    }

    if (0 != mysql_stmt_execute(statement))
    {
        mysql_stmt_close(statement);
        throw MySqlException(connection_);
    }

    // Check that the sizes match
    MYSQL_RES* const result = mysql_stmt_result_metadata(statement);
    const size_t numFields = mysql_num_fields(result);
    if (numFields != sizeof...(Args))
    {
        mysql_free_result(result);
        std::string errorMessage("Incorrect number of columns; expected ");
        errorMessage += boost::lexical_cast<std::string>(sizeof...(Args));
        errorMessage += " but query returned ";
        errorMessage += boost::lexical_cast<std::string>(numFields);
        throw MySqlException(errorMessage);
    }

    // Bind the input and output parameters

    // Parse and save all of the rows
    int fetchStatus = mysql_stmt_fetch(statement);
    while (0 == fetchStatus)
    {
        /*
        std::tuple<Args...> rowTuple;
        try
        {
            setTuple(row, &rowTuple);
        }
        catch (...)
        {
            mysql_free_result(result);
            throw;
        }

        results->push_back(rowTuple);
        row = mysql_fetch_row(result);
        fetchStatus = mysql_stmt_fetch(statement);
        */
    }

    switch (fetchStatus)
    {
        case MYSQL_NO_DATA:
            // No problem! All rows fetched.
            break;
        case MYSQL_DATA_TRUNCATED:
            // TODO(bskari|2013-03-16) Handle truncated data
            break;
        case 1: // Error occurred
            mysql_stmt_close(statement);
            mysql_free_result(result);
            throw MySqlException(mysql_stmt_error(statement));
        default:
            mysql_stmt_close(statement);
            mysql_free_result(result);
            assert(false && "Unknown error code from mysql_stmt_fetch");
            throw MySqlException("Unknown error code from mysql_stmt_fetch");
    }

    mysql_stmt_close(statement);
    mysql_free_result(result);
}


template<typename... Args>
void MySql::extractResults(MYSQL_RES* const result, Args&&... args) const
{
    MYSQL_ROW row = mysql_fetch_row(result);
    if (nullptr == row)
    {
        mysql_free_result(result);
        throw MySqlException(connection_);
    }

    try
    {
        const size_t numFields = mysql_num_fields(result);
        extractRow(row, 0, numFields, std::forward<Args>(args)...);
    }
    catch (...)
    {
        mysql_free_result(result);
        throw;
    }
}


template<typename T, typename... Args>
void MySql::extractRow(
    MYSQL_ROW rowValues,
    const size_t currentField,
    const size_t totalFields,
    T* const value,
    Args... args
) const
{
    assert(nullptr != value);

    if (currentField >= totalFields)
    {
        throw MySqlException("Too few columns returned by query");
    }
    try
    {
        *value = boost::lexical_cast<T>(rowValues[currentField]);
    }
    catch (...)
    {
        std::string errorMessage("Unable to cast \"");
        errorMessage += rowValues[currentField];
        errorMessage += "\" to type ";
        errorMessage += typeid(*value).name();
        throw MySqlException(errorMessage);
    }

    extractRow(rowValues, currentField + 1, totalFields, args...);
}


template<typename Tuple, size_t I>
void MySql::setTupleElements(MYSQL_ROW row, Tuple* const tuple, int_<I>)
{
    assert(nullptr != tuple);

    try
    {
        std::get<I>(*tuple) = boost::lexical_cast<
            typename std::tuple_element<I, Tuple>::type
        >(row[I]);
    }
    catch (...)
    {
        std::string errorMessage("Unable to cast \"");
        errorMessage += row[I];
        errorMessage += "\" to type ";
        errorMessage += typeid(typename std::tuple_element<I, Tuple>::type).name();
        throw MySqlException(errorMessage);
    }
    // Recursively set the rest of the elements
    setTupleElements(row, tuple, int_<I - 1>());
}


template<typename Tuple>
void MySql::setTupleElements(MYSQL_ROW row, Tuple* const tuple, int_<0>)
{
    assert(nullptr != tuple);

    try
    {
        std::get<0>(*tuple) = boost::lexical_cast<
            typename std::tuple_element<0, Tuple>::type
        >(row[0]);
    }
    catch (...)
    {
        std::string errorMessage("Unable to cast \"");
        errorMessage += row[0];
        errorMessage += "\" to type ";
        errorMessage += typeid(typename std::tuple_element<0, Tuple>::type).name();
        throw MySqlException(errorMessage);
    }
}


template<typename... Args>
void MySql::setTuple(MYSQL_ROW row, std::tuple<Args...>* const t)
{
    setTupleElements(row, t, int_<sizeof...(Args) - 1>());
}


#endif  // MYSQL_HPP_
