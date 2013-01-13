#ifndef MYSQL_HPP_
#define MYSQL_HPP_

#include <boost/lexical_cast.hpp>
#include <cstdint>
#include <mysql/mysql.h>
#include <string>
#include <tuple>
#include <typeinfo>
#include <vector>

#include "MySqlException.hpp"
class PreparedStatement;

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

    /**
     * Queries that return exactly one row.
     * @query The query to run.
     * @param args Variables to store the results from the query.
     */
    template<typename... Args>
    void fetchOne(const PreparedStatement& query, Args&&... args) const;

    /**
     * Commands that don't return results, like "USE mysql".
     * @param command The command to run.
     */
    void runCommand(const PreparedStatement& command) const;

    /**
     * Normal query. Results are stored in the given vector.
     * @param query The query to run.
     * @param results A vector of tuples to store the results in.
     */
    template<typename... Args>
    void runQuery(const PreparedStatement& query, std::vector<std::tuple<Args...>>& results);

    /**
     * Escapes a string.
     * @param str The string to escape.
     */
    std::string escape(const std::string& str) const;

private:
    MYSQL* conn;

    template<typename... Args>
    void extractResults(MYSQL_RES* const result, Args&&... args) const;

    template<typename T, typename... Args>
    void extractRow(
        MYSQL_ROW rowValues,
        const size_t currentField,
        const size_t totalFields,
        T& value,
        Args... args
    ) const;
    void extractRow(
        MYSQL_ROW rowValues,
        const size_t currentField,
        const size_t totalFields
    ) const;

    template<std::size_t I> struct int_{}; // Compile-time counter
    template<typename Tuple, size_t I>
    static void setTupleElements(MYSQL_ROW row, Tuple& returnValue, int_<I>);
    template<typename Tuple>
    static void setTupleElements(MYSQL_ROW, Tuple& returnValue, int_<0>);
    template<typename... Args>
    void setTuple(MYSQL_ROW, std::tuple<Args...>& t);

    // Hidden methods
    MySql(const MySql& rhs);
    MySql& operator=(const MySql& rhs);
};


// Work around circular dependencies
#include "PreparedStatement.hpp"


template<typename... Args>
void MySql::fetchOne(const PreparedStatement& query, Args&&... args) const
{
    if (0 != mysql_query(conn, query.c_str()))
    {
        throw MySqlException(conn);
    }

    MYSQL_RES* result = mysql_store_result(conn);
    // If result is NULL, then the query was a statement that doesn't return
    // any results, e.g. 'USE mysql'
    if (nullptr != result)
    {
        extractResults(result, std::forward<Args>(args)...);
    }
    else
    {
        // Well, the command was sent to the database anyway, so at least tell
        // the user if it succeeded or not
        const char* const errorMessage = mysql_error(conn);
        std::string exceptionMessage(
            nullptr == errorMessage
            ? "Statement succeeded"
            : errorMessage
        );
        exceptionMessage += "Arguments must be supplied to functions that return results";
        throw MySqlException(exceptionMessage);
    }
}


template<typename... Args>
void MySql::extractResults(MYSQL_RES* const result, Args&&... args) const
{
    MYSQL_ROW row = mysql_fetch_row(result);
    if (nullptr == row)
    {
        mysql_free_result(result);
        throw MySqlException(conn);
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
    T& value,
    Args... args
) const
{
    if (currentField >= totalFields)
    {
        throw MySqlException("Too few columns returned by query");
    }
    try
    {
        value = boost::lexical_cast<T>(rowValues[currentField]);
    }
    catch (...)
    {
        std::string errorMessage("Unable to cast \"");
        errorMessage += rowValues[currentField];
        errorMessage += "\" to type ";
        errorMessage += typeid(value).name();
        throw MySqlException(errorMessage);
    }

    extractRow(rowValues, currentField + 1, totalFields, args...);
}


template<typename Tuple, size_t I>
void MySql::setTupleElements(MYSQL_ROW row, Tuple& tuple, int_<I>)
{
    try
    {
        std::get<I>(tuple) = boost::lexical_cast<
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
void MySql::setTupleElements(MYSQL_ROW row, Tuple& tuple, int_<0>)
{
    try
    {
        std::get<0>(tuple) = boost::lexical_cast<
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
void MySql::setTuple(MYSQL_ROW row, std::tuple<Args...>& t)
{
    setTupleElements(row, t, int_<sizeof...(Args) - 1>());
}


template<typename... Args>
void MySql::runQuery(const PreparedStatement& query, std::vector<std::tuple<Args...>>& results)
{
    if (0 != mysql_query(conn, query.c_str()))
    {
        throw MySqlException(conn);
    }

    MYSQL_RES* result = mysql_store_result(conn);
    // If result is NULL, then the query was a statement that doesn't return
    // any results, e.g. 'USE mysql'
    if (nullptr != result)
    {
        // Check that the sizes match
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

        // Parse and save all of the rows
        MYSQL_ROW row = mysql_fetch_row(result);
        while (nullptr != row)
        {
            std::tuple<Args...> rowTuple;
            try
            {
                setTuple(row, rowTuple);
            }
            catch (...)
            {
                mysql_free_result(result);
                throw;
            }

            results.push_back(rowTuple);
            row = mysql_fetch_row(result);
        }
    }
    else
    {
        // Well, the command was sent to the database anyway, so at least tell
        // the user if it succeeded or not
        const char* const errorMessage = mysql_error(conn);
        std::string exceptionMessage(
            nullptr == errorMessage
            ? "Statement succeeded"
            : errorMessage
        );
        exceptionMessage += "Arguments must be supplied to functions that return results";
        throw MySqlException(exceptionMessage);
    }
}


#endif  // MYSQL_HPP_
