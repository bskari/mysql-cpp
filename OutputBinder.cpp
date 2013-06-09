#include "MySqlException.hpp"
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

void throwIfArgumentCountWrong(
    const size_t numRequiredParameters,
    MYSQL_STMT* const statement
) {
    // Check that the sizes match
    const size_t fieldCount = mysql_stmt_field_count(statement);
    if (fieldCount != numRequiredParameters) {
        mysql_stmt_close(statement);
        string errorMessage{
            "Incorrect number of output parameters; query required "};
        errorMessage += lexical_cast<string>(fieldCount);
        errorMessage += " but ";
        errorMessage += lexical_cast<string>(numRequiredParameters);
        errorMessage += " parameters were provided";
        throw MySqlException{errorMessage};
    }
}


int bindAndExecuteStatement(
    vector<MYSQL_BIND>* const parameters,
    MYSQL_STMT* const statement
) {
    if (0 != mysql_stmt_bind_result(statement, parameters->data())) {
        MySqlException mse{mysql_stmt_error(statement)};
        mysql_stmt_close(statement);
        throw mse;
    }

    if (0 != mysql_stmt_execute(statement)) {
        MySqlException mse{mysql_stmt_error(statement)};
        mysql_stmt_close(statement);
        throw mse;
    }
    
    return mysql_stmt_fetch(statement);
}

void throwIfFetchError(const int fetchStatus, MYSQL_STMT* const statement) {
    switch (fetchStatus) {
        case MYSQL_NO_DATA:
            // No problem! All rows fetched.
            break;
        case 1: {  // Error occurred {
            MySqlException mse{mysql_stmt_error(statement)};
            mysql_stmt_close(statement);
            throw mse;
        }
        default: {
            assert(false && "Unknown error code from mysql_stmt_fetch");
            MySqlException mse{mysql_stmt_error(statement)};
            mysql_stmt_close(statement);
            throw mse;
        }
    }
}


void refetchTruncatedColumns(
    MYSQL_STMT* const statement,
    vector<MYSQL_BIND>* const parameters,
    vector<vector<char>>* const buffers,
    vector<mysql_bind_length_t>* const lengths
) {
    // Find which buffers were too small, expand them and refetch
    vector<tuple<size_t, size_t>> truncatedColumns;
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
    const vector<tuple<size_t, size_t>>::const_iterator end(
        truncatedColumns.end());
    for (
        vector<tuple<size_t, size_t>>::const_iterator i(
            truncatedColumns.begin());
        i != end;
        ++i
    ) {
        const size_t column = get<0>(*i);
        const size_t offset = get<1>(*i);
        MYSQL_BIND& parameter = parameters->at(column);
        const int status = mysql_stmt_fetch_column(
            statement,
            &parameter,
            column,
            offset);
        if (0 != status) {
            MySqlException mse{mysql_stmt_error(statement)};
            mysql_stmt_close(statement);
            throw mse;
        }

        // Now, for subsequent fetches, we need to reset the buffers
        vector<char>& buffer = buffers->at(column);
        parameter.buffer = buffer.data();
        parameter.buffer_length = buffer.size();
    }

    // If we've changed the buffers, we need to rebind
    if (0 != mysql_stmt_bind_result(statement, parameters->data())) {
        MySqlException mse{mysql_stmt_error(statement)};
        mysql_stmt_close(statement);
        throw mse;
    }
}

}  // namespace OutputBinderPrivate
