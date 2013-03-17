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


template<std::size_t> struct int_{}; // compile-time counter

template<typename Char, typename Traits, typename Tuple, std::size_t I>
void printTuple(std::basic_ostream<Char, Traits>& out, Tuple const& t, int_<I>)
{
    printTuple(out, t, int_<I-1>());
    out << ", " << std::get<I>(t);
}

template<typename Char, typename Traits, typename Tuple>
void printTuple(std::basic_ostream<Char, Traits>& out, Tuple const& t, int_<0>)
{
      out << std::get<0>(t);
}

template<typename Char, typename Traits, typename... Args>
std::ostream& operator<<(
    std::basic_ostream<Char, Traits>& out,
    std::tuple<Args...> const& t
)
{
    out << "(";
    printTuple(out, t, int_<sizeof...(Args) - 1>());
    out << ")";
    return out;
}


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

    conn.runCommand("USE test_mysql_cpp");
    conn.runCommand("DROP TABLE IF EXISTS delete_me");
    conn.runCommand("CREATE TABLE delete_me (id INT AUTO_INCREMENT, name VARCHAR(10) NOT NULL, PRIMARY KEY(id))");
    conn.runCommand("INSERT INTO delete_me VALUES (NULL, ?)", "Brandon");
    conn.runCommand("INSERT INTO delete_me (name) VALUES (?)", "Brandon");

    int x = 0;
    std::vector<std::tuple<int, string>> results;
    conn.runQuery(&results, "SELECT id, name FROM delete_me WHERE id > ?", x);
    const auto end(results.cend());
    cout << "Size: " << results.size() << endl;
    for (auto i(results.cbegin()); i != end; ++i)
    {
        cout << *i << endl;
    }

    return EXIT_SUCCESS;
}
