Registering Global Entities
===========================

Global Functions and Properties
-------------------------------

Global Functions
~~~~~~~~~~~~~~~~

C++ Declarations:

.. code-block:: c++

    void func(int arg);

    void func_gen(asIScriptGeneric* gen);

Registering:

.. code-block:: c++

    asbind20::global(engine)
        // Ordinary function (native)
        .function("void func(int arg)", &func)
        // Ordinary function (generic)
        .function("void func_gen(int arg)", &func_gen);

Synthesize global function by member function and an instance:

.. code-block:: c++

    class my_class
    {
        int f();
    };

    my_class instance{};

.. code-block:: c++

    asbind20::global(engine)
        .function("int f()", &my_class::f, asbind20::auxiliary(instance));

When the `f()` is called by script, it's equivalent to `instance.f()` in C++.

Global Properties
~~~~~~~~~~~~~~~~~

.. code-block:: c++

    int global_var = 42;
    const int const_global_var = 42;

.. code-block:: c++

    asbind20::global(engine)
        .property("int global_var", global_var)
        .property("const int const_global_var", const_global_var);

Type Aliases
------------

.. doxygenclass:: asbind20::global
    :members: funcdef, typedef_, using_

Example code:

.. code-block:: c++

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

The following code is equivalent to the above one:

.. code-block:: c++

    asbind20::enum_<my_enum>(engine, "my_enum")
        .value<my_enum::A>()
        .value<my_enum::B>();

However, as static reflection is still waiting for the C++26, this feature relies on compiler extension and is platform dependent.
**It has some limitations**. For example, it cannot generate string representation for enums with same value.

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
