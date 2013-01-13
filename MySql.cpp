#include "MySql.hpp"

#include <cassert>
#include <cstdint>
#include <mysql/mysql.h>

#include <string>
#include <sstream>
#include <vector>

#include "MySqlException.hpp"


using std::string;
using std::vector;


    // Delegating constructors are supported in GCC 4.7
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7)
MySql::MySql(
    const char* hostname,
    const char* username,
    const char* password,
    const uint16_t port
)
    : MySql(hostname, username, password, nullptr, port)
{
}
#endif


MySql::MySql(
    const char* const hostname,
    const char* const username,
    const char* const password,
    const char* const database,
    const uint16_t port
)
    : conn(mysql_init(nullptr))
{
    if (nullptr == conn)
    {
        throw MySqlException("Unable to connect to MySQL");
    }

    const MYSQL* const success = mysql_real_connect(
        conn,
        hostname,
        username,
        password,
        database,
        port,
        nullptr,
        0
    );
    if (nullptr == success)
    {
        MySqlException mse(conn);
        mysql_close(conn);
        throw mse;
    }
}


MySql::~MySql()
{
    mysql_close(conn);
}


void MySql::runCommand(const PreparedStatement& command) const
{
    if (0 != mysql_query(conn, command.c_str()))
    {
        throw MySqlException(conn);
    }

    MYSQL_RES* result = mysql_store_result(conn);
    // If result is NULL, then the query was a command that doesn't return
    // any results, e.g. 'USE mysql'
    if (nullptr == result)
    {
        // If there's an error message, the command failed
        const char* const message = mysql_error(conn);
        if ('\0' != message[0])
        {
            throw MySqlException(conn);
        }
        mysql_free_result(result);
    }
    else
    {
        // Well, the command was sent to the database anyway, so at least tell
        // the user if it succeeded or not
        const char* const errorMessage = mysql_error(conn);
        string exceptionMessage(
            nullptr == errorMessage
            ? "Statement succeeded"
            : errorMessage
        );
        exceptionMessage += "Arguments cannot be supplied to functions that"
            " don't return results";
        throw MySqlException(exceptionMessage);
    }
}


void MySql::extractRow(
    MYSQL_ROW,
    const size_t currentField,
    const size_t totalFields
) const
{
    if (currentField != totalFields)
    {
        throw MySqlException("Too many columns returned by query");
    }
}


string MySql::escape(const string& s) const
{
    // Make the buffer twice as big so that any escaped values will fit
    vector<char> buffer(s.size() * 2 + 1);

    const size_t usedChars = mysql_real_escape_string(
        conn,
        &buffer.at(0),
        s.c_str(),
        s.size()
    );

    assert('\0' == buffer.at(usedChars));
    return string(&buffer.at(0), &buffer.at(usedChars));
}
