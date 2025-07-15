#include "c_api.hpp"
#include <cassert>
#include <new>

namespace test_bind
{
struct opaque_structure
{
    int data;
    int ref_count;
};

opaque_structure* create_opaque()
{
    opaque_structure* ptr = new(std::nothrow) opaque_structure{};
    if(!ptr)
        return nullptr;

    ptr->data = 0;
    ptr->ref_count = 1;
    return ptr;
}

opaque_structure* create_opaque_with(int data)
{
    opaque_structure* ptr = new(std::nothrow) opaque_structure{};
    if(!ptr)
        return nullptr;

    ptr->data = data;
    ptr->ref_count = 1;
    return ptr;
}

void opaque_addref(opaque_structure* ptr)
{
    assert(ptr != nullptr);
    ++ptr->ref_count;
}

void release_opaque(opaque_structure* ptr)
{
    assert(ptr != nullptr);
    assert(ptr->data > 0);
    --ptr->ref_count;
    if(ptr->ref_count == 0)
        delete ptr;
}

void opaque_set_data(opaque_structure* ptr, int data)
{
    assert(ptr != nullptr);
    ptr->data = data;
}

int opaque_get_data(opaque_structure* ptr)
{
    assert(ptr != nullptr);
    return ptr->data;
}
} // namespace test_bind
