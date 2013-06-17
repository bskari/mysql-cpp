#include <boost/test/included/unit_test.hpp>
#include <string>

#include "testInputBinder.hpp"
#include "testMySql.hpp"
#include "testOutputBinder.hpp"

// Boost lets you name your tests, but I just want my tests to have the same
// name as the function, so use this macro to fill them in
#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)
#define FD(function) FunctionDescription(function, STR(function))

namespace test = boost::unit_test;
using std::string;

test::test_suite* init_unit_test_suite(int, char*[]) {
    typedef void(*testFunction)(void);
    class FunctionDescription {
        public:
            FunctionDescription(testFunction function, const char* name)
                : function_(function)
                , name_(name)
            {  // NOLINT[whitespaces/braces]
            }
            testFunction function_;
            const string name_;
    };

    FunctionDescription functions[] = {
        // Tests from testInputBinder.hpp
        FD(testBind),
        // Tests from testOutputBinder.hpp
        FD(testSetResult),
        FD(testSetParameter),
        // Tests from testMySql.hpp
        FD(testConnection),
        FD(testRunCommand),
        FD(testRunQuery),
        FD(testInvalidCommands),
        FD(testPreparedStatement)
    };

    for (const auto& functionDescription : functions) {
        test::framework::master_test_suite().add(
            test::make_test_case(
                test::callback0<>(functionDescription.function_),
                test::literal_string(
                    functionDescription.name_.c_str(),
                    functionDescription.name_.length())));
    }

    return 0;
}
