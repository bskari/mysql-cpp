#include <cassert>

#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#include "MySql.hpp"
#include "MySqlException.hpp"

using std::basic_ostream;
using std::cin;
using std::cout;
using std::endl;
using std::get;
using std::ostream;
using std::string;
using std::tuple;
using std::vector;


template<std::size_t> struct int_{}; // compile-time counter

template<typename Char, typename Traits, typename Tuple, std::size_t I>
void printTuple(std::basic_ostream<Char, Traits>& out, Tuple const& t, int_<I>);

template<typename Char, typename Traits, typename Tuple>
void printTuple(std::basic_ostream<Char, Traits>& out, Tuple const& t, int_<0>);

template<typename Char, typename Traits, typename... Args>
ostream& operator<<(basic_ostream<Char, Traits>& out, tuple<Args...> const& t);


int main(int argc, char* argv[])
{
    string password;
    if (argc == 1)
    {
        cout << "Enter MySQL root password: " << endl;
        cin >> password;
    }
    else
    {
        password = argv[1];
    }
    MySql conn("127.0.0.1", "root", password.c_str(), nullptr);

    // Initialize a new test database
    conn.runCommand("DROP DATABASE IF EXISTS test_mysql_cpp");
    conn.runCommand("CREATE DATABASE test_mysql_cpp");
    conn.runCommand("USE test_mysql_cpp");
    conn.runCommand("DROP TABLE IF EXISTS user");
    conn.runCommand(
        "CREATE TABLE user ("
            "id INT NOT NULL AUTO_INCREMENT,"
            "PRIMARY KEY(id),"
            "email VARCHAR(64) NOT NULL,"
            "password CHAR(64) NOT NULL,"
            "age INT NOT NULL)"
    );

    // ************
    // Easy inserts
    // ************
    conn.runCommand(
        "INSERT INTO user (email, password, age)"
            " VALUES (?, ?, ?), (?, ?, ?)",
        "brandon.skari@gmail.com",
        "peace",
        21,
        "brandon@skari.org",
        "love",
        26
    );

    typedef tuple<int, string, string, int> userTuple;
    vector<userTuple> users;
    // *****************************************
    // All commands use safe prepared statements
    // *****************************************
    const string naughtyUser("brandon@skari.org'; DROP TABLE users; -- ");
    conn.runQuery(
        &users,
        "SELECT * FROM user WHERE email = ?",
        naughtyUser
    );
    assert(0 == users.size());

    conn.runQuery(
        &users,
        "SELECT * FROM user WHERE email = ?",
        " something' OR 'dummy' = 'dummy"
    );
    assert(0 == users.size());

    // ***************************
    // Automatically typed selects
    // ***************************
    conn.runQuery(&users, "SELECT * FROM user");
    const vector<userTuple>::const_iterator end(users.end());
    for (
        vector<userTuple>::const_iterator user(users.begin());
        user != end;
        ++user
    )
    {
        cout << *user << endl;
    }

    // **************************************
    // Look at all these type-based failures!
    // **************************************
    try
    {
        // Wrong number of fields
        vector<tuple<int>> ages;
        conn.runQuery(&ages, "SELECT * FROM user");
    }
    catch (const MySqlException& e)
    {
        cout << e.what() << endl;
    }

    return EXIT_SUCCESS;
}


template<typename Char, typename Traits, typename Tuple, size_t I>
void printTuple(basic_ostream<Char, Traits>& out, Tuple const& t, int_<I>)
{
    printTuple(out, t, int_<I-1>());
    out << ", " << get<I>(t);
}

template<typename Char, typename Traits, typename Tuple>
void printTuple(basic_ostream<Char, Traits>& out, Tuple const& t, int_<0>)
{
      out << get<0>(t);
}

template<typename Char, typename Traits, typename... Args>
ostream& operator<<(
    basic_ostream<Char, Traits>& out,
    tuple<Args...> const& t
)
{
    out << "(";
    printTuple(out, t, int_<sizeof...(Args) - 1>());
    out << ")";
    return out;
}
