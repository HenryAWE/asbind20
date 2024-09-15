#include <asbind20/bind.hpp>

namespace asbind20
{
bool has_max_portability()
{
    std::string_view options = asGetLibraryOptions();

    return options.find("AS_MAX_PORTABILITY") != options.npos;
}

namespace detail
{
    class_register_helper_base::class_register_helper_base(asIScriptEngine* engine, const char* name)
        : register_helper_base(engine), m_name(name) {}
} // namespace detail

global::global(asIScriptEngine* engine)
    : register_helper_base(engine) {}
} // namespace asbind20
