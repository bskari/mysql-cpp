#include "MySql.hpp"
#include "MySqlException.hpp"

#include <ostream>
#include <sstream>
#include <string>
#include <vector>

using std::ostream;
using std::string;
using std::ostringstream;
using std::vector;


void PreparedStatement::init(const MySql&, const char* const statement)
{
    for (const char* iter = statement; '\0' != *iter; ++iter)
    {
        if ('?' == *iter)
        {
            // There are no arguments to process for this placeholder, so abort
            throw MySqlException("Too few arguments to format string in PreparedStatement");
        }
        prepareStream_ << *iter;
    }

    preparedStatement_ = prepareStream_.str();
}


const char* PreparedStatement::c_str() const
{
    return preparedStatement_.c_str();
}


const string& PreparedStatement::str() const
{
    return preparedStatement_;
}


ostream& operator<<(ostream& out, const PreparedStatement& rhs)
{
    out << rhs.str();
    return out;
}
