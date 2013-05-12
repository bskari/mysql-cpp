#ifndef MY_SQL_EXCEPTION_HPP_
#define MY_SQL_EXCEPTION_HPP_

#include <mysql/mysql.h>

#include <exception>
#include <string>

#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 6)
#define nullptr 0
#endif

class MySqlException : public std::exception {
    public:
        explicit MySqlException(const std::string& message);
        explicit MySqlException(const MYSQL* const connection);
        explicit MySqlException(const MYSQL_STMT* const statement);
        ~MySqlException() throw();

        const char* what() const throw();

    private:
        const std::string message_;
        static inline const char* getServerErrorMessage(
            const MYSQL* const connection);
        static inline const char* getServerErrorMessage(
            const MYSQL_STMT* const statement);
};

#endif  // MY_SQL_EXCEPTION_HPP_
