Generic Wrappers
================

.. doxygenfunction:: asbind20::has_max_portability

Wrap a Specific Method, Function, Behavior
------------------------------------------

With the power of non-type template parameter (NTTP),
asbind20 can generate generic wrapper of any function by macro-free utilities.
Additionally, it supports for wrapping a function with variable type (declared by ``?`` in the AngelScript).
This can be useful for binding interface on platform without native calling convention support, e.g., Emscripten.

Ordinary Methods and Behaviors
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c++

    using asbind20::use_generic;
    using asbind20::fp;
    using asbind20::var_type;

    // Use `use_generic` tag to force generated proxies to use asCALL_GENERIC
    asbind20::value_class<my_value_class>(...)
        .constructor<int>(use_generic, "int")
        .opEquals(use_generic)
        // Due to the limitation of C++, you need to pass the function pointer by a helper (fp<>)
        .method(use_generic, "int mem_func(int arg)", fp<&my_value_class::mem_func>)
        // Lambda can be directly registered
        .method(
            use_generic,
            "int f(int arg)",
            [](my_value_class& this_, int arg) -> int
            { /* ... */ }
        )
        // If the function has already used generic calling convention,
        // just register it as usual
        .method("int gfn(float arg)", &gfn);

Functions with Variable Type Argument
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Functions with variable type argument receive the variable parameter (``?``) from AngelScript by a pair of ``void*`` and ``int``.
The first one contains address of argument, while the second one contains the type ID.

The helper ``var_type<Is...>`` needs the indices of ``?`` in **script** parameter list.

.. list-table:: Examples
   :widths: 25 25 50
   :header-rows: 1

   * - Script Parameter List
     - Helper Usage
     - Parameter List in C++

   * - ``const ?&in``
     - ``var_type<0>``
     - ``void*, int``

   * - ``float, const ?&in``
     - ``var_type<1>``
     - ``float, void*, int``

   * - ``const ?&in, float, const ?&in``
     - ``var_type<0, 2>``
     - ``void*, int, float, void*, int``

Example code:

.. code-block:: c++

    // ...
        .method(
            use_generic, "void log(int level, const ?&in)", fp<&my_value_class::log>, var_type<1>
        )
        .method(
            use_generic,
            "void from_var(const ?&in)",
            [](my_value_class& this_, void* ref, int type_id) -> void
            { /* ... */ },
            var_type<0>
        );

Wrap a Group of Methods, Functions, or Behaviors
------------------------------------------------

If you want to force a group of registered functions to be generic, you can set the ``ForceGeneric`` flag of binding generator to ``true``.
You can use this flag to avoid the code for registering the application interface being flooded by ``use_generic``.

Trying to register functions by native calling convention with ``ForceGeneric`` enabled will trigger a compile-time error.

.. code-block:: c++

    asbind20::value_class<my_value_class, true>(...);
    asbind20::ref_class<my_ref_class, true>(...);
    asbind20::global<true>(...);

.. note::

    If you use an outer template argument to control the mode of binding generator,
    the generator can be a dependent name, thus you may need an additional ``template`` disambiguator to use the binding generator.

    .. code-block:: c++

        template <bool UseGeneric>
        void register_my_class(asIScriptEngine* engine)
        {
            asbind20::value_class<my_value_class, UseGeneric>(engine, "my_value_class")
                .template constructor<int>("int")
                .template opConv<int>()
                // The interfaces that don't need template parameters can be used as usual
                .method("int f()", asbind20::fp<&my_value_class::f>);
        }
