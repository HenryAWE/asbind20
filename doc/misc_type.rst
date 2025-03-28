Miscellaneous Types
===================

Type Aliases
------------

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
   AngelScript uses 32-bit integer to store underlying value of enums.
   Please make sure those values don't overflow or underflow.

Besides, the library provides a convenient interface for generating string representation of enum value at compile-time.

The following code is equivalent to the above one:

.. code-block:: c++

    asbind20::enum_<my_enum>(engine, "my_enum")
        .value<my_enum::A>()
        .value<my_enum::B>();

.. note::
   However, as static reflection is still waiting for the C++26, this feature relies on compiler extension and is platform dependent.
   **It has some limitations**. For example, it cannot generate string representation for enums with same value.

   .. code-block:: c++

    enum overlapped
    {
        A = 1,
        B = 1 // Not supported for this kind of enum value
    };

   If you are interested in how this is achieved, you can read `this article written by YKIKO (Chinese) <https://zhuanlan.zhihu.com/p/680412313>`_,
   or author's `English translation <https://ykiko.me/en/articles/680412313/>`_.

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
