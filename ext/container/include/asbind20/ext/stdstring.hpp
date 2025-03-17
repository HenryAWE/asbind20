/**
 * @file stdstring.hpp
 * @author HenryAWE
 * @brief Extension for string and char
 *
 * @note Set "asEP_USE_CHARACTER_LITERALS" to true for better experience
 */

#ifndef ASBIND20_EXT_CONTAINER_STDSTRING_HPP
#define ASBIND20_EXT_CONTAINER_STDSTRING_HPP

#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <ranges>
#include <asbind20/asbind.hpp>
#include "utf8.hpp"
#include "array.hpp"

namespace asbind20::ext
{
/**
 * @brief Set engine properties for string extension
 */
void configure_engine_for_ext_string(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine
);

void register_script_char(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    bool generic = has_max_portability()
);

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
        as_allocator<std::pair<const std::string, std::size_t>>>;

    const void* GetStringConstant(
        const char* data, AS_NAMESPACE_QUALIFIER asUINT length
    ) override
    {
        std::lock_guard lock(as_exclusive_lock);

        std::string_view view(data, length);
        auto it = m_cache.find(view);
        if(it != m_cache.end())
            it->second += 1;
        else
        {
            try
            {
                it = m_cache.emplace_hint(
                    it,
                    std::piecewise_construct,
                    std::make_tuple(view),
                    std::make_tuple(1u)
                );
            }
            catch(...)
            {
                set_script_exception("string_factory: failed to create string");
                return nullptr;
            }
        }

        return &it->first;
    }

    int ReleaseStringConstant(const void* str) override
    {
        auto* ptr = static_cast<const std::string*>(str);

        if(!ptr) [[unlikely]]
            return AS_NAMESPACE_QUALIFIER asERROR;

        int r = AS_NAMESPACE_QUALIFIER asSUCCESS;

        std::lock_guard lock(as_exclusive_lock);

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

        if(length)
            *length = static_cast<asUINT>(ptr->size());
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

    // The script array can be customized by defining a macro,
    // so keep the code for split_*(), which are related to script array,
    // in the header file for avoiding ODR-violation.

    namespace detail
    {
        inline script_array* new_string_array()
        {
            AS_NAMESPACE_QUALIFIER asIScriptContext* ctx = current_context();
            if(!ctx) [[unlikely]]
                return nullptr;

            auto* engine = ctx->GetEngine();

            using namespace meta;
            return new_script_array(engine, fixed_string("string"));
        }

        inline void string_split_impl(
            script_array& out, std::string_view str, std::string_view delimiter, bool skip_empty
        )
        {
            auto result = str | std::views::split(delimiter);

            for(auto&& i : result)
            {
                if(skip_empty && i.empty())
                    continue;
                std::string s(i.begin(), i.end());
                out.push_back(&s);
            }
        }
    } // namespace detail

    inline script_array* string_split(
        const std::string& this_, const std::string& delimiter, bool skip_empty
    )
    {
        script_array* arr = detail::new_string_array();
        if(!arr) [[unlikely]]
            return nullptr;
        detail::string_split_impl(*arr, this_, delimiter, skip_empty);
        return arr;
    }

    inline script_array* string_split_ch(
        const std::string& this_, char32_t ch, bool skip_empty
    )
    {
        script_array* arr = detail::new_string_array();
        if(!arr) [[unlikely]]
            return nullptr;

        char buf[4];
        unsigned int size_bytes = utf8::u8_int_to_bytes(ch, buf);

        detail::string_split_impl(*arr, this_, std::string_view(buf, size_bytes), skip_empty);
        return arr;
    }

    inline script_array* string_split_simple(const std::string& this_, bool skip_empty)
    {
        script_array* arr = detail::new_string_array();
        if(!arr)
            return nullptr;
        detail::string_split_impl(*arr, this_, " ", skip_empty);
        return arr;
    }

    /**
     * @brief For each characters in the string
     *
     * @param fn Script function signature should be `void(char)`
     */
    void string_for_each(
        const std::string& this_, AS_NAMESPACE_QUALIFIER asIScriptFunction* fn
    );
} // namespace script_string

inline void register_std_string(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    bool as_default = true,
    bool generic = has_max_portability()
)
{
    auto helper = [engine, as_default]<bool UseGeneric>(std::bool_constant<UseGeneric>)
    {
        using std::string;
        using namespace script_string;

        bool has_ch_api = engine->GetTypeIdByDecl("char") >= 0;

        AS_NAMESPACE_QUALIFIER asQWORD flags = 0;
        if(has_ch_api)
            flags |= AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_MORE_CONSTRUCTORS;

        assert(
            engine->GetEngineProperty(AS_NAMESPACE_QUALIFIER asEP_STRING_ENCODING) == 0 &&
            "String extension requires UTF-8 encoding"
        );

        value_class<string, UseGeneric> c(engine, "string", flags);
        c
            .behaviours_by_traits()
            .opEquals()
            .opCmp()
            .opAdd()
            .method("string append(const string&in str) const", fp<&string_append>)
            .method("string prepend(const string&in str) const", fp<&string_prepend>)
            .method("string substr(int pos, uint len=uint(-1)) const", fp<&string_substr>)
            .method("bool empty() const", fp<&string::empty>)
            .method(
                "bool opConv() const",
                [](const string& this_) -> bool
                { return !this_.empty(); }
            )
            .method(
                "uint get_size_bytes() const property",
                [](const string& this_)
                {
                    return static_cast<size_type>(this_.size());
                }
            )
            .method("uint get_size() const property", fp<&string_size>)
            .method(
                "bool starts_with(const string&in str) const",
                [](const string& this_, const string& str) -> bool
                { return this_.starts_with(str); }
            )
            .method(
                "bool ends_with(const string&in str) const",
                [](const string& this_, const string& str) -> bool
                { return this_.ends_with(str); }
            )
            .method("string remove_prefix(uint n) const", fp<&string_remove_prefix>)
            .method("string remove_suffix(uint n) const", fp<&string_remove_suffix>)
            .method("string replace(int where, uint n, const string&in str, int pos=0, uint len=uint(-1)) const", fp<&string_replace>)
            .method("string insert(int where, const string&in str, int pos=0, uint len=uint(-1)) const", fp<&string_insert>)
            .method("string erase(int where, uint n=1) const", fp<&string_erase>)
            .method(
                "uint64 hash() const",
                [](const std::string& this_) -> std::uint64_t
                { return std::hash<std::string>{}(this_); }
            )
            .method("bool contains(const string&in str) const", fp<&string_contains>);

        if(has_ch_api)
        { // Begin APIs for single character
            c
                .constructor_function(
                    "uint count, char ch",
                    [](void* mem, size_type count, char32_t ch) -> void
                    {
                        new(mem) string(string_construct(count, ch));
                    }
                )
                .method("string append(char ch) const", fp<&string_append_ch>)
                .method("string opAdd(char ch) const", fp<&string_append_ch>)
                .method("string prepend(char ch) const", fp<&string_prepend_ch>)
                .method("string opAdd_r(char ch) const", fp<&string_prepend_ch>)
                .method("bool starts_with(char ch) const", fp<&string_starts_with_ch>)
                .method("bool ends_with(char ch) const", fp<&string_ends_with_ch>)
                .method("char opIndex(int idx) const", fp<&string_opIndex>)
                .method("bool contains(char ch) const", fp<&string_contains_ch>)
                .funcdef("void for_each_callback(char ch)")
                .method("void for_each(const for_each_callback&in fn)", fp<&string_for_each>);
        }

        if(engine->GetDefaultArrayTypeId() >= 0)
        {
            c
                .method("array<string>@ split(bool skip_empty=true) const", fp<&string_split_simple>)
                .method("array<string>@ split(const string&in delimiter, bool skip_empty=true) const", fp<&string_split>);
            if(has_ch_api)
                c.method("array<string>@ split(char delimiter, bool skip_empty=true) const", fp<&string_split_ch>);
        };

        if(as_default)
            c.as_string(&string_factory::get());
    };

    if(generic)
        helper(std::true_type{});
    else
        helper(std::false_type{});
}

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

void register_string_utils(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    bool generic = has_max_portability()
);
} // namespace asbind20::ext

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
