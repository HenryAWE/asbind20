Invoking Script Interfaces
==========================

Invoking a Script Function
--------------------------

This library can automatically convert arguments in C++ for invoking an AngelScript function.

.. doxygenfunction:: asbind20::script_invoke(asIScriptContext*, asIScriptFunction*, Args&&...)

This example assumes the ``std::string`` is registered as script string type.
You can change the ``std::string`` to your underlying string type.

AngelScript function:

.. code-block:: AngelScript

    string test(int a, int&out b)
    {
        b = a + 1;
        return "test";
    }

C++ code:

.. code-block:: c++

    asIScriptEngine* engine = /* Get a script engine */;
    asIScriptModule* m = /* Build the above script */;
    asIScriptFunction* func = m->GetFunctionByName("test");
    if(!func)
        /* Error handling */

    // Manage script context using the RAII helper
    asbind20::request_context ctx(engine);

    int val = 0;
    auto result = asbind20::script_invoke<std::string>(
        ctx, func, 1, std::ref(val) // The reference needs a wrapper
    );

    assert(result.value() == "test");
    assert(val == 2);

Using a Script Class
--------------------

The library provides tools for instantiating a script class.

.. doxygenfunction:: asbind20::instantiate_class

The ``script_invoke`` also supports invoking a method, a.k.a., member function.
You need to put the script object in front of the script function pointer in arguments.
This is designed to simulate a method call ``obj.method()``.

The type of script object can be either ``(const) void*`` or a type that can be cast into ``(const) asIScriptObject*``.

.. doxygenfunction:: asbind20::script_invoke(asIScriptContext*, Object&&, asIScriptFunction*, Args&&...)

The script class defined in AngelScript:

.. code-block:: AngelScript

    class my_class
    {
        int m_val;

        void set_val(int new_val) { m_val = new_val; }
        int get_val() const { return m_val; }
        int& get_val_ref() { return m_val; }
    };

C++ code:

.. code-block:: c++

    asIScriptEngine* engine = /* Get a script engine */;
    asIScriptModule* m = /* Build the above script */;
    asITypeInfo* my_class_t = m->GetTypeInfoByName("my_class");

    asbind20::request_context ctx(engine);

    auto my_class = asbind20::instantiate_class(ctx, my_class_t);

    asIScriptFunction* set_val = my_class_t->GetMethodByDecl("void set_val(int)");
    asbind20::script_invoke<void>(ctx, my_class, set_val, 182375);

    asIScriptFunction* get_val = my_class_t->GetMethodByDecl("int get_val() const");
    auto val = asbind20::script_invoke<int>(ctx, my_class, get_val);

    assert(val.value() == 182375);

    asIScriptFunction* get_val_ref = my_class_t->GetMethodByDecl("int& get_val_ref()");
    auto val_ref = asbind20::script_invoke<int&>(ctx, my_class, get_val_ref);

    assert(val_ref.value() == 182375);

    *val_ref = 182376;

    val = asbind20::script_invoke<int>(ctx, my_class, get_val);
    assert(val.value() == 182376);

Script Function Objects
-----------------------

``script_function`` and ``script_method`` wrap an ``asIScriptFunction*`` with ownership semantics,
tracking the module reference count to keep the function valid even after the module is discarded.
``script_function_ref`` and ``script_method_ref`` are non-owning counterparts.

.. code-block:: c++

    #include <asbind20/asbind.hpp>

    auto engine = asbind20::make_script_engine();
    auto* m = engine->GetModule("test", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE);
    m->AddScriptSection("test", "int test() { return 42; }");
    m->Build();

    // Owning wrapper — keeps the module alive
    asbind20::script_function<int()> f(m->GetFunctionByName("test"));

    m->Discard(); // module is discarded, but f still holds a reference

    asbind20::request_context ctx(engine);
    auto result = f(ctx);
    assert(result.value() == 42);

    // Non-owning lightweight reference
    asbind20::script_function_ref<int()> rf = f;
    assert(rf.target() == f.target());

    // Convert ref back to owning
    asbind20::script_function<int()> another = rf;
    assert(another.target() == rf.target());

    // Reset releases ownership
    f.reset();
    assert(!f);
    assert(f.target() == nullptr);

``script_method`` works the same way for member functions. Define a script class
and wrap its methods:

.. code-block:: c++

    auto* m2 = engine->GetModule("test2", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE);
    m2->AddScriptSection("test2",
        "class foo { int get_val() const { return 42; } };");
    m2->Build();

    auto* ti = m2->GetTypeInfoByName("foo");
    auto foo_t = asbind20::script_typeinfo(ti);

    asbind20::request_context ctx(engine);
    auto foo = asbind20::instantiate_class(ctx, foo_t);

    asbind20::script_method<int()> get_val(ti->GetMethodByName("get_val"));
    auto val = get_val(ctx, foo);
    assert(val.value() == 42);

    // Non-owning ref
    asbind20::script_method_ref<int()> rf = get_val;
    auto val2 = rf(ctx, foo);
    assert(val2.value() == 42);

Reference of Invocation Tools
-----------------------------

.. doxygenfunction:: asbind20::get_context_result

The result types of script invocation consist of the primary template,
specialization for references, and specialization for ``void``.

.. doxygenclass:: asbind20::script_invoke_result
  :members:
  :undoc-members:

The library also provides specializations for reference types (``script_invoke_result<T&>``) and ``void`` (``script_invoke_result<void>``).
