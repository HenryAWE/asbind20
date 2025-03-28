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

Member Function as Global Function
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can synthesize global function by member function and an instance:

.. code-block:: c++

    class my_class
    {
        int f();
    };

    my_class instance{};

.. code-block:: c++

    asbind20::global(engine)
        .function("int f()", &my_class::f, asbind20::auxiliary(instance));

When the ``f()`` is called by script, it's equivalent to ``instance.f()`` in C++.

Registering Global Properties
-----------------------------

.. code-block:: c++

    int global_var = 42;
    const int const_global_var = 42;

.. code-block:: c++

    asbind20::global(engine)
        .property("int global_var", global_var)
        .property("const int const_global_var", const_global_var);


Special Functions
-----------------

Please check the official documentation of AngelScript for the requirements of following functions.

Message Callback
~~~~~~~~~~~~~~~~

Registered by ``message_callback``.

.. doxygenclass:: asbind20::global
    :members: message_callback
    :members-only:
    :no-link:

See `AngelScript documentation <https://www.angelcode.com/angelscript/sdk/docs/manual/doc_compile_script.html#doc_compile_script_msg>`_ for details.

Exception Translator
~~~~~~~~~~~~~~~~~~~~

Registered by ``exception_translator``.

.. doxygenclass:: asbind20::global
    :members: exception_translator
    :members-only:
    :no-link:

.. note::
   If your AngelScript is built without exception support (``asGetLibraryOptions()`` reports ``AS_NO_EXCEPTIONS``),
   this helper will fail to register the translator.

See `AngelScript documentation about C++ exceptions <https://www.angelcode.com/angelscript/sdk/docs/manual/doc_cpp_exceptions.html>`_ for details.
