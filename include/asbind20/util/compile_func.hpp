#ifndef ASBIND20_UTIL_COMPILE_FUNC_HPP
#define ASBIND20_UTIL_COMPILE_FUNC_HPP

#include "../fwd.hpp"
#include "strutil.hpp"
#include "../invoke.hpp"

namespace asbind20
{
template <typename Signature>
class compile_function_result;

template <typename Signature>
[[nodiscard]]
compile_function_result<Signature> compile_function(
    module_reference m,
    cstring_ref section_name,
    cstring_ref code,
    int line_offset = 0,
    bool add_to_module = false
);

template <typename Signature>
[[nodiscard]]
compile_function_result<Signature> compile_function(
    module_pointer m,
    cstring_ref section_name,
    cstring_ref code,
    int line_offset = 0,
    bool add_to_module = false
);

/**
 * @brief Result of `compile_function`
 *
 * The result owns the compiled script function, i.e., it will release the
 * function object when destroyed.
 *
 * @note The result is move-only. `get()` returns a reference to the owned
 *       function; keep the result object alive while using it.
 */
template <typename Signature>
class compile_function_result
{
    friend compile_function_result<Signature> compile_function<Signature>(
        module_reference m,
        cstring_ref section_name,
        cstring_ref code,
        int line_offset,
        bool add_to_module
    );

    friend compile_function_result<Signature> compile_function<Signature>(
        module_pointer m,
        cstring_ref section_name,
        cstring_ref code,
        int line_offset,
        bool add_to_module
    );

public:
    using signature_type = Signature;
    using function_type = script_function<Signature>;
    using error_type = AS_NAMESPACE_QUALIFIER asERetCodes;

    compile_function_result() = delete;

private:
    // Takes ownership of `f` without increasing the reference count
    compile_function_result(function_pointer f, int r) noexcept
        : m_func(std::in_place, f), m_r(r) {}

public:
    compile_function_result(compile_function_result&& other) noexcept = default;

    [[nodiscard]]
    function_type& get() noexcept
    {
        return m_func;
    }

    [[nodiscard]]
    const function_type& get() const noexcept
    {
        return m_func;
    }

    [[nodiscard]]
    error_type error() const noexcept
    {
        if(m_r >= 0)
            return AS_NAMESPACE_QUALIFIER asSUCCESS;
        return static_cast<error_type>(m_r);
    }

    /**
     * @brief Get The result of compilation
     *
     * @return Negative value on error
     */
    [[nodiscard]]
    int result() const noexcept
    {
        return m_r;
    }

    explicit operator bool() const noexcept
    {
        return m_r >= 0;
    }

private:
    function_type m_func;
    int m_r;
};

template <typename Signature>
compile_function_result<Signature> compile_function(
    module_reference m,
    cstring_ref section_name,
    cstring_ref code,
    int line_offset,
    bool add_to_module
)
{
    function_pointer out = nullptr;
    int r = m.CompileFunction(
        section_name.c_str(),
        code.c_str(),
        line_offset,
        add_to_module ?
            AS_NAMESPACE_QUALIFIER asCOMP_ADD_TO_MODULE :
            0 /* not add to the module  */,
        &out
    );

    return compile_function_result<Signature>(out, r);
}

template <typename Signature>
compile_function_result<Signature> compile_function(
    module_pointer m,
    cstring_ref section_name,
    cstring_ref code,
    int line_offset,
    bool add_to_module
)
{
    if(!m) [[unlikely]]
        return compile_function_result<Signature>(
            nullptr, AS_NAMESPACE_QUALIFIER asINVALID_ARG
        );
    return compile_function<Signature>(
        *m, section_name, code, line_offset, add_to_module
    );
}
} // namespace asbind20

#endif
