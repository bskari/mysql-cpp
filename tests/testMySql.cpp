#include "testMySql.hpp"
#include "../MySql.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>
#include <exception>
#include <memory>
#include <mysql/mysql.h>
#include <string>
#include <tuple>
#include <vector>

using boost::bad_lexical_cast;
using std::exception;
using std::get;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::vector;
using std::tuple;


// Default user is a user named "test_mysql_cpp" with full privileges a
// database named "test_mysql_cpp" and no other privileges
const char* const username = "test_mysql_cpp";
const char* const password = nullptr;
const char* const database = "test_mysql_cpp";

static void createUserTable(const unique_ptr<MySql>& connection);
static void testSimpleSelects(
    const unique_ptr<MySql>& connection,
    const char* const host
);


void testConnection()
{
    // Try the connection through a Unix domain socket
    try
    {
        const char* const host = "localhost";
        unique_ptr<MySql> connection(
            new MySql(host, username, password, database)
        );
        testSimpleSelects(connection, host);
    }
    catch (const exception& e)
    {
        BOOST_ERROR(e.what());
    }

    // Try the connection through a TCP socket
    try
    {
        const char* const host = "127.0.0.1";
        unique_ptr<MySql> connection(
            new MySql(host, username, password, database)
        );
        // MySQL still reports the user as connection from "localhost"
        testSimpleSelects(connection, "localhost");
    }
    catch (const exception& e)
    {
        BOOST_ERROR(e.what());
    }

    // TODO(bskari|2013-03-23) Try the connection through a remote server and
    // a server running on a different port
}


void testRunCommand()
{
    try
    {
        const char* const host = "localhost";
        unique_ptr<MySql> connection(
            new MySql(host, username, password, database)
        );

        // Test affected row counts
        createUserTable(connection);

        my_ulonglong affectedRows = connection->runCommand(
            "INSERT INTO user (name, password) VALUES "
            "('brandon', 'peace'), "
            "('gary', NULL)"
        );
        BOOST_CHECK(2 == affectedRows);
        
        BOOST_CHECK_THROW(
            connection->runCommand(
                "INSERT INTO user (name, password) VALUES ('brandon', 'password')"
            ),
            MySqlException
        );

        affectedRows = connection->runCommand(
            "UPDATE user SET password = 'love' WHERE name = 'gary'"
        );
        BOOST_CHECK(1 == affectedRows);

        // Test some injection safety
        const string injection("'; CREATE TABLE dummy (i INT); -- ");
        affectedRows = connection->runCommand(
            "UPDATE user SET password = 'griffin' WHERE password = ?",
            injection
        );
        BOOST_CHECK(0 == affectedRows);

        // Test that incorrect number of parameters is handled
        BOOST_CHECK_THROW(
            connection->runCommand(
                "UPDATE user SET password = 'griffin' WHERE password = ?"
            ),
            MySqlException
        );

        BOOST_CHECK_THROW(
            connection->runCommand(
                "UPDATE user SET password = 'griffin' WHERE password = ?",
                injection,
                injection
            ),
            MySqlException
        );
    }
    catch (const exception& e)
    {
        BOOST_ERROR(e.what());
    }
}


void testRunQuery()
{
    try
    {
        const char* const host = "localhost";
        unique_ptr<MySql> connection(
            new MySql(host, username, password, database)
        );

        createUserTable(connection);

        my_ulonglong affectedRows = connection->runCommand(
            "INSERT INTO user (name, password) VALUES "
            "('brandon', 'peace'), "
            "('gary', NULL)"
        );
        BOOST_CHECK(2 == affectedRows);

        // Selecting NULLs without a std::shared_ptr should throw
        vector<tuple<string, string>> rawTypeValues;
        BOOST_CHECK_THROW(
            connection->runQuery(
                &rawTypeValues,
                "SELECT name, password FROM user"
            ),
            MySqlException
        );
        rawTypeValues.clear();

        // Selecting NULLs with std::shared_ptr should be fine though
        vector<tuple<shared_ptr<string>, shared_ptr<string>>> sharedPtrValues;
        connection->runQuery(
            &sharedPtrValues,
            "SELECT name, password FROM user ORDER BY id ASC"
        );
        BOOST_CHECK(
            2 == sharedPtrValues.size()
            && nullptr != get<0>(sharedPtrValues.at(0))
            && nullptr != get<1>(sharedPtrValues.at(0))
            && nullptr != get<0>(sharedPtrValues.at(1))
            && nullptr == get<1>(sharedPtrValues.at(1))
        );
        sharedPtrValues.clear();

        // Test some injection safety
        const string injection = "7 UNION SELECT 'test', 'inject' -- ";
        connection->runQuery(
            &sharedPtrValues,
            "SELECT name, password FROM user WHERE id = ?",
            injection
        );
        BOOST_CHECK(0 == sharedPtrValues.size());

        // Test that incorrect number of parameters is handled
        BOOST_CHECK_THROW(
            connection->runQuery(
                &sharedPtrValues,
                "SELECT name, password FROM user WHERE id = ?"
            ),
            MySqlException
        );
        BOOST_CHECK_THROW(
            connection->runQuery(
                &sharedPtrValues,
                "SELECT name, password FROM user WHERE id = ?",
                injection,
                injection
            ),
            MySqlException
        );
    }
    catch (const exception& e)
    {
        BOOST_ERROR(e.what());
    }
}


void testInvalidCommands()
{
    try
    {
        const char* const host = "localhost";
        unique_ptr<MySql> connection(
            new MySql(host, username, password, database)
        );

        createUserTable(connection);

        my_ulonglong affectedRows = connection->runCommand(
            "INSERT INTO user (name, password) VALUES "
            "('brandon', 'peace'), "
            "('gary', NULL)"
        );
        BOOST_CHECK(2 == affectedRows);

        // Run a query using runCommand
        // runCommand is overloaded, with one taking Args... and one taking
        // no additional arguments, so test both
        BOOST_CHECK_THROW(
            connection->runCommand(
                "SELECT * FROM user"
            ),
            MySqlException
        );
        int arg = 1;
        BOOST_CHECK_THROW(
            connection->runCommand(
                "SELECT * FROM user WHERE id IN (?)",
                arg
            ),
            MySqlException
        );

        // Run a command using runQuery
        vector<tuple<shared_ptr<string>, shared_ptr<string>>> sharedPtrValues;
        BOOST_CHECK_THROW(
            connection->runQuery(
                &sharedPtrValues,
                "UPDATE user SET name = 'brandon2' WHERE name = 'brandon'"
            ),
            MySqlException
        );
        sharedPtrValues.clear();
        // If a command is given to runQuery, it shouldn't be run...
        vector<tuple<int>> counts;
        connection->runQuery(
            &counts,
            "SELECT COUNT(*) FROM user WHERE name = 'brandon2'"
        );
        BOOST_CHECK(1 == counts.size());
        if (1 == counts.size())
        {
            BOOST_CHECK_MESSAGE(
                0 == get<0>(counts.at(0)),
                "Incorrect use of runQuery for running commands should not"
                    " run the provided command"
            );
        }
        counts.clear();

        // General invalid queries
        BOOST_CHECK_THROW(
            connection->runCommand("Dance for me, MySQL!"),
            MySqlException
        );

        // Too few arguments for runCommand
        BOOST_CHECK_THROW(
            connection->runCommand(
                "UPDATE user SET name = 'brandon2' WHERE name = ?"
            ),
            MySqlException
        );
        // Invalid numbers of arguments shouldn't be run
        connection->runQuery(
            &counts,
            "SELECT COUNT(*) FROM user WHERE name = 'brandon2'"
        );
        BOOST_CHECK(1 == counts.size());
        if (1 == counts.size())
        {
            BOOST_CHECK_MESSAGE(
                0 == get<0>(counts.at(0)),
                "Incorrect parameters for running commands should not"
                    " run the provided command"
            );
        }
        counts.clear();

        // Too many arguments for runCommand
        const string brandon("brandon");
        BOOST_CHECK_THROW(
            connection->runCommand(
                "UPDATE user SET name = 'brandon2' WHERE name = ?",
                brandon,
                brandon
            ),
            MySqlException
        );
        // Invalid numbers of arguments shouldn't be run
        connection->runQuery(
            &counts,
            "SELECT COUNT(*) FROM user WHERE name = 'brandon2'"
        );
        BOOST_CHECK(1 == counts.size());
        if (1 == counts.size())
        {
            BOOST_CHECK_MESSAGE(
                0 == get<0>(counts.at(0)),
                "Incorrect parameters for running commands should not"
                    " run the provided command"
            );
        }
        counts.clear();

        // Too few arguments for runQuery
        BOOST_CHECK_THROW(
            connection->runQuery(
                &sharedPtrValues,
                "SELECT name, password FROM user WHERE name = ?"
            ),
            MySqlException
        );

        // Too many arguments for runCommand
        BOOST_CHECK_THROW(
            connection->runQuery(
                &sharedPtrValues,
                "SELECT name, passwor FROM user WHERE name = ?",
                brandon,
                brandon
            ),
            MySqlException
        );
    }
    catch (const exception& e)
    {
        BOOST_ERROR(e.what());
    }
}


void createUserTable(const unique_ptr<MySql>& connection)
{
    my_ulonglong affectedRows = connection->runCommand(
        "DROP TABLE IF EXISTS user"
    );
    BOOST_CHECK(0 == affectedRows);

    affectedRows = connection->runCommand(
        "CREATE TABLE user ("
            "id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,"
            "name VARCHAR(20) NOT NULL,"
            "password VARCHAR(20),"
            "UNIQUE (name)"
        ")"
    );
    BOOST_CHECK(0 == affectedRows);
}


void testSimpleSelects(
    const unique_ptr<MySql>& connection,
    const char* const host
)
{
    vector<tuple<shared_ptr<string>>> results;

    connection->runQuery(&results, "SELECT USER()");
    const string user(string(username) + "@" + host);
    BOOST_CHECK(
        1 == results.size()
        && nullptr != get<0>(results.at(0))
        && user == *get<0>(results.at(0))
    );
    results.clear();

    connection->runQuery(&results, "SELECT DATABASE()");
    BOOST_CHECK(
        1 == results.size()
        && database == *get<0>(results.at(0))
    );
    results.clear();
}
