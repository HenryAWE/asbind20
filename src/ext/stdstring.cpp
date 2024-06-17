#include <asbind20/ext/stdstring.hpp>

namespace asbind20::ext
{
static bool string_opEquals(std::string& lhs, std::string& rhs)
{
    return lhs == rhs;
}

static std::string string_opAdd(const std::string& lhs, const std::string& rhs)
{
    return lhs + rhs;
}

static std::string string_substr(const std::string& this_, asUINT off, asUINT len)
{
    if(off >= this_.size())
        return std::string();
    return this_.substr(off, len);
}

void register_std_string(asIScriptEngine* engine, bool as_default)
{
    using std::string;

    value_class<string>(engine, "string")
        .register_basic_methods()
        .method(
            "bool opEquals(const string &in)",
            &string_opEquals,
            call_conv<asCALL_CDECL_OBJFIRST>
        )
        .method(
            "int opCmp(const string &in)",
            static_cast<int (string::*)(const string&) const>(&string::compare)
        )
        .method(
            "void append(const string &in)",
            static_cast<string& (string::*)(const string&)>(&string::append)
        )
        .method(
            "string opAdd(const string &in) const",
            &string_opAdd,
            call_conv<asCALL_CDECL_OBJFIRST>
        )
        .method("string substr(uint off, uint len) const", &string_substr)
        .method("bool empty()", &string::empty);

    if(as_default)
    {
        int r = engine->RegisterStringFactory("string", &string_factory::get());
        assert(r >= 0);
    }
}
} // namespace asbind20::ext
