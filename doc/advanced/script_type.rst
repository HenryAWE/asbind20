Handling Type IDs from AngelScript
==================================

If you are registering functions with variable type argument or template classes,
you will definitely need tools to deal with those type IDs.

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
