#include <cassert>

#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#include "MySql.hpp"
#include "MySqlException.hpp"

using std::cin;
using std::cout;
using std::endl;
using std::get;
using std::string;
using std::tuple;
using std::vector;


int main()
{
    cout << "Enter MySQL root password: " << endl;
    string password;
    cin >> password;

    MySql conn("127.0.0.1", "root", password.c_str(), nullptr);

    // Initialize a new test database
    conn.runCommand(
        PreparedStatement(
            conn,
            "DROP DATABASE IF EXISTS test_mysql_cpp"
        )
    );
    conn.runCommand(
        PreparedStatement(
            conn,
            "CREATE DATABASE test_mysql_cpp"
        )
    );
    conn.runCommand(PreparedStatement(conn, "USE test_mysql_cpp"));
    conn.runCommand(
        PreparedStatement(
            conn,
            "CREATE TABLE user ("
                "id INT NOT NULL AUTO_INCREMENT,"
                "PRIMARY KEY(id),"
                "email VARCHAR(64) NOT NULL,"
                "password CHAR(64) NOT NULL,"
                "age INT NOT NULL)"
        )
    );

    // ************
    // Easy inserts
    // ************
    conn.runCommand(
        PreparedStatement(
            conn,
            "INSERT INTO user (email, password, age)"
                " VALUES (?, ?, ?), (?, ?, ?)",
            "brandon.skari@gmail.com",
            "peace",
            21,
            "brandon@skari.org",
            "love",
            26
        )
    );

    vector<tuple<int, string, string, int> > users;
    // *****************************
    // Automatically escaped strings
    // *****************************
    conn.runQuery(
        PreparedStatement(
            conn,
            "SELECT * FROM user WHERE email = ?",
            "brandon.skari.org'; DROP DATABASE test_mysql_cpp -- "
        ),
        &users
    );
    assert(0 == users.size());

    // *****************
    // Type safe selects
    // *****************
    conn.runQuery(PreparedStatement(conn, "SELECT * FROM user"), &users);

    // GCC 4.6 added support for foreach loops
#if __GNUC__ > 4 || (4 == __GNUC__ && __GNUC_MINOR__ >= 6)
    for (const auto& user : users)
    {
        cout << get<0>(user)
            << ' ' << get<1>(user)
            << ' ' << get<2>(user)
            << ' ' << get<3>(user) << endl;
    }
    // GCC 4.4 added support for auto-typed variables
#elif __GNUC__ > 4 || (4 == __GNUC__ && __GNUC_MINOR__ >= 4)
    for (
        auto end(users.cend()), auto user(users.cbegin());
        user != end;
        ++user)
    {
        cout << *user << endl;
    }
#else
    // Well, we can still do it the old fashioned way
    for (
        tuple<int, string, string, int>::const_iterator end(users.end()),
        tuple<int, string, string, int>::const_iterator user(users.begin());
        user != end;
        ++user
    )
    {
        cout << *user << endl;
    }
#endif

    // **************************************
    // Look at all these type-based failures!
    // **************************************

    try
    {
        // Wrong number of fields
        vector<tuple<int> > ages;
        conn.runQuery(PreparedStatement(conn, "SELECT * FROM user"), &ages);
    }
    catch (const MySqlException& e)
    {
        cout << e.what() << endl;
    }

    try
    {
        // Bad casts
        vector<tuple<int> > integers;
        conn.runQuery(PreparedStatement(conn, "SELECT 1.0"), &integers);
    }
    catch (const MySqlException& e)
    {
        cout << e.what() << endl;
    }

    // Cleanup
    conn.runCommand(
        PreparedStatement(
            conn,
            "DROP DATABASE IF EXISTS test_mysql_cpp"
        )
    );
}
