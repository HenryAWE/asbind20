#include "c_api.hpp"
#include <new>
#include <gtest/gtest.h>

namespace test_bind
{
struct opaque_structure
{
    int data;
    int ref_count;
};

opaque_structure* create_opaque()
{
    auto* ptr = new(std::nothrow) opaque_structure{};
    if(!ptr)
    {
        ADD_FAILURE() << "out of memory";
        return nullptr;
    }

    ptr->data = 0;
    ptr->ref_count = 1;
    return ptr;
}

opaque_structure* create_opaque_with(int data)
{
    opaque_structure* ptr = create_opaque();

    if(ptr)
        ptr->data = data;
    return ptr;
}

void opaque_addref(opaque_structure* ptr)
{
    ASSERT_NE(ptr, nullptr);
    ++ptr->ref_count;
}

void release_opaque(opaque_structure* ptr)
{
    ASSERT_NE(ptr, nullptr);
    EXPECT_GT(ptr->ref_count, 0)
        << "Bad reference count";
    --ptr->ref_count;
    if(ptr->ref_count <= 0)
        delete ptr;
}

void opaque_set_data(opaque_structure* ptr, int data)
{
    ASSERT_NE(ptr, nullptr);
    ptr->data = data;
}

int opaque_get_data(const opaque_structure* ptr)
{
    if(ptr == nullptr)
    {
        ADD_FAILURE() << "ptr is nullptr";
        return 0;
    }
    return ptr->data;
}
} // namespace test_bind
