#include "MySql.hpp"
#include "MySqlException.hpp"

#include <cassert>

#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

using std::basic_ostream;
using std::cin;
using std::cout;
using std::endl;
using std::get;
using std::ostream;
using std::string;
using std::tuple;
using std::unique_ptr;
using std::vector;


template <std::size_t> struct int_ {};  // compile-time counter

template <typename Char, typename Traits, typename Tuple, std::size_t I>
void printTuple(basic_ostream<Char, Traits>& out, Tuple const& t, int_<I>);

template <typename Char, typename Traits, typename Tuple>
void printTuple(basic_ostream<Char, Traits>& out, Tuple const& t, int_<0>);

template <typename Tuple>
void printTupleVector(const vector<Tuple>& v);

template <typename Char, typename Traits, typename... Args>
ostream& operator<<(basic_ostream<Char, Traits>& out, tuple<Args...> const& t);

template <typename T>
ostream& operator<<(ostream& out, const unique_ptr<T>& ptr);


int main(int argc, char* argv[]) {
    string password;
    if (argc == 1) {
        cout << "Enter MySQL root password: " << endl;
        cin >> password;
    } else {
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
            "age INT)");

    // ************
    // Easy inserts
    // ************
    int ages[] = {27, 21, 26};
    string emails[] = {
        "bskari@yelp.com",
        "brandon.skari@gmail.com",
        "brandon@skari.org"};
    string passwords[] = {
        "peace",
        "love",
        "griffin"};
    conn.runCommand(
        "INSERT INTO user (email, password, age)"
            " VALUES (?, ?, ?), (?, ?, ?), (?, ?, ?)",
        emails[0], passwords[0], ages[0],
        emails[1], passwords[1], ages[1],
        emails[2], passwords[2], ages[2]);

    typedef tuple<int, string, string, int> userTuple;
    vector<userTuple> users;
    // *****************************************
    // All commands use safe prepared statements
    // *****************************************
    const string naughtyUser("brandon@skari.org'; DROP TABLE users; -- ");
    conn.runQuery(&users, "SELECT * FROM user WHERE email = ?", naughtyUser);
    assert(0 == users.size());

    const char naughtyUser2[] = "something' OR '1' = 1' --  ";
    const char* charPtr = naughtyUser2;
    conn.runQuery(&users, "SELECT * FROM user WHERE email = ?", charPtr);
    assert(0 == users.size());

    // ***************************
    // Automatically typed selects
    // ***************************
    conn.runQuery(&users, "SELECT * FROM user");
    printTupleVector(users);
    users.clear();

    // ************************
    // Dealing with NULL values
    // ************************
    conn.runCommand(
        "INSERT INTO user (email, password, age) VALUES (?, ?, NULL)",
        emails[0],
        passwords[0]);

    try {
        // Trying to insert NULLs into a normal tuple will throw
        conn.runQuery(&users, "SELECT * FROM user");
    } catch (const MySqlException& e) {
        cout << e.what() << endl;
    }

    // But, we can select into tuples with unique_ptr
    typedef tuple<
        unique_ptr<int>,
        unique_ptr<string>,
        unique_ptr<string>,
        unique_ptr<int>
    > autoPtrUserTuple;
    vector<autoPtrUserTuple> autoPtrUsers;
    conn.runQuery(&autoPtrUsers, "SELECT * FROM user");
    printTupleVector(autoPtrUsers);

    // *********************************************
    // Raw pointers are gross, so this won't compile
    // *********************************************
    /*
    vector<tuple<int*>> rawPointers;
    conn.runQuery(&rawPointers, "SELECT age FROM user");
    */

    // **************************************
    // Look at all these type-based failures!
    // **************************************
    try {
        // Wrong number of fields
        vector<tuple<int>> selectAges;
        conn.runQuery(&selectAges, "SELECT * FROM user");
    } catch (const MySqlException& e) {
        cout << e.what() << endl;
    }

    exit(EXIT_SUCCESS);
}


template<typename Char, typename Traits, typename Tuple, size_t I>
void printTuple(basic_ostream<Char, Traits>& out, Tuple const& t, int_<I>) {
    printTuple(out, t, int_<I - 1>());
    out << ", " << get<I>(t);
}

template<typename Char, typename Traits, typename Tuple>
void printTuple(basic_ostream<Char, Traits>& out, Tuple const& t, int_<0>) {
      out << get<0>(t);
}

template <typename Tuple>
void printTupleVector(const vector<Tuple>& v) {
#if __GNUC__ >= 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
    for (const auto& item : v)
    {
        cout << item << endl;
    }
#elif __GNUC__ >= 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4)
    auto end = v.cend();
    for (auto item(v.cbegin()); item != end; ++item) {
        cout << *item<< endl;
    }
#else
    vector<Tuple>::const_iterator end(users.end());
    for (
        vector<Tuple>::const_iterator item(v.begin());
        item != end;
        ++item
    ) {
        cout << *item << endl;
    }
#endif
}

template<typename Char, typename Traits, typename... Args>
ostream& operator<<(
    basic_ostream<Char, Traits>& out,
    tuple<Args...> const& t
) {
    out << "(";
    printTuple(out, t, int_<sizeof...(Args) - 1>());
    out << ")";
    return out;
}

template <typename T>
ostream& operator<<(ostream& out, const unique_ptr<T>& ptr) {
    if (nullptr != ptr.get()) {
        out << *ptr;
    } else {
        out << "NULL";
    }
    return out;
}
