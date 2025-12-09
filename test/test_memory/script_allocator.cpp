#include <asbind_test/framework.hpp>
#include <asbind20/memory.hpp>

namespace test_memory
{
consteval bool check_constant_expr(int val)
{
    using asbind20::as_allocator;
    std::vector<int, as_allocator<int>> v;

    v.push_back(val);
    return v[0] == 1013 && v.size() == 1;
}

static_assert(check_constant_expr(1013));
} // namespace test_memory

TEST(ScriptAllocator, CommonInterfaces)
{
    using asbind20::as_allocator;

    as_allocator<int> alloc;

    int* ptr = alloc.allocate(4);
    EXPECT_NE(ptr, nullptr);

    alloc.deallocate(ptr, 4);
}

#ifndef ASBIND20_NO_EXCEPTIONS

TEST(ScriptAllocator, CheckLength)
{
    using asbind20::as_allocator;

    struct big_type
    {
        std::int64_t dummy[4096];
    };

    as_allocator<big_type> alloc;

    EXPECT_THROW(
        (void)alloc.allocate(std::size_t(-1)),
        std::bad_array_new_length
    );
}

#endif

namespace test_memory
{
static std::byte hooked_mem_buffer[128];
static std::size_t hooked_mem_used = 0;

static void* hooked_alloc(std::size_t n) noexcept
{
    if(hooked_mem_used >= 128)
        return nullptr;

    void* mem = hooked_mem_buffer + hooked_mem_used;
    hooked_mem_used += n;
    return mem;
}

static void hooked_free(void* mem) noexcept
{
    EXPECT_GE(mem, hooked_mem_buffer);
    EXPECT_LT(mem, hooked_mem_buffer + 128);
}

void setup_mem_hooks()
{
    int r = AS_NAMESPACE_QUALIFIER asSetGlobalMemoryFunctions(
        &hooked_alloc, &hooked_free
    );
    EXPECT_GE(r, 0);
}
} // namespace test_memory

TEST(ScriptAllocator, BadAlloc)
{
    test_memory::setup_mem_hooks();

    using asbind20::as_allocator;

    as_allocator<std::byte> alloc;
    auto* mem = alloc.allocate(128);
    EXPECT_NE(mem, nullptr);

#ifndef ASBIND20_NO_EXCEPTIONS

    EXPECT_THROW(
        (void)alloc.allocate(32),
        std::bad_alloc
    );

#endif

    alloc.deallocate(mem, 128);

    AS_NAMESPACE_QUALIFIER asResetGlobalMemoryFunctions();
}

#include <map>
#include <vector>
#include <gmock/gmock-matchers.h>

TEST(ScriptAllocator, ContainerAllocVector)
{
    using asbind20::as_allocator;

    using vector_type = std::vector<
        std::string,
        as_allocator<std::string>>;

    vector_type v;
    v.push_back("10");
    v.push_back("13");

    EXPECT_THAT(v, ::testing::ElementsAre("10", "13"));

    vector_type copied = v;
    EXPECT_EQ(copied, v);
}

TEST(ScriptAllocator, ContainerAllocMap)
{
    using asbind20::as_allocator;
    using map_type = std::map<
        std::string,
        int,
        std::less<std::string>,
        as_allocator<std::pair<const std::string, int>>>;
    using pair_type = map_type::value_type;

    map_type m;
    m.insert({"mon", 10});
    m.insert({"day", 13});

    EXPECT_THAT(
        m,
        ::testing::ElementsAre(
            pair_type("day", 13),
            pair_type("mon", 10)
        )
    );

    map_type copied = m;
    EXPECT_EQ(copied, m);
}
