/**
 * @file std_string.hpp
 * @author HenryAWE
 * @brief  Register std::string as script string for building test script
 */

#ifndef ASBIND_TEST_STD_STRING_HPP
#define ASBIND_TEST_STD_STRING_HPP

#pragma once

#include <string>
#include <asbind20/asbind.hpp>
#include "utf8.hpp"

namespace asbind_test
{
/**
 * @brief Set engine properties for string extension
 *
 * This will configure the script engine to use UTF-8 everywhere,
 * and enable char literal, which means 'a' is an integral value instead of a string.
 */
void configure_engine_for_ext_string(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine
);

/**
 * @brief Setup `char32_t` as script char
 * @note Request asEP_USE_CHARACTER_LITERALS set to true
 */
void setup_script_char(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    bool generic = asbind20::has_max_portability()
);

/**
 * @brief String factory for std::string
 */
class string_factory : public AS_NAMESPACE_QUALIFIER asIStringFactory
{
public:
    struct string_hash
    {
        using is_transparent = void;

        std::size_t operator()(const char* txt) const
        {
            return std::hash<std::string_view>{}(txt);
        }

        std::size_t operator()(std::string_view txt) const
        {
            return std::hash<std::string_view>{}(txt);
        }

        std::size_t operator()(const std::string& txt) const
        {
            return std::hash<std::string>{}(txt);
        }
    };

    using container_type = std::unordered_map<
        std::string,
        std::size_t,
        string_hash,
        std::equal_to<>,
        asbind20::script_allocator<std::pair<const std::string, std::size_t>>>;

    const void* GetStringConstant(
        const char* data, AS_NAMESPACE_QUALIFIER asUINT length
    ) override
    {
        std::lock_guard lock(asbind20::as_exclusive_lock);

        std::string_view view(data, length);
        auto it = m_cache.find(view);
        if(it != m_cache.end())
            it->second += 1;
        else
        {
#ifndef ASBIND20_NO_EXCEPTIONS
            try
#endif
            {
                it = m_cache.emplace_hint(
                    it,
                    std::piecewise_construct,
                    std::make_tuple(view),
                    std::make_tuple(1u)
                );
            }
#ifndef ASBIND20_NO_EXCEPTIONS
            catch(...)
            {
                asbind20::set_script_exception(
                    "string_factory: failed to create string"
                );
                return nullptr;
            }
#endif
        }

        return &it->first;
    }

    int ReleaseStringConstant(const void* str) override
    {
        auto* ptr = static_cast<const std::string*>(str);

        if(!ptr) [[unlikely]]
            return AS_NAMESPACE_QUALIFIER asERROR;

        int r = AS_NAMESPACE_QUALIFIER asSUCCESS;

        std::lock_guard lock(asbind20::as_exclusive_lock);

        auto it = m_cache.find(*ptr);

        if(it == m_cache.end())
            r = AS_NAMESPACE_QUALIFIER asERROR;
        else
        {
            assert(it->second != 0);
            it->second -= 1;
            if(it->second == 0)
                m_cache.erase(it);
        }

        return r;
    }

    int GetRawStringData(
        const void* str, char* data, AS_NAMESPACE_QUALIFIER asUINT* length
    ) const override
    {
        auto* ptr = static_cast<const std::string*>(str);

        if(ptr == nullptr)
            return AS_NAMESPACE_QUALIFIER asERROR;

        std::lock_guard lock(asbind20::as_shared_lock);

        if(length)
            *length = static_cast<AS_NAMESPACE_QUALIFIER asUINT>(ptr->size());
        if(data)
            ptr->copy(data, ptr->size());
        return AS_NAMESPACE_QUALIFIER asSUCCESS;
    }

    static string_factory& get()
    {
        static string_factory instance{};
        return instance;
    }

private:
    container_type m_cache;
};

namespace script_string
{
    // For security concerns,
    // all APIs that modify the string will return the result as a copy.

    using size_type = AS_NAMESPACE_QUALIFIER asUINT;
    using index_type = std::make_signed_t<size_type>;

    std::string string_construct(size_type count, char32_t ch);

    size_type string_size(const std::string& this_);

    std::string string_append(const std::string& this_, const std::string& str);
    std::string string_append_ch(const std::string& this_, char32_t ch);

    std::string string_prepend(const std::string& this_, const std::string& str);
    std::string string_prepend_ch(const std::string& this_, char32_t ch);

    std::string string_remove_prefix(const std::string& this_, size_type n);

    std::string string_remove_suffix(const std::string& this_, size_type n);

    char32_t string_opIndex(const std::string& this_, index_type idx);

    bool string_starts_with_ch(const std::string& this_, char32_t ch);

    bool string_ends_with_ch(const std::string& this_, char32_t ch);

    bool string_contains(const std::string& this_, const std::string& str);
    bool string_contains_ch(const std::string& this_, char32_t ch);

    std::string string_substr(const std::string& this_, int pos, size_type n);

    std::string string_replace(
        const std::string& this_,
        index_type where,
        size_type n,
        const std::string& str,
        index_type pos = 0,
        size_type len = size_type(-1)
    );

    std::string string_insert(
        const std::string& this_,
        index_type where,
        const std::string& str,
        index_type pos = 0,
        size_type len = size_type(-1)
    );

    std::string string_erase(
        const std::string& this_, index_type where, size_type n
    );
} // namespace script_string

void setup_script_string(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    bool generic = asbind20::has_max_portability(),
    bool as_default = true
);

/**
 * @brief Convert a character to string
 */
[[nodiscard]]
inline std::string script_chr(char32_t ch)
{
    char buf[4];
    unsigned int size_bytes = utf8::u8_int_to_bytes(ch, buf);
    return std::string(buf, size_bytes);
}

void setup_string_utils(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    bool generic = asbind20::has_max_portability()
);
} // namespace asbind_test

template <>
struct asbind20::type_traits<char32_t>
{
    static char32_t get_arg(
        AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen,
        AS_NAMESPACE_QUALIFIER asUINT arg
    )
    {
        return *static_cast<char32_t*>(gen->GetAddressOfArg(arg));
    }

    static int set_return(
        AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen, char32_t val
    )
    {
        new(gen->GetAddressOfReturnLocation()) char32_t(val);
        return AS_NAMESPACE_QUALIFIER asSUCCESS;
    }
};

#endif
