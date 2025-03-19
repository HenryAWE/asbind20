#include <asbind20/ext/assert.hpp>
#include <stdexcept>

namespace asbind20::ext
{
std::string extract_string(AS_NAMESPACE_QUALIFIER asIStringFactory* factory, const void* str)
{
    assert(factory);

    AS_NAMESPACE_QUALIFIER asUINT sz = 0;
    if(factory->GetRawStringData(str, nullptr, &sz) < 0)
    {
        throw std::runtime_error("failed to get raw string data");
    }

    std::string result;
    result.resize(sz);

    if(factory->GetRawStringData(str, result.data(), nullptr) < 0)
    {
        throw std::runtime_error("failed to get raw string data");
    }

    return result;
}

class script_assert_impl
{
public:
    std::function<assert_handler_t> callback;
    bool set_ex = true;
    AS_NAMESPACE_QUALIFIER asIStringFactory* str_factory = nullptr;

    void assert_simple(bool pred)
    {
        if(!pred)
        {
            using namespace std::string_view_literals;
            on_failure("assertion failure"sv);
        }
    }

    static void assert_msg_wrapper(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
    {
        assert(gen->GetArgCount() == 2);
        bool pred = get_generic_arg<bool>(gen, 0);

        if(!pred)
        {
            auto& this_ = get_generic_auxiliary<script_assert_impl>(gen);
            const void* str = gen->GetArgAddress(1);

            std::string msg = extract_string(this_.str_factory, str);
            this_.on_failure(msg);
        }
    }

private:
    void on_failure(std::string_view msg)
    {
        AS_NAMESPACE_QUALIFIER asIScriptContext* ctx = current_context();
        if(set_ex)
        {
            with_cstr(
                &AS_NAMESPACE_QUALIFIER asIScriptContext::SetException,
                ctx,
                msg,
                true // allow catch
            );
        }
        if(callback)
            callback(msg);
    }
};

void register_script_assert(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    std::function<assert_handler_t> callback,
    bool set_ex,
    AS_NAMESPACE_QUALIFIER asIStringFactory* str_factory
)
{
    global<true> g(engine);

    static script_assert_impl impl;

    impl.set_ex = set_ex;
    impl.callback = std::move(callback);

    g
        .function(
            use_generic,
            "void assert(bool pred)",
            fp<&script_assert_impl::assert_simple>,
            auxiliary(impl)
        );

    if(str_factory)
    {
#if ANGELSCRIPT_VERSION >= 23800
        int string_t_id = engine->GetStringFactory();
#else
        int string_t_id = engine->GetStringFactoryReturnTypeId();
#endif
        if(string_t_id != AS_NAMESPACE_QUALIFIER asNO_FUNCTION)
        {
            assert(string_t_id >= 0);

            const char* string_t_decl = engine->GetTypeDeclaration(string_t_id, true);

            impl.str_factory = str_factory;

            g
                .function(
                    string_concat("void assert(bool pred,const ", string_t_decl, "&in msg)"),
                    &script_assert_impl::assert_msg_wrapper,
                    auxiliary(impl)
                );
        }
    }
}
} // namespace asbind20::ext
