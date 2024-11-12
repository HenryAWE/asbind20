#include <iostream>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/array.hpp>
#include <asbind20/ext/stdstring.hpp>

void my_print(const std::string& msg)
{
    std::cout << "[script] " << msg << std::endl;
}

int main()
{
    std::cout
        << "test_subdir\n"
        << asbind20::library_version()
        << std::endl;

    asIScriptEngine* engine = asCreateScriptEngine();
    asbind20::ext::register_script_array(engine);
    asbind20::ext::register_std_string(engine);
    asbind20::global(engine)
        .function<&my_print>(asbind20::use_generic, "void print(const string&in msg)");

    int result = asbind20::exec(engine, R"(print("hello");)");

    return result == asSUCCESS ? 0 : 1;
}
