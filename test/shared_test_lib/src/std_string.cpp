#include <asbind_test/std_string.hpp>

#include "asbind20/bind/function_tools.hpp"

#include <asbind_test/framework.hpp>

namespace asbind_test
{
void configure_engine_for_ext_string(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine
)
{
    engine->SetEngineProperty(
        AS_NAMESPACE_QUALIFIER asEP_USE_CHARACTER_LITERALS, true
    );
    engine->SetEngineProperty(
        AS_NAMESPACE_QUALIFIER asEP_SCRIPT_SCANNER, 1 // UTF-8
    );
    engine->SetEngineProperty(
        AS_NAMESPACE_QUALIFIER asEP_STRING_ENCODING, 0 // UTF-8
    );
}

void setup_script_char(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, bool generic
)
{
    using namespace asbind20;

    constexpr AS_NAMESPACE_QUALIFIER asQWORD char_flags =
        AS_NAMESPACE_QUALIFIER asOBJ_APP_PRIMITIVE |
        AS_NAMESPACE_QUALIFIER asOBJ_POD;

    auto helper = [engine]<bool UseGeneric>(std::bool_constant<UseGeneric>)
    {
        value_class<char32_t, UseGeneric>(engine, "char", char_flags)
            .constructor_function(
                "uint",
                [](void* mem, std::uint32_t val)
                {
                    new(mem) char32_t(val);
                }
            )
            .opAssign()
            .opEquals()
            .opCmp()
            .opAdd()
            .opAddAssign()
            .opSub()
            .opSubAssign()
            .template opConv<std::uint32_t>();
    };

    if(generic)
        helper(std::true_type{});
    else
        helper(std::false_type{});
}

namespace script_string
{
    static std::size_t idx_to_offset(std::string_view str, index_type idx)
    {
        if(idx < 0)
        {
            size_type u_idx = static_cast<size_type>(-idx);
            return utf8::u8_index_r(str, u_idx);
        }
        else
        {
            return utf8::u8_index(str, static_cast<size_type>(idx));
        }
    }

    static std::string_view substr_helper(std::string_view raw_src, index_type pos, size_type len)
    {
        if(pos < 0)
        {
            size_type u_pos = static_cast<size_type>(-pos);
            return utf8::u8_substr_r(
                raw_src, u_pos, len
            );
        }
        else
        {
            return utf8::u8_substr(
                raw_src, static_cast<size_type>(pos), len
            );
        }
    }

    std::string string_construct(size_type count, char32_t ch)
    {
        char buf[4];
        unsigned int size_bytes = utf8::u8_int_to_bytes(ch, buf);
        std::string result;
        result.reserve(count * size_bytes);
        for(size_type i = 0; i < count; ++i)
            result += std::string_view(buf, size_bytes);

        return result;
    }

    size_type string_size(const std::string& this_)
    {
        return static_cast<size_type>(utf8::u8_strlen(this_));
    }

    std::string string_append(const std::string& this_, const std::string& str)
    {
        return asbind20::string_concat(this_, str);
    }

    std::string string_append_ch(const std::string& this_, char32_t ch)
    {
        char buf[4];
        unsigned int size_bytes = utf8::u8_int_to_bytes(ch, buf);
        return asbind20::string_concat(this_, std::string_view(buf, size_bytes));
    }

    std::string string_prepend(const std::string& this_, const std::string& str)
    {
        return asbind20::string_concat(str, this_);
    }

    std::string string_prepend_ch(const std::string& this_, char32_t ch)
    {
        char buf[4];
        unsigned int size_bytes = utf8::u8_int_to_bytes(ch, buf);

        return asbind20::string_concat(std::string_view(buf, size_bytes), this_);
    }

    std::string string_remove_prefix(const std::string& this_, size_type n)
    {
        if(n == 0)
            return this_;
        return std::string(utf8::u8_remove_prefix(this_, n));
    }

    std::string string_remove_suffix(const std::string& this_, size_type n)
    {
        if(n == 0)
            return this_;
        return std::string(utf8::u8_remove_suffix(this_, n));
    }

    char32_t string_opIndex(const std::string& this_, index_type idx)
    {
        std::string_view sv = this_;
        std::size_t offset = idx_to_offset(this_, idx);
        if(offset == std::size_t(-1))
            return U'\0';
        return utf8::u8_bytes_to_int(sv.substr(offset).data());
    }

    bool string_starts_with_ch(const std::string& this_, char32_t ch)
    {
        char buf[4];
        std::size_t size_bytes = utf8::u8_int_to_bytes(ch, buf);
        return this_.starts_with(std::string_view(buf, size_bytes));
    }

    bool string_ends_with_ch(const std::string& this_, char32_t ch)
    {
        char buf[4];
        std::size_t size_bytes = utf8::u8_int_to_bytes(ch, buf);
        return this_.ends_with(std::string_view(buf, size_bytes));
    }

    bool string_contains(const std::string& this_, const std::string& str)
    {
        return this_.find(str) != this_.npos;
    }

    bool string_contains_ch(const std::string& this_, char32_t ch)
    {
        char buf[4];
        unsigned int size_bytes = utf8::u8_int_to_bytes(ch, buf);
        return this_.find(std::string_view(buf, size_bytes)) != this_.npos;
    }

    std::string string_substr(const std::string& this_, int pos, size_type n)
    {
        return std::string(substr_helper(this_, pos, n));
    }

    std::string string_replace(
        const std::string& this_,
        index_type where,
        size_type n,
        const std::string& str,
        index_type pos,
        size_type len
    )
    {
#ifndef ASBIND20_NO_EXCEPTIONS
        try
#endif
        {
            std::string_view src = substr_helper(str, pos, len);

            std::string result = this_;
            if(where < 0)
            {
                size_type u_where = static_cast<size_type>(-where);
                utf8::u8_replace_inplace_r(result, u_where, n, src);
            }
            else
            {
                utf8::u8_replace_inplace(result, static_cast<size_type>(where), n, src);
            }

            return result;
        }
#ifndef ASBIND20_NO_EXCEPTIONS
        catch(const std::out_of_range&)
        {
            asbind20::set_script_exception("string.replace(): out of range");
            return std::string();
        }
#endif
    }

    std::string string_insert(
        const std::string& this_,
        index_type where,
        const std::string& str,
        index_type pos,
        size_type len
    )
    {
        std::size_t offset = idx_to_offset(this_, where);

        std::string_view src = substr_helper(str, pos, len);
        if(offset == size_type(-1)) [[unlikely]]
        {
            if(where < 0)
                return asbind20::string_concat(src, this_);
            else
                return asbind20::string_concat(this_, src);
        }

        std::string result = this_;
        result.insert(offset, src);
        return result;
    }

    std::string string_erase(
        const std::string& this_, index_type where, size_type n
    )
    {
        std::size_t offset = idx_to_offset(this_, where);
        if(offset == size_type(-1))
        {
            asbind20::set_script_exception("out of range");
        }
        if(n == 0)
            return this_;

        std::string result = this_;
        std::size_t bytes_to_erase = utf8::u8_index(
            std::string_view(this_).substr(offset), n
        );
        result.erase(offset, bytes_to_erase);
        return result;
    }
} // namespace script_string

template <bool UseGeneric>
static void setup_script_string_impl(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    bool as_default = true
)
{
    using namespace asbind20;

    using std::string;
    using namespace script_string;

    bool has_ch_api = engine->GetTypeIdByDecl("char") >= 0;

    AS_NAMESPACE_QUALIFIER asQWORD flags = 0;
    if(has_ch_api)
        flags |= AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_MORE_CONSTRUCTORS;

    EXPECT_EQ(
        engine->GetEngineProperty(AS_NAMESPACE_QUALIFIER asEP_STRING_ENCODING), 0
    ) << "String extension requires UTF-8 encoding!";

    value_class<string, UseGeneric> c(engine, "string", flags);
    c
        .behaviours_by_traits()
        .opEquals()
        .opCmp(
            [](const std::string& lhs, const std::string& rhs) -> int
            {
                return lhs.compare(rhs);
            }
        )
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
        .method("bool contains(const string&in str) const", fp<&string_contains>);

    if(has_ch_api)
    { // Begin APIs for single character
        c
            .constructor_function(
                "uint count, char ch",
                fn_tools::as_ctor_fn<&string_construct>()
            )
            .method("string append(char ch) const", fp<&string_append_ch>)
            .method("string opAdd(char ch) const", fp<&string_append_ch>)
            .method("string prepend(char ch) const", fp<&string_prepend_ch>)
            .method("string opAdd_r(char ch) const", fp<&string_prepend_ch>)
            .method("bool starts_with(char ch) const", fp<&string_starts_with_ch>)
            .method("bool ends_with(char ch) const", fp<&string_ends_with_ch>)
            .method("char opIndex(int idx) const", fp<&string_opIndex>)
            .method("bool contains(char ch) const", fp<&string_contains_ch>);
    }

    if(as_default)
        c.as_string(&string_factory::get());
}

void setup_script_string(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    bool generic,
    bool as_default
)
{
    if(generic)
        setup_script_string_impl<true>(engine, as_default);
    else
        setup_script_string_impl<false>(engine, as_default);
}

void setup_string_utils(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    bool generic
)
{
    using namespace asbind20;

    if(engine->GetTypeInfoByName("char"))
        return;

    auto helper = [engine]<bool UseGeneric>(std::bool_constant<UseGeneric>)
    {
        global<UseGeneric>(engine)
            .function("string chr(char ch)", fp<&script_chr>);
    };

    if(generic)
        helper(std::true_type{});
    else
        helper(std::false_type{});
}
} // namespace asbind_test
