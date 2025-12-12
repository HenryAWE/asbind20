#include <atomic>
#include <asbind_test/framework.hpp>
#include <asbind20/container/small_vector.hpp>

namespace test_container
{
class counted_alloc_base
{
public:
    inline static std::atomic_size_t counter;
};

template <typename T>
class counted_alloc :
    private counted_alloc_base,
    public std::allocator<T>
{
    using base_alloc = std::allocator<T>;

public:
    using size_type = std::size_t;
    using value_type = typename base_alloc::value_type;

    using base_alloc::base_alloc;

    value_type* allocate(std::size_t n)
    {
        counter += n * sizeof(T);
        return base_alloc::allocate(n);
    }

    void deallocate(value_type* p, size_type n)
    {
        base_alloc::deallocate(p, n);
        counter -= n * sizeof(T);
    }
};
} // namespace test_container

TEST(SmallVector, CountedAlloc)
{
    namespace container = asbind20::container;
    using asbind20::container::small_vector;

    using sv_type = container::small_vector<
        container::typeinfo_identity,
        4 * sizeof(void*),
        test_container::counted_alloc<void>>;

    auto& counter = test_container::counted_alloc_base::counter;

    EXPECT_EQ(counter, 0);

    sv_type sv(nullptr, AS_NAMESPACE_QUALIFIER asTYPEID_INT32);
    EXPECT_EQ(counter, 0);

    sv.emplace_back_n(32);
    EXPECT_GE(counter, 32 * sizeof(AS_NAMESPACE_QUALIFIER asINT32));

    sv.clear();
    sv.shrink_to_fit();
    EXPECT_EQ(counter, 0);
}
