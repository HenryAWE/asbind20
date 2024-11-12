#ifndef ASBIND20_EXT_VOCABULARY_HPP
#define ASBIND20_EXT_VOCABULARY_HPP

#pragma once

#include <asbind20/asbind.hpp>

namespace asbind20::ext
{
namespace detail
{
    template <bool UseGeneric>
    void register_script_optional_impl(asIScriptEngine* engine);
}

void register_script_optional(asIScriptEngine* engine, bool generic = has_max_portability());

class script_optional
{
    template <bool UseGeneric>
    friend void detail::register_script_optional_impl(asIScriptEngine* engine);

public:
    script_optional(asITypeInfo* ti);

    script_optional(asITypeInfo* ti, const void* value);

private:
    ~script_optional();

public:
    script_optional& operator=(const script_optional& other);

    bool operator==(const script_optional& rhs) const;

    void assign(const void* val);

    void* operator new(std::size_t bytes);
    void operator delete(void* mem);

    bool has_value() const noexcept
    {
        return m_has_value;
    }

    void* value();
    const void* value() const;

    void* value_or(void* val);
    const void* value_or(const void* val) const;

    void reset();

    explicit operator bool() const
    {
        return has_value();
    }

    asITypeInfo* get_type_info() const
    {
        return m_ti;
    }

private:
    asITypeInfo* m_ti = nullptr;

    union data_t
    {
    private:
        void* ptr;
        std::byte storage[8];

        static bool use_storage(asITypeInfo* ti);

    public:
        bool copy_construct(asITypeInfo* ti, const void* val);
        void destruct(asITypeInfo* ti);

        void* get(asITypeInfo* ti);
        const void* get(asITypeInfo* ti) const;
    };

    alignas(alignof(double)) data_t m_data;
    bool m_has_value = false;

    void release();
};
} // namespace asbind20::ext

#endif
