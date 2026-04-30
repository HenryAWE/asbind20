Global Functions and Properties
===============================

Registering Global Functions
----------------------------

Ordinary Functions
~~~~~~~~~~~~~~~~~~

C++ Declarations:

.. code-block:: c++

    void func(int arg);

    void gfn(asIScriptGeneric* gen);
    void gfn_using_aux(asIScriptGeneric* gen);

Registering:

.. code-block:: c++

    asbind20::global(engine)
        // Ordinary function (native)
        .function("void func(int arg)", &func)
        // Ordinary function (generic)
        .function("void gfn(int arg)", &gfn)
        .function("void gfn(int arg)", &gfn_using_aux, asbind20::auxiliary(/* some auxiliary data */));

.. note::
  Make sure the script function declaration matches what the registered function does with the ``asIScriptGeneric``!

For overloaded functions,
you need to use ``overload_cast`` with arguments to choose the function you want.

.. code-block:: c++

    void func(int, int);
    void func(float);

.. code-block:: c++

    using namespace asbind20;

    global(engine)
        .function("void func(int, int)", overload_cast<int, int>(&func))
        .function("void func(float)", overload_cast<float>(&func));

Member Function as Global Function
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can synthesize a global function from a member function and an instance:

.. code-block:: c++

    class my_class
    {
        int f();
    };

.. code-block:: c++

    my_class instance{};

    asbind20::global(engine)
        .function("int f()", &my_class::f, asbind20::auxiliary(instance));

When the ``f()`` is called by script, it's equivalent to ``instance.f()`` in C++.

Lambdas
~~~~~~~

A lambda can be registered as a global function:

.. code-block:: c++

    asbind20::global(engine)
        .function(
            "int gen_int()",
            []() -> int { return 42; }
        );

Auxiliary Data
~~~~~~~~~~~~~~

Use ``aux_value`` to pass a small pointer-sized integer as auxiliary data.
It can be retrieved via ``asIScriptGeneric::GetAuxiliary()``.

.. code-block:: c++

    void from_aux(asIScriptGeneric* gen)
    {
        auto val = std::bit_cast<std::intptr_t>(gen->GetAuxiliary());
        asbind20::set_generic_return<int>(gen, static_cast<int>(val));
    }

    asbind20::global(engine)
        .function("int from_aux()", &from_aux, asbind20::aux_value(1013));

.. note::
    DO NOT use this helper unless you know what you are exactly doing!

Force-Generic Mode
~~~~~~~~~~~~~~~~~~

When targeting platforms without native calling convention support (e.g. Emscripten),
force all registered functions to use the generic convention:

.. code-block:: c++

    asbind20::global<true> g(engine);
    g.function("int stdcall_func(int a, float b)", fp<&stdcall_func>);

Registering Global Properties
-----------------------------

.. code-block:: c++

    int global_var = 42;
    const int const_global_var = 42;

.. code-block:: c++

    asbind20::global(engine)
        .property("int global_var", global_var)
        .property("const int const_global_var", const_global_var);


Special Callbacks
-----------------

Please check the official documentation of AngelScript for the requirements of the following functions.

Message Callback
~~~~~~~~~~~~~~~~

Registered by ``set_message_callback``.

.. doxygenfunction:: asbind20::set_message_callback(asIScriptEngine*,Callback,void*)
.. doxygenfunction:: asbind20::set_message_callback(asIScriptEngine*,Callback,auxiliary_wrapper<T>)

.. code-block:: c++

    #include <asbind20/asbind.hpp>

    static void message_cb(asSMessageInfo* info, void* data)
    {
        auto* counter = static_cast<int*>(data);
        (*counter)++;
    }

    int main()
    {
        auto engine = asbind20::make_script_engine();
        int msg_count = 0;

        asbind20::set_message_callback(engine, &message_cb, &msg_count);

        // ... register and build script ...

        assert(msg_count >= 0); // callback fires for each compiler message
    }

A member function can also be used as message callback.

.. code-block:: c++

    struct logger
    {
        void on_message(const asSMessageInfo* info)
        {
            if(info->type == asMSGTYPE_ERROR)
                ++errors;
        }

        int errors = 0;
    };

    logger log;
    asbind20::set_message_callback(
        engine,
        &logger::on_message,
        asbind20::auxiliary(log)
    );

See `AngelScript documentation <https://www.angelcode.com/angelscript/sdk/docs/manual/doc_compile_script.html#doc_compile_script_msg>`_ for details.

Exception Translator
~~~~~~~~~~~~~~~~~~~~

Registered by ``set_exception_translator``.

.. doxygenfunction:: asbind20::set_exception_translator(asIScriptEngine*,Callback,void*)
.. doxygenfunction:: asbind20::set_exception_translator(asIScriptEngine*,Callback,auxiliary_wrapper<T>)

.. note::
  If your AngelScript is built without exception support (``asGetLibraryOptions()`` reports ``AS_NO_EXCEPTIONS``),
  this helper will fail to register the translator.

See `AngelScript documentation about C++ exceptions <https://www.angelcode.com/angelscript/sdk/docs/manual/doc_cpp_exceptions.html>`_ for details.
