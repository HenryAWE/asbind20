#include <asbind20/ext/stdstring.hpp>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <charconv>
#include <string>
#include <string_view>

namespace asbind20::ext
{
std::size_t u8_index(std::string_view str, std::size_t n) noexcept
{
    std::size_t i = 0;
    std::size_t count = 0;

    while(i < str.size())
    {
        if(count == n)
            return i;

        std::uint8_t ch = str[i];

        if((ch & 0b1111'1000) == 0b1110'0000)
            i += 4;
        else if((ch & 0b1110'0000) == 0b1100'0000)
            i += 3;
        else if((ch & 0b1100'0000) == 0b1000'0000)
            i += 2;
        else
            ++i;

        ++count;
    }

    return -1;
}

std::size_t u8_index_r(std::string_view str, std::size_t n) noexcept
{
    if(str.empty())
        return -1;

    std::size_t count = 0;

    std::size_t i = str.size() - 1;

    while(true)
    {
        std::uint8_t b = str[i];
        if((b & 0b1100'0000) != 0b1000'0000)
        {
            ++count;
            if(count == n)
                return i;
        }

        if(i == 0)
            break;
        --i;
    };

    return -1;
}

static unsigned int u8_bytes(char first) noexcept
{
    if((first & 0b1111'1000) == 0b1111'0000)
        return 4;
    else if((first & 0b1111'0000) == 0b1110'0000)
        return 3;
    else if((first & 0b1110'0000) == 0b1100'0000)
        return 2;
    else
        return 1;
}

std::size_t u8_strlen(std::string_view str) noexcept
{
    std::size_t i = 0;
    std::size_t result = 0;

    while(i < str.size())
    {
        std::uint8_t ch = str[i];

        if((ch & 0b1111'1000) == 0b1110'0000)
            i += 4;
        else if((ch & 0b1110'0000) == 0b1100'0000)
            i += 3;
        else if((ch & 0b1100'0000) == 0b1000'0000)
            i += 2;
        else
            ++i;

        ++result;
    }

    return result;
}

char32_t u8_bytes_to_int(const char* bytes)
{
    // ASCII (single byte)
    if(bytes[0] <= 0b0111'1111)
    {
        return bytes[0];
    }
    // 2 bytes
    else if(0b1100'0000 <= bytes[0] && bytes[0] <= 0b1101'1111)
    {
        char32_t result = U'\0';
        result |= bytes[0] & 0b0001'1111;
        result <<= 6;
        result |= bytes[1] & 0b0011'1111;

        return result;
    }
    // 3 bytes
    else if(0b1110'0000 <= bytes[0] && bytes[0] <= 0b1110'1111)
    {
        char32_t result = U'\0';
        result |= bytes[0] & 0b0000'1111;
        result <<= 6;
        result |= bytes[1] & 0b0011'1111;
        result <<= 6;
        result |= bytes[2] & 0b0011'1111;

        return result;
    }
    // 4 bytes
    else if(0b1111'0000 <= bytes[0] && bytes[0] <= 0b1111'0111)
    {
        char32_t result = U'\0';
        result |= bytes[0] & 0b0000'1111;
        result <<= 6;
        result |= bytes[1] & 0b0011'1111;
        result <<= 6;
        result |= bytes[2] & 0b0011'1111;
        result <<= 6;
        result |= bytes[3] & 0b0011'1111;

        return result;
    }

    return U'\0';
}

unsigned int u8_int_to_bytes(char32_t ch, char* buf)
{
    if(ch <= 0x7F)
    {
        buf[0] = static_cast<char>(ch);
        return 1;
    }
    else if(ch <= 0x7FF)
    {
        buf[1] = static_cast<char>((ch & 0b0011'1111) | 0b1000'0000);
        buf[0] = static_cast<char>((ch >> 6) | 0b1100'0000);
        return 2;
    }
    else if(ch <= 0xFFFF)
    {
        buf[2] = static_cast<char>((ch & 0b0011'1111) | 0b1000'0000);
        buf[1] = static_cast<char>(((ch >> 6) & 0b0011'1111) | 0b1000'0000);
        buf[0] = static_cast<char>((ch >> 12) | 0b1110'0000);
        return 3;
    }
    else if(ch <= 0x10FFFF)
    {
        buf[3] = static_cast<char>((ch & 0b0011'1111) | 0b1000'0000);
        buf[2] = static_cast<char>(((ch >> 6) & 0b0011'1111) | 0b1000'0000);
        buf[1] = static_cast<char>(((ch >> 12) & 0b0011'1111) | 0b1000'0000);
        buf[0] = static_cast<char>((ch >> 18) | 0b1111'0000);
        return 4;
    }

    return 0;
}

static int string_opCmp(const std::string& lhs, const std::string& rhs)
{
    return lhs.compare(rhs);
}

static std::string string_opAdd(const std::string& lhs, const std::string& rhs)
{
    return lhs + rhs;
}

static std::string string_opAdd_ch(const std::string& lhs, std::uint32_t ch)
{
    std::string tmp = lhs;
    string_append_ch(tmp, ch);
    return tmp;
}

static asUINT string_size(const std::string& this_)
{
    return static_cast<asUINT>(this_.size());
}

static asUINT string_length(const std::string& this_)
{
    return static_cast<asUINT>(u8_strlen(this_));
}

static std::uint32_t string_get_opIndex(const std::string& this_, asINT32 idx)
{
    if(idx < 0)
        return 0;

    std::string_view sv = this_;
    std::size_t offset = u8_index(sv, idx);
    if(offset == -1)
        return 0;
    return u8_bytes_to_int(sv.substr(offset).data());
}

static void string_set_opIndex(std::string& this_, asINT32 idx, std::uint32_t ch)
{
    if(idx < 0)
        return;

    std::size_t offset = u8_index(this_, idx);
    if(offset == -1)
        return;

    char buf[4];
    unsigned int size_bytes = u8_int_to_bytes(ch, buf);
    this_.replace(
        offset, u8_bytes(this_[offset]), std::string_view(buf, size_bytes)
    );
}

std::string_view u8_substr(std::string_view sv, std::size_t pos, std::size_t n)
{
    std::size_t idx = u8_index(sv, pos);
    if(idx == -1)
        return std::string_view();
    sv = sv.substr(idx);

    if(n == std::string_view::npos)
        return sv;
    idx = u8_index(sv, n);
    return sv.substr(0, idx);
}

static std::string string_substr(const std::string& this_, int pos, asUINT n)
{
    return std::string(u8_substr(this_, pos, n));
}

static bool string_starts_with(const std::string& this_, const std::string& str)
{
    return this_.starts_with(str);
}

static bool string_ends_with(const std::string& this_, const std::string& str)
{
    return this_.ends_with(str);
}

bool string_starts_with_ch(const std::string& this_, std::uint32_t ch)
{
    char buf[4];
    unsigned int size_bytes = u8_int_to_bytes(ch, buf);
    return this_.starts_with(std::string_view(buf, size_bytes));
}

bool string_ends_with_ch(const std::string& this_, std::uint32_t ch)
{
    char buf[4];
    unsigned int size_bytes = u8_int_to_bytes(ch, buf);
    return this_.ends_with(std::string_view(buf, size_bytes));
}

std::string_view u8_remove_prefix(std::string_view str, std::size_t n)
{
    std::size_t idx = u8_index(str, n);
    if(idx == -1)
        return std::string_view();

    return str.substr(idx);
}

std::string_view u8_remove_suffix(std::string_view str, std::size_t n)
{
    std::size_t idx = u8_index_r(str, n);
    if(idx == -1)
        return std::string_view();

    return str.substr(0, idx);
}

static std::string string_remove_prefix(const std::string& this_, asUINT n)
{
    return std::string(u8_remove_prefix(this_, n));
}

static std::string string_remove_suffix(const std::string& this_, asUINT n)
{
    return std::string(u8_remove_suffix(this_, n));
}

static void string_append(std::string& this_, const std::string& str)
{
    this_.append(str);
}

void string_append_ch(std::string& this_, std::uint32_t ch)
{
    char buf[4];
    unsigned int size_bytes = u8_int_to_bytes(ch, buf);
    this_.append(std::string_view(buf, size_bytes));
}

void register_std_string(asIScriptEngine* engine, bool as_default)
{
    using std::string;

    value_class<string> c(engine, "string", asOBJ_APP_CLASS_CDAK);
    c
        .common_behaviours()
        .opEquals()
        .method(
            "int opCmp(const string &in) const",
            &string_opCmp
        )
        .method(
            "void append(const string &in)",
            &string_append
        )
        .method(
            "string opAdd(const string &in) const",
            &string_opAdd
        )
        .opAddAssign()
        .method("string substr(int pos, uint len=-1) const", &string_substr)
        .method("bool empty() const", &string::empty)
        .method("uint size() const", &string_size)
        .method("uint length() const", &string_length)
        .method("void clear()", &string::clear)
        .method("bool starts_with(const string &in str) const", &string_starts_with)
        .method("bool ends_with(const string &in str) const", &string_ends_with)
        .method("string remove_prefix(uint n) const", &string_remove_prefix)
        .method("string remove_suffix(uint n) const", &string_remove_suffix);

    if(engine->GetEngineProperty(asEP_USE_CHARACTER_LITERALS))
    {
        c
            .method("void append(uint ch)", &string_append_ch)
            .method("string opAdd(uint ch) const", &string_opAdd_ch)
            .method("bool starts_with(uint ch) const", &string_starts_with_ch)
            .method("bool ends_with(uint ch) const", &string_ends_with_ch)
            .method("uint get_opIndex(int idx) const property", &string_get_opIndex)
            .method("void set_opIndex(int idx, uint ch) property", &string_set_opIndex);
    }

    if(as_default)
    {
        int r = engine->RegisterStringFactory("string", &string_factory::get());
        assert(r >= 0);
    }
}

template <std::integral T>
static std::string as_int_to_string(T val, int base)
{
    char buf[80];
    auto result = std::to_chars(buf, buf + 80, val);
    if(result.ec != std::errc())
    {
        asGetActiveContext()->SetException(
            std::make_error_code(result.ec).message().c_str()
        );
    }

    return std::string(buf, result.ptr);
}

static std::string as_float_to_string(float val, std::chars_format fmt)
{
    char buf[80];
    auto result = std::to_chars(buf, buf + 80, val, fmt);
    if(result.ec != std::errc())
    {
        asGetActiveContext()->SetException(
            std::make_error_code(result.ec).message().c_str()
        );
    }

    return std::string(buf, result.ptr);
}

static std::string as_chr(std::uint32_t ch)
{
    char buf[4];
    unsigned int size_bytes = u8_int_to_bytes(ch, buf);
    return std::string(buf, size_bytes);
}

void register_string_utils(asIScriptEngine* engine)
{
    global(engine)
        .function("string to_string(int val, int base=10)", &as_int_to_string<int>)
        .function("string to_string(uint val, int base=10)", &as_int_to_string<asUINT>)
        .enum_type("float_format")
        .enum_value("float_format", std::chars_format::scientific, "scientific")
        .enum_value("float_format", std::chars_format::hex, "hex")
        .enum_value("float_format", std::chars_format::general, "general")
        .enum_value("float_format", std::chars_format::fixed, "fixed")
        .function("string to_string(float val, float_format fmt=float_format::general)", &as_float_to_string);

    if(engine->GetEngineProperty(asEP_USE_CHARACTER_LITERALS))
    {
        global(engine)
            .function("string chr(uint ch)", &as_chr);
    }
}
} // namespace asbind20::ext
