/**
 * Integration tests for the MySQL interface. You should have a user named
 * 'test_mysql_cpp' that can log in from the localhost without a password and
 * who has all permissions on a database named 'test_mysql_cpp'
 */
#ifndef TESTS_TESTMYSQL_HPP_
#define TESTS_TESTMYSQL_HPP_

/**
 * Tests the connection to MySQL.
 */
void testConnection();

void testRunCommand();

void testRunQuery();

/**
 * Tests what happens when invalid commands are run, e.g. when runCommand is
 * used to run a query or vice-versa, or when just plain invalid commands are
 * run, e.g. "DANCE FOR ME"
 */
void testInvalidCommands();

void testPreparedStatement();

#endif  // TESTS_TESTMYSQL_HPP_
