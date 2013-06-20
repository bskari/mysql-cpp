#include "MySqlException.hpp"
#include "MySqlPreparedStatement.hpp"

#include <mysql/mysql.h>
#include <string>

using std::string;

MySqlException::MySqlException(const string& message)
    : message_(message)
{
}


MySqlException::MySqlException(const MYSQL* const connection)
    : message_(getServerErrorMessage(connection))
{
}


MySqlException::MySqlException(const MySqlPreparedStatement& statement)
    : message_(getServerErrorMessage(statement.statementHandle_))
{
}


MySqlException::~MySqlException() noexcept {
}


const char* MySqlException::what() const noexcept {
    return message_.c_str();
}


const char* MySqlException::getServerErrorMessage(const MYSQL* const conn) {
    // This error should be unique per connection, so it should be thread safe
    // The MySQL C interface is backward compatible with C89, so it doesn't
    // recognize const. It *should* be const though, so just work around it.
    const char* const message = mysql_error(const_cast<MYSQL*>(conn));
    if ('\0' != message[0]) {  // Error message isn't empty
        return message;
    }
    return "Unknown error";
}


const char* MySqlException::getServerErrorMessage(
    const MYSQL_STMT* const statement
) {
    // The MySQL C interface is backward compatible with C89, so it doesn't
    // recognize const. It *should* be const though, so just work around it.
    const char* const message = mysql_stmt_error(
        const_cast<MYSQL_STMT*>(statement));
    if ('\0' != message[0]) {  // Error message isn't empty
        return message;
    }
    return "Unknown error";
}
