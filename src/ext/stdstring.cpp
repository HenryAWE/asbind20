#include <asbind20/ext/stdstring.hpp>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <charconv>
#include <string>
#include <string_view>
#ifdef ASBIND20_EXT_ARRAY
#    include <ranges>
#    include <asbind20/ext/array.hpp>
#endif

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

        if((ch & 0b1111'1000) == 0b1111'0000)
            i += 4;
        else if((ch & 0b1111'0000) == 0b1110'0000)
            i += 3;
        else if((ch & 0b1110'0000) == 0b1100'0000)
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

        if((ch & 0b1111'1000) == 0b1111'0000)
            i += 4;
        else if((ch & 0b1111'0000) == 0b1110'0000)
            i += 3;
        else if((ch & 0b1110'0000) == 0b1100'0000)
            i += 2;
        else
            ++i;

        ++result;
    }

    return result;
}

char32_t u8_bytes_to_int(const char* str)
{
    const std::uint8_t* bytes = reinterpret_cast<const std::uint8_t*>(str);

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

static asUINT string_size_bytes(const std::string& this_)
{
    return static_cast<asUINT>(this_.size());
}

static asUINT string_size(const std::string& this_)
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

static void string_prepend(std::string& this_, const std::string& str)
{
    this_.insert(0, str);
}

void string_prepend_ch(std::string& this_, std::uint32_t ch)
{
    char buf[4];
    unsigned int size_bytes = u8_int_to_bytes(ch, buf);
    this_.insert(0, std::string_view(buf, size_bytes));
}

static std::string string_opAdd_ch(const std::string& lhs, std::uint32_t rhs)
{
    std::string tmp = lhs;
    string_append_ch(tmp, rhs);
    return tmp;
}

static std::string string_opAdd_r_ch(std::uint32_t lhs, const std::string& rhs)
{
    std::string tmp = rhs;
    string_prepend_ch(tmp, lhs);
    return tmp;
}

void u8_replace(std::string& target, std::size_t idx, std::size_t n, std::string_view str)
{
    std::string_view view(target);

    std::size_t target_start = u8_index(view, idx);
    if(target_start == -1)
    {
        throw std::out_of_range("out of range");
    }
    if(n == 0)
        return;
    std::size_t target_stop = u8_index(view.substr(target_start), n);

    target.replace(
        target_start,
        target_stop - target_start,
        str
    );
}

static void string_replace(std::string& this_, asINT32 idx, asUINT n, const std::string& str, asUINT len)
{
    try
    {
        u8_replace(this_, idx, n, u8_substr(str, 0, len));
    }
    catch(const std::out_of_range& e)
    {
        set_script_exception(e.what());
    }
}

static void string_insert(std::string& this_, asINT32 idx, const std::string& str, asUINT len)
{
    std::size_t offset = u8_index(this_, idx);
    if(offset == -1)
    {
        set_script_exception("out of range");
        return;
    }

    this_.insert(offset, u8_substr(str, 0, len));
}

static void string_erase(std::string& this_, asINT32 idx, asUINT n)
{
    std::size_t start = u8_index(this_, idx);
    if(start == -1)
    {
        set_script_exception("out of range");
    }
    if(n == 0)
        return;

    std::size_t bytes_to_erase = u8_index(
        std::string_view(this_).substr(start), n
    );
    this_.erase(start, bytes_to_erase);
}

std::uint64_t string_hash(const std::string& this_)
{
    return std::hash<std::string>{}(this_);
}

bool string_contains(const std::string& this_, const std::string& str)
{
    return this_.find(str) != this_.npos;
}

bool string_contains_ch(const std::string& this_, std::uint32_t ch)
{
    char buf[4];
    unsigned int size_bytes = u8_int_to_bytes(ch, buf);
    return this_.find(std::string_view(buf, size_bytes)) != this_.npos;
}

#ifdef ASBIND20_EXT_ARRAY

static script_array* new_string_array()
{
    asIScriptContext* ctx = asGetActiveContext();
    if(!ctx) [[unlikely]]
        return nullptr;

    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* ti = engine->GetTypeInfoByDecl("array<string>");

    return new script_array(ti);
}

static void string_split_impl(script_array& out, std::string_view str, std::string_view delimiter, bool skip_empty)
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

static script_array* string_split(const std::string& this_, const std::string& delimiter, bool skip_empty)
{
    script_array* arr = new_string_array();
    if(!arr)
        return nullptr;
    string_split_impl(*arr, this_, delimiter, skip_empty);
    return arr;
}

static script_array* string_split_ch(const std::string& this_, std::uint32_t ch, bool skip_empty)
{
    script_array* arr = new_string_array();
    if(!arr)
        return nullptr;

    char buf[4];
    unsigned int size_bytes = u8_int_to_bytes(ch, buf);

    string_split_impl(*arr, this_, std::string_view(buf, size_bytes), skip_empty);
    return arr;
}

static script_array* string_split_simple(const std::string& this_, bool skip_empty)
{
    script_array* arr = new_string_array();
    if(!arr)
        return nullptr;
    string_split_impl(*arr, this_, " ", skip_empty);
    return arr;
}

#endif

void register_std_string(asIScriptEngine* engine, bool as_default)
{
    using std::string;

    value_class<string> c(engine, "string");
    c
        .common_behaviours()
        .opEquals()
        .opCmp()
        .opAdd()
        .method("void append(const string &in)", &string_append)
        .opAddAssign()
        .method("void prepend(const string&in)", &string_prepend)
        .method("string substr(int pos, uint len=-1) const", &string_substr)
        .method("bool get_empty() const property", &string::empty)
        .method("uint get_size_bytes() const property", &string_size_bytes)
        .method("uint get_size() const property", &string_size)
        .method("void clear()", &string::clear)
        .method("bool starts_with(const string&in str) const", &string_starts_with)
        .method("bool ends_with(const string&in str) const", &string_ends_with)
        .method("string remove_prefix(uint n) const", &string_remove_prefix)
        .method("string remove_suffix(uint n) const", &string_remove_suffix)
        .method("void replace(int idx, uint n, const string&in str, uint len=-1)", &string_replace)
        .method("void insert(int idx, const string&in str, uint len=-1)", &string_insert)
        .method("void erase(int idx, uint n=1)", &string_erase)
        .method("uint64 get_hash() const property", &string_hash)
        .method("bool contains(const string&in) const", &string_contains);

    if(engine->GetEngineProperty(asEP_USE_CHARACTER_LITERALS))
    {
        c
            .method("void append(uint ch)", &string_append_ch)
            .method("string opAdd(uint ch) const", &string_opAdd_ch)
            .method("void prepend(uint ch)", &string_prepend_ch)
            .method("string opAdd_r(uint ch) const", &string_opAdd_r_ch)
            .method("bool starts_with(uint ch) const", &string_starts_with_ch)
            .method("bool ends_with(uint ch) const", &string_ends_with_ch)
            .method("uint get_opIndex(int idx) const property", &string_get_opIndex)
            .method("void set_opIndex(int idx, uint ch) property", &string_set_opIndex)
            .method("bool contains(uint ch) const", &string_contains_ch);
    }


#ifdef ASBIND20_EXT_ARRAY

    if(engine->GetDefaultArrayTypeId() >= 0)
    {
        c
            .method("array<string>@ split(bool skip_empty=true) const", &string_split_simple)
            .method("array<string>@ split(const string&in delimiter, bool skip_empty=true) const", &string_split);
        if(engine->GetEngineProperty(asEP_USE_CHARACTER_LITERALS))
        {
            c.method("array<string>@ split(uint delimiter, bool skip_empty=true) const", &string_split_ch);
        }
    }

#endif

    if(as_default)
    {
        int r = engine->RegisterStringFactory("string", &string_factory::get());
        assert(r >= 0);
    }
}

static std::string as_bool_to_string(bool val)
{
    return val ? "true" : "false";
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
        .function("string to_string(bool val)", &as_bool_to_string)
        .function("string to_string(int val, int base=10)", &as_int_to_string<int>)
        .function("string to_string(uint val, int base=10)", &as_int_to_string<asUINT>)
        .function("string to_string(int64 val, int base=10)", &as_int_to_string<std::int64_t>)
        .function("string to_string(uint64 val, int base=10)", &as_int_to_string<std::uint64_t>)
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
