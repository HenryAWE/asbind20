#include <asbind20/ext/testing.hpp>
#include <sstream>
#include <iostream>

namespace asbind20::ext
{
template <bool UseGeneric>
void register_script_test_framework_impl(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, testing::suite& instance
)
{
    using testing::suite;

    global<UseGeneric>(engine)
        .function("void expect_true(bool val)", fp<&suite::expect_true>, auxiliary(instance))
        .function("void expect_false(bool val)", fp<&suite::expect_false>, auxiliary(instance));
}

void register_script_test_framework(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    testing::suite& instance,
    bool generic
)
{
    namespace_ ns(engine, "testing");

    if(generic)
        register_script_test_framework_impl<true>(engine, instance);
    else
        register_script_test_framework_impl<false>(engine, instance);
}

namespace testing
{
    suite::suite(std::string name)
        : m_name(std::move(name)),
          m_failed(false) {}

    suite::~suite() = default;

    void suite::expect_true(bool val)
    {
        if(val) [[likely]]
            return;

        set_failed();
        write_message(string_concat(
            "Expected: true\nActual: false\n",
            format_current_loc(current_context())
        ));
    }

    void suite::expect_false(bool val)
    {
        if(!val) [[likely]]
            return;

        set_failed();
        write_message(string_concat(
            "Expected: false\nActual: true\n",
            format_current_loc(current_context())
        ));
    }

    void suite::set_failed() noexcept
    {
        m_failed = true;
    }

    std::ostream& suite::get_ostream() const
    {
        return std::cout;
    }

    void suite::write_message(std::string_view msg)
    {
        std::ostream& os = get_ostream();

        auto prefix_helper = [&]()
        {
            os << '[' << m_name << "] ";
        };

        prefix_helper();
        if(msg.empty())
        {
            os << '\n';
            return;
        }

        for(char c : msg)
        {
            if(c == '\n')
            {
                os << '\n';
                prefix_helper();
                continue;
            }

            os.put(c);
        }

        if(!msg.empty() && msg.back() != '\n')
            os << '\n';
    }

    std::string suite::format_current_loc(AS_NAMESPACE_QUALIFIER asIScriptContext* ctx)
    {
        if(!ctx) [[unlikely]]
            return "invalid context";

        auto* f = ctx->GetFunction();
        if(!f) [[unlikely]]
            return "invalid function";

        std::stringstream ss;

        ss
            << "Func: "
            << f->GetDeclaration(true, true, true);

        const char* section;
        int row, col;
        int r = f->GetDeclaredAt(&section, &row, &col);
        if(r < 0) [[unlikely]]
            ss << " (unknown location)";
        else
        {
            ss
                << " ("
                << section
                << ": "
                << row << ':' << col << ')';
        }

        return std::move(ss).str();
    }
} // namespace testing
} // namespace asbind20::ext
