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

Example: a container element accessor that handles different type categories:

.. code-block:: c++

    void* get_element_ref(int type_id, void* data, int index)
    {
        if(is_void_type(type_id))
            return nullptr;

        if(is_objhandle(type_id))
        {
            // Handle references — the element is a pointer to the actual object
            auto* handle_ptr = static_cast<void**>(data) + index;
            if(!*handle_ptr)
                return nullptr;
            return *handle_ptr;
        }

        // Primitive and value types — the element is stored inline
        std::size_t elem_size = sizeof_script_type(type_id);
        return static_cast<char*>(data) + (index * elem_size);
    }

    void copy_element(void* dst, const void* src, int type_id)
    {
        if(is_void_type(type_id))
            return;

        if(is_objhandle(type_id))
        {
            auto* handle = static_cast<void* const*>(src);
            if(*handle) (*handle)->AddRef();
            *static_cast<void**>(dst) = *handle;
            return;
        }

        // Primitive type — fast path via memcpy
        if(is_primitive_type(type_id))
        {
            copy_primitive_value(dst, src, type_id);
            return;
        }

        // Enum or script object — use AngelScript's copy assignment
        auto* engine = asGetActiveContext()->GetEngine();
        auto* ti = engine->GetTypeInfoById(type_id);
        auto* script_ctx = engine->RequestContext();
        script_ctx->Prepare(ti->GetFactoryByIndex(0)); // copy op
        script_ctx->SetArgAddress(0, dst);
        script_ctx->SetArgAddress(1, src);
        script_ctx->Execute();
        engine->ReturnContext(script_ctx);
    }

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
                // Object handle — elements are void* pointers
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
