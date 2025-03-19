#include <asbind20/ext/stdstring.hpp>
#include <concepts>
#include <cstdint>
#include <charconv>

namespace asbind20::ext
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

void register_script_char(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, bool generic
)
{
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
        std::size_t offset;
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
        std::size_t offset;
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
        return string_concat(this_, str);
    }

    std::string string_append_ch(const std::string& this_, char32_t ch)
    {
        char buf[4];
        unsigned int size_bytes = utf8::u8_int_to_bytes(ch, buf);
        return string_concat(this_, std::string_view(buf, size_bytes));
    }

    std::string string_prepend(const std::string& this_, const std::string& str)
    {
        return string_concat(str, this_);
    }

    std::string string_prepend_ch(const std::string& this_, char32_t ch)
    {
        char buf[4];
        unsigned int size_bytes = utf8::u8_int_to_bytes(ch, buf);

        return string_concat(std::string_view(buf, size_bytes), this_);
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
        try
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

            return std::move(result);
        }
        catch(const std::out_of_range&)
        {
            set_script_exception("string.replace(): out of range");
            return std::string();
        }
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
                return string_concat(src, this_);
            else
                return string_concat(this_, src);
        }

        std::string result = this_;
        result.insert(offset, src);
        return std::move(result);
    }

    std::string string_erase(
        const std::string& this_, index_type where, size_type n
    )
    {
        std::size_t offset = idx_to_offset(this_, where);
        if(offset == size_type(-1))
        {
            set_script_exception("out of range");
        }
        if(n == 0)
            return this_;

        std::string result = this_;
        std::size_t bytes_to_erase = utf8::u8_index(
            std::string_view(this_).substr(offset), n
        );
        result.erase(offset, bytes_to_erase);
        return std::move(result);
    }

    void string_for_each(
        const std::string& this_, AS_NAMESPACE_QUALIFIER asIScriptFunction* fn
    )
    {
        reuse_active_context ctx(fn->GetEngine());
        const auto sentinel = utf8::string_cend(this_);
        for(auto it = utf8::string_cbegin(this_); it != sentinel; ++it)
        {
            auto result = script_invoke<void>(ctx, fn, *it);
            if(!result.has_value())
                break;
        }
    }
} // namespace script_string

static std::string script_bool_to_string(bool val)
{
    using namespace std::string_literals;
    return val ? "true"s : "false"s;
}

template <std::integral Int>
static std::string script_int_to_string(Int val, int base)
{
    char buf[128];
    auto result = std::to_chars(buf, buf + 128, val);
    assert(result.ec != std::errc::value_too_large);
    if(result.ec != std::errc())
    {
        set_script_exception(string_concat(
            "to_string<", name_of<Int>(), ">(): error"
        ));
    }

    return std::string(buf, result.ptr);
}

template <typename Float>
static std::string script_float_to_string(Float val, std::chars_format fmt)
{
    char buf[128];
    auto result = std::to_chars(buf, buf + 128, val, fmt);
    assert(result.ec != std::errc::value_too_large);
    if(result.ec != std::errc())
    {
        set_script_exception(string_concat(
            "to_string<", name_of<Float>(), ">(): error"
        ));
    }

    return std::string(buf, result.ptr);
}

template <bool UseGeneric>
static void register_string_utils_impl(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine
)
{
    enum_<std::chars_format>(engine, "float_format")
        .value(std::chars_format::scientific, "scientific")
        .value(std::chars_format::hex, "hex")
        .value(std::chars_format::general, "general")
        .value(std::chars_format::fixed, "fixed");

    global<UseGeneric> g(engine);
    g
        .function("string to_string(bool val)", fp<&script_bool_to_string>)
        .function("string to_string(int val, int base=10)", fp<&script_int_to_string<AS_NAMESPACE_QUALIFIER asINT32>>)
        .function("string to_string(uint val, int base=10)", fp<&script_int_to_string<AS_NAMESPACE_QUALIFIER asUINT>>)
        .function("string to_string(int64 val, int base=10)", fp<&script_int_to_string<std::int64_t>>)
        .function("string to_string(uint64 val, int base=10)", fp<&script_int_to_string<std::uint64_t>>)
        .function("string to_string(float val, float_format fmt=float_format::general)", fp<&script_float_to_string<float>>)
        .function("string to_string(double val, float_format fmt=float_format::general)", fp<&script_float_to_string<double>>);

    if(engine->GetTypeIdByDecl("char") >= 0)
    {
        g.function("string chr(char ch)", fp<&script_chr>);
    }
}

void register_string_utils(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, bool generic
)
{
    if(generic)
        register_string_utils_impl<true>(engine);
    else
        register_string_utils_impl<false>(engine);
}
} // namespace asbind20::ext
