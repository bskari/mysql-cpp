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
#include "OutputBinder.hpp"

#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 6)
#ifndef nullptr
// I know that this isn't a perfect substitution, but I'm lazy
#define nullptr 0
#endif
#endif


class MySql {
    public:
        MySql(
            const char* const hostname,
            const char* const username,
            const char* const password,
            const char* const database,
            const uint16_t port = 3306);

        MySql(
            const char* hostname,
            const char* username,
            const char* password,
            const uint16_t port = 3306);

        ~MySql();

        MySql(const MySql& rhs) = delete;
        MySql(MySql&& rhs) = delete;
        MySql& operator=(const MySql& rhs) = delete;
        MySql& operator=(MySql&& rhs) = delete;

        /**
         * Normal query. Results are stored in the given vector.
         * @param query The query to run.
         * @param results A vector of tuples to store the results in.
         * @param args Arguments to bind to the query.
         */
        template<typename... InputArgs, typename... OutputArgs>
        void runQuery(
            std::vector<std::tuple<OutputArgs...>>* const results,
            const char* const query,
            // Args needs to be sent by reference, because the values need to be
            // nontemporary (that is, lvalues) so that their memory locations
            // can be bound to MySQL's prepared statement API
            const InputArgs&... args);

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
            // Args needs to be sent by reference, because the values need to be
            // nontemporary (that is, lvalues) so that their memory locations
            // can be bound to MySQL's prepared statement API
            const Args&... args);
        my_ulonglong runCommand(const char* const command);

    private:
        MYSQL* connection_;

        // Hidden methods
};


inline static void closeAndThrow(MYSQL_STMT* const statement);


template<typename... Args>
my_ulonglong MySql::runCommand(
    const char* const command,
    const Args&... args
) {
    MYSQL_STMT* const statement = mysql_stmt_init(connection_);
    if (nullptr == statement) {
        throw MySqlException{connection_};
    }

    const size_t length = ::strlen(command);
    const int status = mysql_stmt_prepare(statement, command, length);
    if (0 != status) {
        closeAndThrow(statement);
    }

    // Commands (e.g. INSERTs or DELETEs) should always have this set to 0
    if (0 != mysql_stmt_field_count(statement)) {
        std::string errorMessage;
        if (0 != mysql_stmt_free_result(statement)) {
            errorMessage = "Unable to free result - ";
        }
        if (0 != mysql_stmt_close(statement)) {
            errorMessage += "Unable to close statement - ";
        }
        errorMessage += "Tried to run query with runCommand";
        throw MySqlException{errorMessage};
    }

    const size_t parameterCount = mysql_stmt_param_count(statement);
    if (sizeof...(args) != parameterCount) {
        std::string errorMessage;
        if (0 != mysql_stmt_free_result(statement)) {
            errorMessage = "Unable to free result - ";
        }
        if (0 != mysql_stmt_close(statement)) {
            errorMessage += "Unable to close statement - ";
        }
        errorMessage += "Incorrect number of parameters; command required ";
        errorMessage += boost::lexical_cast<std::string>(parameterCount);
        errorMessage += " but ";
        errorMessage += boost::lexical_cast<std::string>(sizeof...(args));
        errorMessage += " parameters were provided.";
        throw MySqlException{errorMessage};
    }

    std::vector<MYSQL_BIND> bindParameters;
    bindParameters.resize(parameterCount);
    bindInputs<Args...>(&bindParameters, args...);
    if (0 != mysql_stmt_bind_param(statement, &bindParameters[0])) {
        closeAndThrow(statement);
    }

    if (0 != mysql_stmt_execute(statement)) {
        closeAndThrow(statement);
    }

    // If the user ran a SELECT statement or something else, at least warn them
    const my_ulonglong affectedRows = mysql_stmt_affected_rows(statement);
    if (((my_ulonglong)(-1)) == affectedRows) {
        closeAndThrow(statement);
    }

    // Cleanup
    if (0 != mysql_stmt_free_result(statement)) {
        std::string errorMessage;
        if (0 != mysql_stmt_close(statement)) {
            errorMessage += "Unable to close statement - ";
        }
        errorMessage += "Unable to free result";
        throw MySqlException{errorMessage};
    }
    if (0 != mysql_stmt_close(statement)) {
        throw MySqlException{"Unable to close statement"};
    }
    return affectedRows;
}


template<typename... InputArgs, typename... OutputArgs>
void MySql::runQuery(
    std::vector<std::tuple<OutputArgs...>>* const results,
    const char* const query,
    const InputArgs&... args
) {
    assert(nullptr != results);
    MYSQL_STMT* const statement = mysql_stmt_init(connection_);

    const size_t length = ::strlen(query);
    if (0 != mysql_stmt_prepare(statement, query, length)) {
        closeAndThrow(statement);
    }

    // SELECTs should always return something. Commands (e.g. INSERTs or
    // DELETEs) should always have this set to 0.
    if (0 == mysql_stmt_field_count(statement)) {
        std::string errorMessage;
        if (0 != mysql_stmt_free_result(statement)) {
            errorMessage = "Unable to free result - ";
        }
        if (0 != mysql_stmt_close(statement)) {
            errorMessage += "Unable to close statement - ";
        }
        errorMessage += "Tried to run command with runQuery";
        throw MySqlException{errorMessage};
    }

    // Bind the input parameters
    // Check that the parameter count is right
    const size_t parameterCount = mysql_stmt_param_count(statement);
    if (sizeof...(InputArgs) != parameterCount) {
        std::string errorMessage;
        if (0 != mysql_stmt_free_result(statement)) {
            errorMessage = "Unable to free result - ";
        }
        if (0 != mysql_stmt_close(statement)) {
            errorMessage += "Unable to close statement - ";
        }

        errorMessage += "Incorrect number of input parameters; query required ";
        errorMessage += boost::lexical_cast<std::string>(parameterCount);
        errorMessage += " but ";
        errorMessage += boost::lexical_cast<std::string>(sizeof...(args));
        errorMessage += " parameters were provided.";
        throw MySqlException{errorMessage};
    }

    std::vector<MYSQL_BIND> inputBindParameters;
    inputBindParameters.resize(parameterCount);
    bindInputs<InputArgs...>(&inputBindParameters, args...);
    if (0 != mysql_stmt_bind_param(statement, &inputBindParameters[0])) {
        closeAndThrow(statement);
    }

    setResults<OutputArgs...>(statement, results);

    // Cleanup
    if (0 != mysql_stmt_free_result(statement)) {
        std::string errorMessage("Unable to free result");
        if (0 != mysql_stmt_close(statement)) {
            errorMessage += " - Unable to close statement";
        }
        throw MySqlException{errorMessage};
    }
    if (0 != mysql_stmt_close(statement)) {
        throw MySqlException{"Unable to close statement"};
    }
}


static void closeAndThrow(MYSQL_STMT* const statement) {
    std::string errorMessage(MySqlException::getServerErrorMessage(statement));
    if (0 != mysql_stmt_free_result(statement)) {
        errorMessage += "; There was an error freeing this statement";
    }
    if (0 != mysql_stmt_close(statement)) {
        errorMessage += "; There was an error closing this statement";
    }
    throw MySqlException{errorMessage};
}

#endif  // MYSQL_HPP_
