Handling AngelScript Types
==========================

If you are registering functions with variable type argument or template classes,
you will definitely need tools to deal with AngelScript types.

Type Traits for Script Type IDs
-------------------------------

.. doxygenfunction:: asbind20::is_void_type
.. doxygenfunction:: asbind20::is_primitive_type
.. doxygenfunction:: asbind20::is_enum_type
.. doxygenfunction:: asbind20::is_objhandle
.. doxygenfunction:: asbind20::type_requires_gc
.. doxygenfunction:: asbind20::sizeof_script_type

Dispatching Function Calls Based on Type Ids
--------------------------------------------

This feature is similar to how `std::visit` and `std::variant` works.
It can be used for developing templated container for AngelScript.

.. doxygenfunction:: asbind20::visit_primitive_type
.. doxygenfunction:: asbind20::visit_script_type
.. doxygenfunction:: asbind20::visit_primitive_type_id

Example code:

.. code-block:: c++

    asbind20::visit_primitive_type(
        []<typename T>(T* val)
        {
            using type = std::remove_const_t<T>;

            if constexpr(std::same_as<type, int>)
            {
                // play with int
            }
            else if constexpr(std::same_as<type, float>)
            {
                // play with float
            }
            else
            {
                // ...
            }
        },
        as_type_id, // asTYPEID_BOOL, asTYPEID_INT32, etc.
        ptr_to_val // (const) void* to value
    );

If you are certain that types you are dealing with are all primitive types,
you can use the primitive-specific function.

.. doxygenfunction:: copy_primitive_value
