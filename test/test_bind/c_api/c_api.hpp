#pragma once

namespace test_bind
{
// Opaque structure to simulate common C-API pattern
struct opaque_structure;

opaque_structure* create_opaque();
opaque_structure* create_opaque_with(int data);

void opaque_addref(opaque_structure* ptr);
void release_opaque(opaque_structure* ptr);

void opaque_set_data(opaque_structure* ptr, int data);
int opaque_get_data(opaque_structure* ptr);
} // namespace test_bind
