#include "testInputBinder.hpp"

#include <boost/test/included/unit_test.hpp>
#include <string>

// Boost lets you name your tests, but I just want my tests to have the same
// name as the function, so use this macro to fill them in
#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)
#define FD(function) FunctionDescription(function, STR(function))

namespace test = boost::unit_test;
using std::string;

test::test_suite* init_unit_test_suite(int, char*[])
{
    typedef void(*testFunction)(void);
    class FunctionDescription
    {
        public:
            FunctionDescription(testFunction function, const char* name)
                : function_(function)
                , name_(name)
            {
            }
            testFunction function_;
            const string name_;
    };

    FunctionDescription functions[] = {
        // Tests from testInputBinder.hpp
        FD(testBind)
    };

    for (size_t i = 0; i < sizeof(functions) / sizeof(functions[0]); ++i)
    {
        test::framework::master_test_suite().add(
            test::make_test_case(
                test::callback0<>(functions[i].function_),
                test::literal_string(
                    functions[i].name_.c_str(),
                    functions[i].name_.length()
                )
            )
        );
    }

    return 0;
}
