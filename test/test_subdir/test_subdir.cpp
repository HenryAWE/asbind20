#include <iostream>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/array.hpp>
#include <asbind20/ext/stdstring.hpp>
#include <asbind20/ext/exec.hpp>

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

    using asbind20::fp;
    asIScriptEngine* engine = asCreateScriptEngine();
    asbind20::ext::register_script_array(engine);
    asbind20::ext::register_std_string(engine);
    asbind20::global(engine)
        .function(asbind20::use_generic, "void print(const string&in msg)", fp<&my_print>);

    int result = asbind20::ext::exec(engine, R"(print("hello");)");

    return result == asSUCCESS ? 0 : 1;
}
