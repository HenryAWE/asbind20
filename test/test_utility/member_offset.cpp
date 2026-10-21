#include <cstddef>
#include <memory>
#include <gtest/gtest.h>
#include <asbind20/asbind.hpp>

namespace
{
struct simple
{
    int a;
    int b;
};

struct padding
{
    char c;
    long long ll;
    int i;
};

// Class with overloaded address-of operator
struct tricky_member
{
    int data;

    tricky_member* operator&() noexcept
    {
        return nullptr;
    }

    const tricky_member* operator&() const noexcept
    {
        return nullptr;
    }
};

struct has_tricky_member
{
    tricky_member member;
    int after;
};

struct empty
{};

struct has_empty_member
{
    [[no_unique_address]] empty e;
    int n;
};

struct aligned_type
{
    alignas(64) char c;
    int i;
};

// Non-standard layout types

struct polymorphic
{
    virtual ~polymorphic() = default;

    int v;
};

struct base_a
{
    int a;
};

struct base_b
{
    int b;
};

struct multiple : base_a, base_b
{
    int c;
};

struct virtual_base
{
    int vb;
};

struct with_virtual_base : virtual virtual_base
{
    int d;
};

struct abstract_type
{
    virtual void pure() = 0;

    int ax;
};

struct concrete_type : abstract_type
{
    void pure() override {}
};

union union_type
{
    int a;
    char c[4];
};

struct has_const_member
{
    int before;
    const int ci;
};

template <typename Class, typename Member>
std::size_t actual_member_offset(Class& obj, Member Class::* mp)
{
    auto* base = reinterpret_cast<const char*>(std::addressof(obj));
    auto* address = reinterpret_cast<const char*>(std::addressof(obj.*mp));
    return static_cast<std::size_t>(address - base);
}
} // namespace

// Compile-time assertions for standard layout classes

static_assert(asbind20::member_offset(&simple::a) == offsetof(simple, a));
static_assert(asbind20::member_offset(&simple::b) == offsetof(simple, b));

static_assert(asbind20::member_offset(&padding::c) == offsetof(padding, c));
static_assert(asbind20::member_offset(&padding::ll) == offsetof(padding, ll));
static_assert(asbind20::member_offset(&padding::i) == offsetof(padding, i));

static_assert(asbind20::member_offset(&has_tricky_member::member) == 0);
static_assert(asbind20::member_offset(&has_tricky_member::after) == sizeof(tricky_member));

static_assert(asbind20::member_offset(&has_empty_member::e) == 0);
static_assert(asbind20::member_offset(&has_empty_member::n) == offsetof(has_empty_member, n));

static_assert(asbind20::member_offset(&aligned_type::c) == 0);
static_assert(asbind20::member_offset(&aligned_type::i) == 4);

static_assert(asbind20::member_offset(&union_type::a) == 0);
static_assert(asbind20::member_offset(&union_type::c) == 0);

static_assert(asbind20::member_offset(&has_const_member::before) == 0);
static_assert(asbind20::member_offset(&has_const_member::ci) == sizeof(int));

static_assert(asbind20::member_offset(&multiple::c) == sizeof(base_a) + sizeof(base_b));

constexpr std::size_t polymorphic_offset = asbind20::member_offset(&polymorphic::v);
constexpr std::size_t virtual_member_offset = asbind20::member_offset(&with_virtual_base::d);

// Composite wrapper

constexpr asbind20::composite_wrapper composite_nontype = asbind20::composite<&simple::b>();
static_assert(composite_nontype.get_offset() == offsetof(simple, b));

constexpr asbind20::composite_wrapper composite_member_pointer = asbind20::composite(&simple::b);
static_assert(composite_member_pointer.get_offset() == offsetof(simple, b));

constexpr asbind20::composite_wrapper composite_offset = asbind20::composite<offsetof(simple, b)>();
static_assert(composite_offset.get_offset() == offsetof(simple, b));

static consteval bool test_member_offset_constexpr()
{
    struct local_type
    {
        int a;
        int b;
        int c;
    };

    static_assert(asbind20::member_offset(&local_type::a) == 0);
    static_assert(asbind20::member_offset(&local_type::b) == sizeof(int));
    static_assert(asbind20::member_offset(&local_type::c) == 2 * sizeof(int));

    static_assert(asbind20::member_offset(&local_type::c) == offsetof(local_type, c));

    using asbind20::composite;
    using asbind20::composite_wrapper;

    static_assert(
        static_cast<composite_wrapper>(composite<&local_type::b>()).get_offset() == sizeof(int)
    );
    static_assert(
        composite(&local_type::c).get_offset() == 2 * sizeof(int)
    );

    return true;
}

static_assert(test_member_offset_constexpr());

TEST(MemberOffset, StandardLayout)
{
    simple obj{};
    EXPECT_EQ(asbind20::member_offset(&simple::a), actual_member_offset(obj, &simple::a));
    EXPECT_EQ(asbind20::member_offset(&simple::b), actual_member_offset(obj, &simple::b));

    padding pad{};
    EXPECT_EQ(asbind20::member_offset(&padding::c), actual_member_offset(pad, &padding::c));
    EXPECT_EQ(asbind20::member_offset(&padding::ll), actual_member_offset(pad, &padding::ll));
    EXPECT_EQ(asbind20::member_offset(&padding::i), actual_member_offset(pad, &padding::i));
}

TEST(MemberOffset, NonStandardLayout)
{
    polymorphic poly;
    EXPECT_EQ(asbind20::member_offset(&polymorphic::v), actual_member_offset(poly, &polymorphic::v));
    EXPECT_EQ(polymorphic_offset, actual_member_offset(poly, &polymorphic::v));

    multiple multi{};
    auto& a_base = static_cast<base_a&>(multi);
    auto& b_base = static_cast<base_b&>(multi);
    // The type of `&multiple::a` is `int base_a::*`, so the offset is relative to `base_a`
    EXPECT_EQ(asbind20::member_offset(&multiple::a), actual_member_offset(a_base, &base_a::a));
    EXPECT_EQ(asbind20::member_offset(&multiple::b), actual_member_offset(b_base, &base_b::b));
    EXPECT_EQ(asbind20::member_offset(&multiple::c), actual_member_offset(multi, &multiple::c));

    // Converted pointer to member: the offset is now relative to the derived class
    EXPECT_EQ(
        asbind20::member_offset(static_cast<int multiple::*>(&multiple::a)),
        actual_member_offset(multi, static_cast<int multiple::*>(&multiple::a))
    );

    with_virtual_base vobj{};
    EXPECT_EQ(
        asbind20::member_offset(&with_virtual_base::d),
        actual_member_offset(vobj, &with_virtual_base::d)
    );
    EXPECT_EQ(virtual_member_offset, actual_member_offset(vobj, &with_virtual_base::d));

    auto& vbase = static_cast<virtual_base&>(vobj);
    EXPECT_EQ(
        asbind20::member_offset(&virtual_base::vb),
        actual_member_offset(vbase, &virtual_base::vb)
    );
}

TEST(MemberOffset, AbstractClass)
{
    concrete_type obj;
    auto& base = static_cast<abstract_type&>(obj);

    EXPECT_EQ(
        asbind20::member_offset(&abstract_type::ax),
        actual_member_offset(base, &abstract_type::ax)
    );
}

TEST(MemberOffset, Composite)
{
    simple obj{};

    EXPECT_EQ(
        static_cast<asbind20::composite_wrapper>(asbind20::composite<&simple::b>()).get_offset(),
        actual_member_offset(obj, &simple::b)
    );

    multiple multi{};
    EXPECT_EQ(
        asbind20::composite(&multiple::c).get_offset(),
        actual_member_offset(multi, &multiple::c)
    );
}
