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

Dispatching Function Calls Based on Type IDs
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

``visit_script_type`` extends this to non-primitive types — it dispatches primitive
types to ``visit_primitive_type`` and object handles as ``void*``:

.. code-block:: c++

    asbind20::visit_script_type(
        []<typename T>(T* begin, T* end)
        {
            using type = std::remove_const_t<T>;

            if constexpr(std::same_as<type, int>)
                std::sort((int*)begin, (int*)end);
            else if constexpr(std::same_as<type, float>)
                std::sort((float*)begin, (float*)end);
            else
            {
                // ...
            }
        },
        type_id,   // any AngelScript type ID
        data_begin,
        data_end
    );

If you are certain that types you are dealing with are all primitive types,
you can use the primitive-specific function.

.. doxygenfunction:: copy_primitive_value

Example code:

.. code-block:: c++

    // Copy primitive values between void* buffers
    int src_int = 42;
    int dst_int = 0;

    copy_primitive_value(&dst_int, &src_int, asTYPEID_INT32);
    assert(dst_int == 42);

    // Works for any primitive type
    float src_f = 3.14f;
    float dst_f = 0.0f;

    std::size_t bytes = copy_primitive_value(&dst_f, &src_f, asTYPEID_FLOAT);
    assert(bytes == sizeof(float));
    assert(dst_f == 3.14f);

    // void type copies nothing
    std::size_t n = copy_primitive_value(&dst_int, &src_int, asTYPEID_VOID);
    assert(n == 0);
