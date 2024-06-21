#include <asbind20/bind.hpp>

namespace asbind20
{
namespace detail
{
    static bool max_portability_req()
    {
        std::string_view options = asGetLibraryOptions();

        return options.find("AS_MAX_PORTABILITY") != options.npos;
    }

    class_register_base::class_register_base(asIScriptEngine* engine, const char* name)
        : m_engine(engine), m_name(name), m_force_generic(max_portability_req())
    {}
} // namespace detail
} // namespace asbind20
