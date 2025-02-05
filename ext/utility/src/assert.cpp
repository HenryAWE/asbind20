#include <asbind20/ext/assert.hpp>
#include <stdexcept>

namespace asbind20::ext
{
std::string extract_string(asIStringFactory* factory, void* str)
{
    assert(factory);

    asUINT sz = 0;
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
    asIStringFactory* str_factory = nullptr;

    void assert_simple(bool pred)
    {
        if(!pred)
        {
            on_failure("assertion failure");
        }
    }

    static void assert_msg_wrapper(asIScriptGeneric* gen)
    {
        assert(gen->GetArgCount() == 2);
        bool pred = gen->GetArgByte(0);

        if(!pred)
        {
            auto* this_ = static_cast<script_assert_impl*>(gen->GetAuxiliary());
            void* str = gen->GetArgAddress(1);

            std::string msg = extract_string(this_->str_factory, str);
            this_->on_failure(msg.c_str());
        }
    }

    void on_failure(const char* msg)
    {
        asIScriptContext* ctx = asGetActiveContext();
        if(callback)
            callback(msg);
        if(set_ex)
            ctx->SetException(msg);
    }
};

std::function<assert_handler_t> register_script_assert(
    asIScriptEngine* engine,
    std::function<assert_handler_t> callback,
    bool set_ex,
    asIStringFactory* str_factory
)
{
    global<true> g(engine);

    static script_assert_impl impl;

    impl.set_ex = set_ex;
    auto old = std::exchange(impl.callback, std::move(callback));

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
        if(string_t_id != asNO_FUNCTION)
        {
            assert(string_t_id >= 0);

            const char* string_t_decl = engine->GetTypeDeclaration(string_t_id, true);

            impl.str_factory = str_factory;

            g
                .function(
                    string_concat("void assert(bool pred, const ", string_t_decl, " &in msg)").c_str(),
                    &script_assert_impl::assert_msg_wrapper,
                    auxiliary(impl)
                );
        }
    }

    return std::move(old);
}
} // namespace asbind20::ext
