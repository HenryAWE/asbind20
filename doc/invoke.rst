Invoking Script Interfaces
==========================

Invoking a Script Function
--------------------------

This library can automatically convert arguments in C++ for invoking an AngelScript function.

This example uses the string extension from asbind20 as an example,
you can change the ``std::string`` to your underlying string type.

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
        ctx, func, 1, std::ref(val) // The reference of primitive type needs a wrapper
    );

    assert(result.value() == "test");
    assert(val == 2);

Using a Script Class
--------------------

The library provides tools for instantiating a script class.

.. doxygenfunction:: asbind20::instantiate_class

The ``script_invoke`` also supports invoking a method, a.k.a., member function.
You need to put the script object in front of the script function in arguments,
this is designed to simulate a method call ``obj.method()``.

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

Reference of Invocation Tools
-----------------------------

.. doxygenfunction:: asbind20::script_invoke(asIScriptContext*, asIScriptFunction*, Args&&...)
.. doxygenfunction:: asbind20::script_invoke(asIScriptContext*, Object&&, asIScriptFunction*, Args&&...)

.. doxygenclass:: asbind20::script_invoke_result
   :members:
   :undoc-members:
