#include "MySql.hpp"
#include "MySqlException.hpp"

#include <cassert>
#include <cstdint>
#include <mysql/mysql.h>

#include <string>
#include <sstream>
#include <vector>


using std::string;
using std::vector;


MySql::MySql(
    const char* hostname,
    const char* username,
    const char* password,
    const uint16_t port
)
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7)
    : MySql(hostname, username, password, nullptr, port)
{
}
#else
    : connection_(mysql_init(nullptr))
{
    if (nullptr == connection_) {
        throw MySqlException("Unable to connect to MySQL");
    }

    const MYSQL* const success = mysql_real_connect(
        connection_,
        hostname,
        username,
        password,
        nullptr,
        port,
        nullptr,
        0);
    if (nullptr == success) {
        MySqlException mse(connection_);
        mysql_close(connection_);
        throw mse;
    }
}
#endif


MySql::MySql(
    const char* const hostname,
    const char* const username,
    const char* const password,
    const char* const database,
    const uint16_t port
)
    : connection_(mysql_init(nullptr))
{
    if (nullptr == connection_) {
        throw MySqlException("Unable to connect to MySQL");
    }

    const MYSQL* const success = mysql_real_connect(
        connection_,
        hostname,
        username,
        password,
        database,
        port,
        nullptr,
        0);
    if (nullptr == success) {
        MySqlException mse(connection_);
        mysql_close(connection_);
        throw mse;
    }
}


MySql::~MySql() {
    mysql_close(connection_);
}


my_ulonglong MySql::runCommand(const char* const command) {
    if (0 != mysql_real_query(connection_, command, strlen(command))) {
        throw MySqlException(connection_);
    }

    // If the user ran a SELECT statement or something else, at least warn them
    const my_ulonglong affectedRows = mysql_affected_rows(connection_);
    if ((my_ulonglong) - 1 == affectedRows) {
        // Clean up after the query
        MYSQL_RES* const result = mysql_store_result(connection_);
        mysql_free_result(result);

        throw MySqlException("Tried to run query with runCommand");
    }

    return affectedRows;
}


MySqlPreparedStatement MySql::prepareStatement(const char* const command) const {
    return MySqlPreparedStatement(command, connection_);
}
