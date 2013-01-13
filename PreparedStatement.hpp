#ifndef PREPAREDSTATEMENT_HPP_
#define PREPAREDSTATEMENT_HPP_

#include <sstream>

class MySql;


class PreparedStatement
{
public:
    template<typename... Args>
    PreparedStatement(
        const MySql& conn,
        const char* const statement,
        const Args... args
    );

    const char* c_str() const;
    const std::string& str() const;

private:
    template<typename T, typename... Args>
    void init(
        const MySql& conn,
        const char* const statement,
        const T& arg,
        const Args... args
    );
    void init(const MySql& conn, const char* const statement);

    std::ostringstream prepareStream_;
    std::string preparedStatement_;

    friend std::ostream& operator<<(std::ostream& out, const PreparedStatement& rhs);
};


// Work around circular dependencies
#include "MySql.hpp"


template<typename... Args>
PreparedStatement::PreparedStatement(
    const MySql& conn,
    const char* const query,
    const Args... args
)
    : prepareStream_()
    , preparedStatement_()
{
    init(conn, query, args...);
}


template<typename T, typename... Args>
void PreparedStatement::init(
    const MySql& conn,
    const char* const statement,
    const T& arg,
    const Args... args
)
{
    for (const char* iter = statement; '\0' != *iter; ++iter)
    {
        // Replace placedholders with and escaped variants
        if ('?' == *iter && (iter == statement || '\\' != *(iter - 1)))
        {
            std::ostringstream argStream;
            argStream << arg;
            // Quote things that aren't already quoted
            const bool needsQuoting = (
                iter > statement
                && '\'' != *(iter - 1)
                && '"' != *(iter - 1)
            );

            if (needsQuoting)
            {
                prepareStream_ << '\'' << conn.escape(argStream.str()) << '\'';
            }
            else
            {
                prepareStream_ << conn.escape(argStream.str());
            }

            init(conn, iter + 1, args...);
            return;
        }
        // It's not a placeholder, so just print it
        prepareStream_ << *iter;
    }
    // There weren't enough formatters to process all the args, so abort
    throw;
}


#endif  // PREPARED_STATEMENT_HPP_
