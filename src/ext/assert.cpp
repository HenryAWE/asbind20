#include <asbind20/ext/assert.hpp>
#include <asbind20/ext/helper.hpp>

namespace asbind20::ext
{
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
            assert(gen->GetEngine()->GetStringFactoryReturnTypeId() == gen->GetArgTypeId(1));

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
    static script_assert_impl impl;

    auto old = std::exchange(impl.callback, std::move(callback));

    global(engine)
        .function(
            "void assert(bool pred)",
            &script_assert_impl::assert_simple,
            impl
        );

    if(str_factory)
    {
        int string_t_id = engine->GetStringFactoryReturnTypeId();
        if(string_t_id != asNO_FUNCTION)
        {
            assert(string_t_id >= 0);

            const char* string_t_decl = engine->GetTypeDeclaration(string_t_id);

            impl.str_factory = str_factory;

            global(engine)
                .function(
                    asbind20::detail::concat("void assert(bool pred, const ", string_t_decl, " &in msg)").c_str(),
                    &script_assert_impl::assert_msg_wrapper,
                    &impl
                );
        }
    }

    return std::move(old);
}
} // namespace asbind20::ext
