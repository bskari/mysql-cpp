#ifndef MY_SQL_EXCEPTION_HPP_
#define MY_SQL_EXCEPTION_HPP_

#include <mysql/mysql.h>

#include <exception>
#include <string>

class MySqlPreparedStatement;

class MySqlException : public std::exception {
    public:
        explicit MySqlException(const std::string& message);
        explicit MySqlException(const MYSQL* const connection);
        explicit MySqlException(const MySqlPreparedStatement& statement);
        ~MySqlException() noexcept;

//        MySqlException& operator=(const MySqlException&) = delete;
//        MySqlException& operator=(MySqlException&&) = delete;

        const char* what() const noexcept;

        static const char* getServerErrorMessage(
            const MYSQL* const connection);
        static const char* getServerErrorMessage(
            const MYSQL_STMT* const statement);
    private:
        const std::string message_;
};

#endif  // MY_SQL_EXCEPTION_HPP_
