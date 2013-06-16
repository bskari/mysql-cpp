#ifndef MYSQL_PREPARED_STATEMENT_HPP_
#define MYSQL_PREPARED_STATEMENT_HPP_

#include <cstddef>
// MYSQL and MYSQL_STMT are typedefs so we have to include mysql.h.
// Otherwise, I would just forward declare them.
#include <mysql/mysql.h>

#include <vector>

namespace OutputBinderPrivate {
    // Used in the friend class declaration below
    class Friend;
}

class MySqlPreparedStatement {
    public:
        MySqlPreparedStatement(MySqlPreparedStatement&& rhs) = default;
        ~MySqlPreparedStatement();

        size_t getParameterCount() const {
            return parameterCount_;
        }

        size_t getFieldCount() const {
            return fieldCount_;
        }


    private:
        // I don't want external uses to mess with this class, but these
        // friends need to access the raw MYSQL_STMT* to fetch rows and other
        // stuff.
        friend class MySql;
        friend class OutputBinderPrivate::Friend;
        friend class MySqlException;

        // External users should call MySQL::prepareStatement
        MySqlPreparedStatement(const char* query, MYSQL* connection);

        MySqlPreparedStatement() = delete;
        MySqlPreparedStatement(const MySqlPreparedStatement&) = delete;
        const MySqlPreparedStatement& operator=(
            const MySqlPreparedStatement&) = delete;
        MySqlPreparedStatement& operator=(MySqlPreparedStatement&&) = default;

        // This should be const, but the MySQL C interface doesn't use const
        // anywhere, so I'd have to typecast the constness whenever I'd want
        // to use it
        MYSQL_STMT* statementHandle_;
        size_t parameterCount_;
        size_t fieldCount_;
};

#endif  // MYSQL_PREPARED_STATEMENT_HPP_
