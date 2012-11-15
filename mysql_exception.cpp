#include "mysql_exception.h"
#include <mysql/mysql.h>

#include <string>

using std::string;

MySqlException::MySqlException(const string& message)
    : message_(message)
{
}


MySqlException::MySqlException(const MYSQL* const conn)
    : message_(getServerErrorMessage(conn))
{
}


MySqlException::~MySqlException() throw()
{
}


const char* MySqlException::what() const throw()
{
    return message_.c_str();
}


const char* MySqlException::getServerErrorMessage(const MYSQL* const conn)
{
    // This error should be unique per connection, so it should be thread safe
    // The MySQL C interface is backward compatible with C89, so it doesn't
    // recognize const. It *should* be const though, so just work around it.
    const char* const message = mysql_error(const_cast<MYSQL*>(conn));
    if ('\0' != message[0])  // Error message isn't empty
    {
        return message;
    }
    return "Unknown error";
}
