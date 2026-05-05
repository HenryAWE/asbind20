Auxiliary Interfaces
====================

Registering an Interface
------------------------

Class interfaces can be registered if you want to guarantee that script classes implement a specific set of class methods.
Interfaces can be easier to use when working with script classes from the application,
but they are not necessary as the application can easily enumerate available methods and properties even without the interfaces.

.. code-block:: c++

    asbind20::interface(engine, "my_interface")
        // Declarations only
        .method("int get() const")
        .funcdef("int callback(int)")
        .method("int invoke(callback@ cb) const");

.. note::
  Unlike the raw AngelScript interface,
  you don't need to add the class name into the declaration of member ``funcdef`` for asbind20.

Type Aliases
------------

Function definitions can be registered when you wish to allow the script to pass function pointers to the application,
e.g. to implement callback routines.

Enumeration types and ``typedef``\ s can also be registered to improve readability of the scripts.

.. doxygenclass:: asbind20::global
    :members: funcdef, typedef_, using_
    :members-only:
    :no-link:

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

.. code-block:: c++

    enum_<my_enum>(engine, "my_enum")
        .value(my_enum::A, "A")
        .value(my_enum::B, "B");

.. note::
  Up until version 2.39, AngelScript used a 32-bit integer to store the underlying value of enums,
  before it supported customizable underlying types.
  Please make sure those values don't overflow or underflow.

Besides, the library provides a convenient interface for generating string representation of enum value at compile-time.

The following code is equivalent to the above one:

.. code-block:: c++

    asbind20::enum_<my_enum>(engine, "my_enum")
        .value<my_enum::A>()
        .value<my_enum::B>();

.. note::
  However, as static reflection is still waiting for C++26, this feature relies on compiler extension.
  It's guaranteed to work on mainstream compilers (MSVC, GCC, and Clang).
  **It has some limitations**. For example, it cannot generate string representation for enums with same value.

  .. code-block:: c++

    enum overlapped
    {
        A = 1,
        B = 1 // Not supported for this kind of enum value
    };

Since the version 2.39, AngelScript supports enumerations with custom underlying types.

You can register them by ``enum_underlying``,
which is an alias of ``enum_<Enum, std::underlying_type_t<Enum>>``.

.. code-block:: c++

    enum class enum_uint64 : std::uint64_t
    {
        A,
        B = std::uint64_t(-1) // Larger than UINT32_MAX
    };

.. code-block:: c++

    asbind20::enum_underlying<enum_uint64>(engine, "enum_uint64")
        .value(enum_uint64::A, "A")
        .value(enum_uint64::B, "B");

.. note::

    If you need to interact these enums from C++ side,
    the enums with custom underlying type :ref:`need to specify the conversion rules <custom-rule-for-enum-underlying>`.

Listener API
------------

.. warning::
    The listener API described in this section is **experimental**. Its interface may change in future releases.

Every binding generator (``value_class``, ``ref_class``, ``global``, ``enum_``, ``interface``) accepts an optional
``Listener`` template parameter. When an entity is registered, the generator invokes the corresponding callback
on the listener, passing itself and the returned value from AngelScript API.

This allows you to:

- Record what was registered for logging or debugging
- Validate registered entities
- Collect metadata about the bound interface

Defining a listener
~~~~~~~~~~~~~~~~~~~

A listener is a class that implements any subset of the following callback methods.
Each is optional — if omitted, the call is silently skipped (no-op).

.. code-block:: c++

    struct my_listener
    {
        // Global scope callbacks
        void on_function(auto& gen, int func_id);
        void on_global_property(auto& gen, int prop_id);
        void on_funcdef(auto& gen, int funcdef_id);
        void on_typedef(auto& gen, int type_id);

        // Class/interface scope callbacks
        void on_class(auto& gen, int type_id);
        void on_interface(auto& gen, int type_id);
        void on_method(auto& gen, int method_id);

        // Enum callbacks
        void on_enum(auto& gen, int type_id);
        void on_enum_value(auto& gen, int value_id);
    };

The ``gen`` parameter is a reference to the binding generator instance (e.g. ``global``, ``value_class<T>``).
Use it to query the engine, type info, or the generator's type ID.

The ``int`` parameter is the AngelScript ID returned by the registration call:

- Non-negative values indicate success.
- By default, negative values will throw a ``std::system_error``.
  Define ``ASBIND20_NO_THROW_ON_BAD_BINDING`` to suppress this.

Using a listener
~~~~~~~~~~~~~~~~

Pass the listener as a template parameter and retrieve it with ``get_listener()``:

.. code-block:: c++

    value_class<my_class, false, class_listener> c(engine, "my_class");

    c.method("void foo()", &my_class::foo);
    c.method("void bar() const", &my_class::bar);

    auto& listener = c.get_listener();
    // listener.recorded_class == ["my_class"]
    // listener.recorded_method == ["foo", "bar"]

Listeners are default-constructed unless you pass an instance to the generator's constructor:

.. code-block:: c++

    my_listener pre_conf;
    // ... configure pre_conf ...
    global<false, my_listener> g(engine, std::move(pre_conf));
