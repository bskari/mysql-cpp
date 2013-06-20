#include "MySqlException.hpp"
#include "MySqlPreparedStatement.hpp"
#include "OutputBinder.hpp"

#include <cassert>
#include <mysql/mysql.h>

#include <boost/lexical_cast.hpp>
#include <string>
#include <tuple>
#include <vector>

using boost::lexical_cast;
using std::get;
using std::string;
using std::tuple;
using std::vector;

namespace OutputBinderPrivate {

void Friend::throwIfParameterCountWrong(
    const size_t numRequiredParameters,
    const MySqlPreparedStatement& statement
) {
    // Check that the sizes match
    if (statement.getFieldCount() != numRequiredParameters) {
        string errorMessage(
            "Incorrect number of output parameters; query required ");
        errorMessage += lexical_cast<string>(statement.getFieldCount());
        errorMessage += " but ";
        errorMessage += lexical_cast<string>(numRequiredParameters);
        errorMessage += " parameters were provided";
        throw MySqlException(errorMessage);
    }
}


int Friend::bindAndExecuteStatement(
    vector<MYSQL_BIND>* const parameters,
    const MySqlPreparedStatement& statement
) {
    if (0 != mysql_stmt_bind_result(
        statement.statementHandle_,
        parameters->data()))
    {
        throw MySqlException(mysql_stmt_error(statement.statementHandle_));
    }

    if (0 != mysql_stmt_execute(statement.statementHandle_)) {
        throw MySqlException(mysql_stmt_error(statement.statementHandle_));
    }
    
    return mysql_stmt_fetch(statement.statementHandle_);
}

void Friend::throwIfFetchError(
    const int fetchStatus,
    const MySqlPreparedStatement& statement
) {
    switch (fetchStatus) {
        case MYSQL_NO_DATA:
            // No problem! All rows fetched.
            break;
        case 1: {  // Error occurred {
            throw MySqlException(mysql_stmt_error(statement.statementHandle_));
        }
        default: {
            assert(false && "Unknown error code from mysql_stmt_fetch");
            throw MySqlException(mysql_stmt_error(statement.statementHandle_));
        }
    }
}

void Friend::refetchTruncatedColumns(
    const MySqlPreparedStatement& statement,
    vector<MYSQL_BIND>* const parameters,
    vector<vector<char>>* const buffers,
    vector<mysql_bind_length_t>* const lengths
) {
    // Find which buffers were too small, expand them and refetch
    typedef unsigned int mysql_column_t;
    typedef unsigned long mysql_offset_t;
    vector<tuple<mysql_column_t, mysql_offset_t>> truncatedColumns;
    for (size_t i = 0; i < lengths->size(); ++i) {
        vector<char>& buffer = buffers->at(i);
        const size_t untruncatedLength = lengths->at(i);
        if (untruncatedLength > buffer.size()) {
            // Only refetch the part that we didn't get the first time
            const size_t alreadyRetrieved = buffer.size();
            truncatedColumns.push_back(
                tuple<size_t, size_t>(i, alreadyRetrieved));
            buffer.resize(untruncatedLength + 1);
            MYSQL_BIND& bind = parameters->at(i);
            bind.buffer = &buffer.at(alreadyRetrieved);
            bind.buffer_length = buffer.size() - alreadyRetrieved - 1;
        }
    }

    // I'm not sure why, but I occasionally get the truncated status
    // code when nothing was truncated... so just break out?
    if (truncatedColumns.empty()) {
        // No truncations!
        return;
    }

    // Refetch only the data that were truncated
    for (const auto& i : truncatedColumns) {
        const mysql_column_t column = get<0>(i);
        const mysql_offset_t offset = get<1>(i);
        MYSQL_BIND& parameter = parameters->at(column);
        const int status = mysql_stmt_fetch_column(
            statement.statementHandle_,
            &parameter,
            column,
            offset);
        if (0 != status) {
            throw MySqlException(mysql_stmt_error(statement.statementHandle_));
        }

        // Now, for subsequent fetches, we need to reset the buffers
        vector<char>& buffer = buffers->at(column);
        parameter.buffer = buffer.data();
        parameter.buffer_length = buffer.size();
    }

    // If we've changed the buffers, we need to rebind
    if (0 != mysql_stmt_bind_result(
        statement.statementHandle_,
        parameters->data()))
    {
        throw MySqlException(mysql_stmt_error(statement.statementHandle_));
    }
}


int Friend::fetch(const MySqlPreparedStatement& statement) {
    return mysql_stmt_fetch(statement.statementHandle_);
}

}  // namespace OutputBinderPrivate
