#ifndef MYSQL_HPP_
#define MYSQL_HPP_

class PreparedStatement;

#include "mysql_exception.h"

#include <boost/lexical_cast.hpp>
#include <cstdint>
#include <mysql/mysql.h>
#include <string>
#include <tuple>
#include <typeinfo>
#include <vector>


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

    MySql(
        const char* hostname,
        const char* username,
        const char* password,
        const uint16_t port = 3306
    );
    MySql(const MySql& rhs) = delete;
    MySql& operator=(const MySql& rhs) = delete;

    ~MySql();

    /**
     * Queries that return exactly one row.
     * @query The query to run.
     * @param args Variables to store the results from the query.
     */
    template<typename... Args>
    void FetchOne(const PreparedStatement& query, Args&&... args) const;

    /**
     * Commands that don't return results, like "USE mysql".
     * @param command The command to run.
     */
    void RunCommand(const PreparedStatement& command) const;

    /**
     * Normal query. Results are stored in the given vector.
     * @param query The query to run.
     * @param results A vector of tuples to store the results in.
     */
    template<typename... Args>
    void RunQuery(const PreparedStatement& query, std::vector<std::tuple<Args...>>& results);

    /**
     * Escapes a string.
     * @param Str The string to escape.
     */
    std::string Escape(const std::string& Str) const;

private:
    MYSQL* conn;

    template<typename... Args>
    void ExtractResults(MYSQL_RES* const result, Args&&... args) const;

    template<typename T, typename... Args>
    void ExtractRow(MYSQL_ROW rowValues, const size_t currentField,
        const size_t totalFields, T& value, Args... args) const;
    void ExtractRow(MYSQL_ROW rowValues, const size_t currentField,
        const size_t totalFields) const;

    template<std::size_t I> struct int_{}; // Compile-time counter
    template<typename Tuple, size_t I>
    static void SetTupleElements(MYSQL_ROW row, Tuple& returnValue, int_<I>);
    template<typename Tuple>
    static void SetTupleElements(MYSQL_ROW, Tuple& returnValue, int_<0>);
    template<typename... Args>
    void SetTuple(MYSQL_ROW, std::tuple<Args...>& t);
};


class PreparedStatement
{
public:
    template<typename... Args>
    PreparedStatement(
        const MySql& conn,
        const char* const statement,
        const Args... args
    );

    const char* CStr() const;
    const std::string& Str() const;

private:
    template<typename T, typename... Args>
    void Init(
        const MySql& conn,
        const char* const statement,
        const T& arg,
        const Args... args
    );
    void Init(const MySql& conn, const char* const statement);

    std::ostringstream prepareStream_;
    std::string preparedStatement_;

    friend std::ostream& operator<<(std::ostream& out, const PreparedStatement& rhs);
};


template<typename... Args>
void MySql::FetchOne(const PreparedStatement& query, Args&&... args) const {
  if (0 != mysql_query(conn, query.CStr())) {
    perror(query.CStr());
    throw MySqlException(conn);
  }

  MYSQL_RES* result = mysql_store_result(conn);
  // If result is NULL, then the query was a statement that doesn't return
  // any results, e.g. 'USE mysql'
  if (nullptr != result) {
    ExtractResults(result, std::forward<Args>(args)...);
  } else {
    // Well, the command was sent to the database anyway, so at least tell
    // the user if it succeeded or not
    const char* const errorMessage = mysql_error(conn);
    if ('\0' != errorMessage[0]) {
      std::string exceptionMessage(query.CStr());
      exceptionMessage += ": Arguments cannot be supplied to functions that don't return results: ";
      exceptionMessage += errorMessage;
      throw MySqlException(exceptionMessage);
    }
  }
}


template<typename... Args>
void MySql::ExtractResults(MYSQL_RES* const result, Args&&... args) const {
  MYSQL_ROW row = mysql_fetch_row(result);
  if (nullptr == row) {
    mysql_free_result(result);
    throw MySqlException(conn);
  }

  try {
    const size_t numFields = mysql_num_fields(result);
    ExtractRow(row, 0, numFields, std::forward<Args>(args)...);
  } catch (...) {
    mysql_free_result(result);
    throw;
  }
}


template<typename T, typename... Args>
void MySql::ExtractRow(
    MYSQL_ROW rowValues,
    const size_t currentField,
    const size_t totalFields,
    T& value,
    Args... args
    ) const {
  if (currentField >= totalFields) {
    throw MySqlException("Too few columns returned by query");
  }
  try {
    value = boost::lexical_cast<T>(rowValues[currentField]);
  } catch (...) {
    std::string errorMessage("Unable to cast \"");
    errorMessage += rowValues[currentField];
    errorMessage += "\" to type ";
    errorMessage += typeid(value).name();
    throw MySqlException(errorMessage);
  }

  ExtractRow(rowValues, currentField + 1, totalFields, args...);
}


template<typename Tuple, size_t I>
void MySql::SetTupleElements(MYSQL_ROW row, Tuple& tuple, int_<I>) {
  try {
    std::get<I>(tuple) = boost::lexical_cast<
        typename std::tuple_element<I, Tuple>::type
        >(row[I]);
  } catch (...) {
    std::string errorMessage("Unable to cast \"");
    errorMessage += row[I];
    errorMessage += "\" to type ";
    errorMessage += typeid(typename std::tuple_element<I, Tuple>::type).name();
    throw MySqlException(errorMessage);
  }
  // Recursively set the rest of the elements
  SetTupleElements(row, tuple, int_<I - 1>());
}


template<typename Tuple>
void MySql::SetTupleElements(MYSQL_ROW row, Tuple& tuple, int_<0>) {
  try {
    std::get<0>(tuple) = boost::lexical_cast<
        typename std::tuple_element<0, Tuple>::type
        >(row[0]);
  } catch (...) {
    std::string errorMessage("Unable to cast \"");
    errorMessage += row[0];
    errorMessage += "\" to type ";
    errorMessage += typeid(typename std::tuple_element<0, Tuple>::type).name();
    throw MySqlException(errorMessage);
  }
}


template<typename... Args>
void MySql::SetTuple(MYSQL_ROW row, std::tuple<Args...>& t) {
  SetTupleElements(row, t, int_<sizeof...(Args) - 1>());
}


template<typename... Args>
void MySql::RunQuery(const PreparedStatement& query, std::vector<std::tuple<Args...>>& results) {
  if (0 != mysql_query(conn, query.CStr())) {
    throw MySqlException(conn);
  }

  MYSQL_RES* result = mysql_store_result(conn);
  // If result is NULL, then the query was a statement that doesn't return
  // any results, e.g. 'USE mysql'
  if (nullptr != result) {
    // Check that the sizes match
    const size_t numFields = mysql_num_fields(result);
    if (numFields != sizeof...(Args)) {
      mysql_free_result(result);
      std::string errorMessage("Incorrect number of columns; expected ");
      errorMessage += boost::lexical_cast<std::string>(sizeof...(Args));
      errorMessage += " but query returned ";
      errorMessage += boost::lexical_cast<std::string>(numFields);
      throw MySqlException(errorMessage);
    }

    // Parse and save all of the rows
    MYSQL_ROW row = mysql_fetch_row(result);
    while (nullptr != row) {
      std::tuple<Args...> rowTuple;
      try {
        SetTuple(row, rowTuple);
      } catch (...) {
        mysql_free_result(result);
        throw;
      }

      results.push_back(rowTuple);
      row = mysql_fetch_row(result);
    }
  } else {
    // Well, the command was sent to the database anyway, so at least tell
    // the user if it succeeded or not
    const char* const errorMessage = mysql_error(conn);
    std::string exceptionMessage(
        nullptr == errorMessage
        ? "Statement succeeded"
        : errorMessage
        );
    exceptionMessage += "Arguments must be supplied to functions that return results: ";
    exceptionMessage += query.CStr();
    throw MySqlException(exceptionMessage);
  }
}


template<typename... Args>
PreparedStatement::PreparedStatement(
    const MySql& conn,
    const char* const query,
    const Args... args
    ) : prepareStream_(), preparedStatement_() {
  Init(conn, query, args...);
}


template<typename T, typename... Args>
void PreparedStatement::Init(
    const MySql& conn,
    const char* const statement,
    const T& arg,
    const Args... args) {
  for (const char* iter = statement; '\0' != *iter; ++iter) {
    // Replace placedholders with and escaped variants
    if ('?' == *iter && (iter == statement || '\\' != *(iter - 1))) {
      std::ostringstream argStream;
      argStream << arg;
      // Quote things that aren't already quoted
      const bool needsQuoting = (
          iter > statement
          && '\'' != *(iter - 1)
          && '"' != *(iter - 1)
          );

      if (needsQuoting) {
        prepareStream_ << '\'' << conn.Escape(argStream.str()) << '\'';
      } else {
        prepareStream_ << conn.Escape(argStream.str());
      }

      Init(conn, iter + 1, args...);
      return;
    }
    // It's not a placeholder, so just print it
    prepareStream_ << *iter;
  }
  // There weren't enough formatters to process all the args, so abort
  throw;
}


#endif
