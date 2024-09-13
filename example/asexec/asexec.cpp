#include <iostream>
#include <cstdlib>
#include <angelscript.h>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/array.hpp>
#include <asbind20/ext/stdstring.hpp>
#include <asbind20/ext/math.hpp>
#include <asbind20/ext/helper.hpp>
#include <iostream>

void script_print(const std::string& str, bool newline)
{
    std::cout << str;
    if(newline)
        std::cout << std::endl;
}

void message_callback(const asSMessageInfo* msg, void*)
{
    const char* level_str = "";
    switch(msg->type)
    {
    case asMSGTYPE_ERROR: level_str = "ERROR: "; break;
    case asMSGTYPE_WARNING: level_str = "WARNING: "; break;
    case asMSGTYPE_INFORMATION: level_str = "INFO: "; break;
    }
    std::cerr
        << level_str
        << msg->section
        << "(" << msg->row << ':' << msg->col << "): "
        << msg->message
        << std::endl;
}

int main(int argc, char* argv[])
{
    if(argc <= 1)
    {
        std::cout
            << "USAGE\n"
            << "asexec [script]\n"
            << '\n'
            << "The asexec will use \"int main()\" or \"void main()\" in the script as entry point.\n"
            << '\n'
            << "INFORMATION\n"
            << "ANGELSCRIPT_VERSION_STRING: " << ANGELSCRIPT_VERSION_STRING
            << '\n'
            << "asGetLibraryVersion: " << asGetLibraryVersion()
            << '\n'
            << "asGetLibraryOptions: " << asGetLibraryOptions()
            << '\n'
            << "asbind20::library_version: " << asbind20::library_version()
            << '\n'
            << "asbind20::library_options: " << asbind20::library_options()
            << std::endl;
        return 0;
    }

    asIScriptEngine* engine = asCreateScriptEngine();
    engine->SetMessageCallback(asFUNCTION(message_callback), nullptr, asCALL_CDECL);

    engine->SetEngineProperty(asEP_USE_CHARACTER_LITERALS, true);

    asbind20::ext::register_script_array(engine);
    asbind20::ext::register_math_function(engine);
    asbind20::ext::register_script_hash(engine);
    asbind20::ext::register_std_string(engine);
    asbind20::ext::register_string_utils(engine);
    asbind20::global(engine)
        .function("void print(const string&in str, bool newline=true)", &script_print);

    asIScriptModule* m = engine->GetModule("asexec", asGM_ALWAYS_CREATE);
    int r = asbind20::load_file(
        m,
        argv[1]
    );
    if(r < 0)
    {
        std::cerr << "Failed to load script: " << r << std::endl;
        return 1;
    }
    r = m->Build();
    if(r < 0)
    {
        std::cerr << "Failed to build module: " << r << std::endl;
        return 1;
    }

    int ret_val = EXIT_SUCCESS;
    asIScriptFunction* entry_i = m->GetFunctionByDecl("int main()");
    asIScriptFunction* entry_v = m->GetFunctionByDecl("void main()");
    if(entry_i)
    {
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, entry_i);
        if(!result.has_value())
        {
            std::cerr << "Script execution error: " << result.error() << std::endl;
            ret_val = EXIT_FAILURE;
        }
        else
            ret_val = *result;
    }
    else if(entry_v)
    {
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<void>(ctx, entry_v);
        if(!result.has_value())
        {
            std::cerr << "Script execution error: " << result.error() << std::endl;
            ret_val = EXIT_FAILURE;
        }
    }
    else
    {
        std::cerr
            << "Cannot find a suitable entry point (either \"int main()\" or \"void main()\")"
            << std::endl;
        ret_val = EXIT_FAILURE;
    }

    m->Discard();
    engine->ShutDownAndRelease();
    return ret_val;
}
