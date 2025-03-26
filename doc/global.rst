Registering Global Entities
===========================

Global Functions and Properties
-------------------------------

C++ code:

.. code-block:: c++

    void func(int arg);

    void func_gen(asIScriptGeneric* gen);

    class my_class
    {
        int f();
    };

    my_class instance{...};

    int global_var = ...;

Registering:

.. code-block:: c++

    asIScriptEngine* engine = ...;
    asbind20::global(engine)
        // Ordinary function (native)
        .function("void func(int arg)", &func)
        // Ordinary function (generic)
        .function("void func_gen(int arg)", &func_gen)
        // asCALL_THISCALL_ASGLOBAL
        // Equivalent to `instance.f()`
        // The instance will be stored in the auxiliary pointer.
        .function("int f()", &my_class::f, asbind20::auxiliary(instance))
        .property("int global_var", global_var);

Type Aliases
------------

.. doxygenclass:: asbind20::global
    :members: funcdef, typedef_, using_

Example code:

.. code-block:: c++

    asIScriptEngine* engine = ...;
    asbind20::global(engine)
        .funcdef("bool callback(int, int)")
        .typedef_("float", "real32")
        // For those who feel more comfortable with the C++11 style
        // "using alias = type;"
        .using_("float32", "float");

Enumerations
------------

.. code-block:: c++

    enum class my_enum : int
    {
        A,
        B
    };

    enum_<my_enum>(engine, "my_enum")
        .value(my_enum::A, "A")
        .value(my_enum::B, "B");

Besides, the library provides tool for generating string representation of enum value at compile-time.

.. code-block:: c++

    // The following code is equivalent to the above one
    asbind20::enum_<my_enum>(engine, "my_enum")
        .value<my_enum::A>()
        .value<my_enum::B>();

However, as static reflection is still waiting for the C++26, this feature relies on compiler extension and is platform dependent. **It has some limitations**. For example, it cannot generate string representation for enums with same value.

.. code-block:: c++

    enum overlapped
    {
        A = 1,
        B = 1 // Not supported for this kind of enum value
    };

If you are interested in how this is achieved, you can read `this article written by YKIKO (Chinese) <https://zhuanlan.zhihu.com/p/680412313>`_
(or author's `English translation <https://ykiko.me/en/articles/680412313/>`_).

Special Functions
-----------------

Please check the official documentation of AngelScript for the requirements of following functions.

Message Callback
~~~~~~~~~~~~~~~~

Registered by ``message_callback``.

.. doxygenclass:: asbind20::global
    :members: message_callback

See `AngelScript documentation <https://www.angelcode.com/angelscript/sdk/docs/manual/doc_compile_script.html#doc_compile_script_msg>`_ for details.

Exception Translator
~~~~~~~~~~~~~~~~~~~~

Registered by ``exception_translator``.

.. doxygenclass:: asbind20::global
    :members: exception_translator

NOTE: If your AngelScript is built without exception support (``asGetLibraryOptions()`` reports ``AS_NO_EXCEPTIONS``), this function will fail to register the translator.
See `AngelScript documentation about C++ exceptions <https://www.angelcode.com/angelscript/sdk/docs/manual/doc_cpp_exceptions.html>`_ for details.
