mysql-cpp
=========

Type safe C++ interface to MySQL using C++11 variadic templates.

Examples
--------

> MySql conn("127.0.0.1", "root", "password", nullptr);
> conn.runCommand(PreparedStatement(conn, "DROP DATABASE IF EXISTS mysqlcpp"));
> conn.runCommand(PreparedStatement(conn, "CREATE DATABASE mysqlcpp"));
> conn.runCommand(
>     PreparedStatement(
>         conn,
>         "CREATE TABLE user (\
>             id INT NOT NULL AUTO_INCREMENT,\
>             PRIMARY KEY(id),\
>             email VARCHAR(64) NOT NULL,\
>             password CHAR(64) NOT NULL,\
>             age INT NOT NULL)"
>     )
> );
> conn.runCommand(
>     PreparedStatement(
>         conn,
>         "INSERT INTO user (email, password, age) VALUES (?, ?, ?), (?, ?, ?)",
>         "brandon.skari@gmail.com",
>         "peace",
>         21,
>         "brandon@skari.org",
>         "love'; DROP DATABASE mysqlcpp -- ",
>         26
>     )
> );
>
> vector<tuple<int, string, string, int>> users;
> conn.runQuery(PreparedStatement(conn, "SELECT * FROM user"), users);
> for (const auto& user : users)
> {
>     cout << get<0>(user)
>         << ' ' << get<1>(user)
>         << ' ' << get<2>(user)
>         << ' ' << get<3>(user)
>         << endl;
> }
