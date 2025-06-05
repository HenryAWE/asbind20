Registering an Object Type
==========================

Registering a Reference Type
----------------------------

The basic reference type will be instantiated by factory function and managed by reference counting.

Factory
~~~~~~~

Factory Functions
^^^^^^^^^^^^^^^^^

Factory functions can be automatically generated from constructors of C++ type.
The memory for new object will be allocated using ``new``,
so if an allocation function provided by user is required, you can create a custom overload of ``operator new``.

If you already have a function for instantiating the class, you can register it by ``.factory_function``.

.. code-block:: c++

    class my_ref_class
    {
    public:
        my_ref_class();
        my_ref_class(float);
        my_ref_class(int, int);

        static my_ref_class* new_class_by_int(int);

        // ...
    };

You need to provide a parameter list for factories other than the default factory.

.. code-block:: c++

    asbind20::ref_class<my_ref_class>(engine, "my_ref_class")
        .default_factory()
        .factory<float>("float")
        .factory<int, int>("int, int")
        .factory_function("int", use_explicit, &new_class_by_int)

If you want certain factories to be marked with ``explicit``, you can use the tag ``use_explicit``.
Just put the tag right after the parameter list.

.. code-block:: c++

    asbind20::ref_class<my_ref_class>(engine, "my_ref_class")
        .factory<float>("float", asbind20::use_explicit)
        .factory_function("int", asbind20::use_explicit, &new_class_by_int);

Factory with Auxiliary Object
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

It might be useful to call the factory with a helper object.

.. code-block:: c++

    struct helper
    {
        my_ref_class* create_from_bool(bool arg);
    };

    my_ref_class* create_from_int(helper* aux, int arg);
    my_ref_class* create_from_float(float arg, helper* aux);

.. code-block:: c++

    using asbind20::auxiliary;
    // It needs an instance
    helper instance{};

    // ...
        .factory_function("bool", &helper::create_from_bool, auxiliary(instance))
        .factory_function("int", &create_from_int, auxiliary(instance))
        .factory_function("float", &create_from_float, auxiliary(instance));

.. note::
  If the factory function with an auxiliary object is not a member function,
  the parameter for receiving pointer to auxiliary object will be located by the following logic:

  1. Check if the first/last parameter is a reference/pointer to the helper object
  2. If both first and last parameters satisfy the condition, asbind20 will prefer the first one.

  If this is not the desired behavior, you can manually specify the position of that special parameter.

  .. code-block:: c++

    using namespace asbind20;
    // ...
        .factory_function("int", &create_from_int, auxiliary(instance), call_conv<asCALL_CDECL_OBJFIRST>)
        .factory_function("float", &create_from_float, auxiliary(instance), call_conv<asCALL_CDECL_OBJLAST>);

Specially, the auxiliary object can be the ``asITypeInfo*`` of type being registered.
This can be done by the tag ``this_type``.

This might be helpful when dealing with garbage collected types.

.. code-block:: c++

    my_ref_class* create_with_typeinfo(asITypeInfo* ti, int arg);


.. code-block:: c++

    using namespace asbind20;
    // ...
        .factory_function("int", &create_with_typeinfo, auxiliary(this_type));

List Factory
^^^^^^^^^^^^

List factory allows the reference type to be created from an initialization list.

:doc:`It will be discussed in a separated page. <advanced/init_list>`

Behaviors for Reference Counting
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The basic reference type uses reference counting to manage its lifetime.

.. code-block:: c++

    class my_ref_class
    {
    public:
        void addref()
        {
            ++m_refcount;
        }

        void release()
        {
            if(--m_refcount == 0)
                delete this;
        }

    private:
        int m_refcount = 1;
    };

.. code-block:: c++

    // ...
        .addref(&my_ref_class::addref)
        .release(&my_ref_class::release);

Tips for Reference Types
~~~~~~~~~~~~~~~~~~~~~~~~

- For reference counted type, the reference counter should be set to ``1`` during initialization.

- If your type involves GC, you need to notify the GC of a newly instantiated object by ``NotifyGarbageCollectorOfNewObject``,
  `as explained in AngelScript's official document <https://www.angelcode.com/angelscript/sdk/docs/manual/doc_gc_object.html#doc_reg_gcref_2>`_.

  The asbind20 also provides a policy called ``policies::notify_gc`` for (list) factory functions to automatically notify the GC after a new object created.

Registering a Value Type
------------------------

Flags of Value Type
~~~~~~~~~~~~~~~~~~~~

If the type doesn't require any special treatment,
i.e. doesn't contain any pointers or other resource references that must be maintained,
then the type can be registered with the flag ``asOBJ_POD``.
In this case AngelScript doesn't require the default constructor, assignment behavior, or destructor,
as it will be able to automatically handle these cases the same way it handles built-in primitives.

If you plan on passing or returning the type by value to registered functions that uses native calling convention,
you also need to inform how the type is implemented in the application.
But if you only plan on using generic calling conventions,
or don't pass these types by value then you don't need to worry about that.

The asbind20 will handle common flags for you.
However, due to limitation of C++, the following flags still need user to provide them manually.

.. list-table::
   :widths: 25 75
   :header-rows: 1

   * - Flag
     - Description

   * - ``asOBJ_APP_CLASS_MORE_CONSTRUCTORS``
     - The C++ class has additional constructors beyond the default/copy constructor

   * - ``asOBJ_APP_CLASS_ALLINTS``
     - The C++ class members can be treated as if all integers

   * - ``asOBJ_APP_CLASS_ALLFLOATS``
     - The C++ class members can be treated as if all ``float``\ s or ``double``\ s

   * - ``asOBJ_APP_CLASS_ALIGN8``
     - The C++ class contains members that may require 8-byte alignment.

       For example, a ``double``

   * - ``asOBJ_APP_CLASS_UNION``
     - The C++ class contains unions as members

.. note::
   C++ compiler may provide some functions automatically if one of the members is of a type that requires it.
   So even if the type you want to register doesn't have a declared default constructor,
   it may still be necessary to register the type with the flag ``asOBJ_APP_CLASS_MORE_CONSTRUCTORS``.

.. warning::
   Be careful to inform the correct flags,
   because if the wrong flags are used you may get unexpected behavior when calling registered functions that receives or returns these types by value.
   Common problems are stack corruptions or invalid memory accesses.
   In some cases you may face more silent errors that may be difficult to detect,
   e.g., the function is not returning the expected values.

You can also read the official documentation about
`value types and native calling convention <https://www.angelcode.com/angelscript/sdk/docs/manual/doc_register_val_type.html#doc_reg_val_2>`_ .

Constructors and Destructor
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Constructor Functions
^^^^^^^^^^^^^^^^^^^^^

The memory of value types are allocated by AngelScript,
then the memory needs to be initialized using the placement ``new``.

If you already have a function for initializing the class, you can register it by ``.constructor_function``.
You can also use a lambda to create a constructor function in-place.

.. code-block:: c++

    struct my_val_class
    {
        my_val_class() = default;
        my_val_class(const my_val_class&) = default;

        my_val_class(bool val);

        static void init_by_int(my_val_class* mem, int val);
        static void init_by_float(float val, my_val_class* mem);

        static my_val_class get_val(int arg0, int arg1);
    };

You need to provide a parameter list for constructors other than the default/copy constructor.

.. code-block:: c++

    asbind20::value_class<my_val_class>(
        engine, "my_val_class", asOBJ_APP_CLASS_MORE_CONSTRUCTORS
    )
        .default_constructor()
        .copy_constructor()
        .constructor<bool>("bool")
        .constructor_function("int", &init_by_int)
        .constructor_function("float", &init_by_float)
        .constructor_function(
            "int, int",
           [](void* mem, int arg0, int arg1)
           { new(mem) my_val_class(get_val(arg0, arg1)); }
        );

If you want certain factories to be marked with ``explicit``, you can use the tag ``use_explicit``.
Just put the tag right after the parameter list.

.. code-block:: c++

    // ...
        .constructor<float>("bool", asbind20::use_explicit)
        .constructor_function("int", asbind20::use_explicit, &init_by_int);

.. note::
  The parameter for receiving pointer to allocated memory will be located by the following logic:

  1. Check if the first/last parameter is a reference/pointer to the type being registered
  2. Check if the type of first/last parameter is ``void*``
  3. If both first and last parameters satisfy the condition, asbind20 will prefer the first one.

  If this is not the desired behavior, you can manually specify the position of that special parameter.

  .. code-block:: c++

    // ...
        .constructor_function("int", &init_by_int, asbind20::call_conv<asCALL_CDECL_OBJFIRST>)
        .constructor_function("float", &init_by_float, asbind20::call_conv<asCALL_CDECL_OBJLAST>);

List Constructor
^^^^^^^^^^^^^^^^

The list constructor allows the value type to be initialized from an initialization list.

:doc:`It will be discussed in a separated page. <advanced/init_list>`

Destructor
^^^^^^^^^^

.. code-block:: c++

    // ...
        .destructor();

Automatically Registering Required Behaviors
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can call the ``.behaviours_by_traits()`` to automatically register type behaviors required by the type flags.
It will register default constructor, copy constructor, destructor,
and assignment operator (``operator=``/``opAssign``) according to the type flags.

.. warning::
   Be careful not to register those behaviors again by standalone helpers,
   otherwise you will get an error message about duplicated things.

This helper function uses flags provided by ``asGetTypeTraits<T>()`` by default.

.. code-block:: c++

    // ...
        .behaviours_by_traits();

You can also provide the flags manually:

.. code-block:: c++

    // ...
        .behaviours_by_traits(asOBJ_APP_CLASS_CDAK);

Object Methods
--------------

Object methods are registered by ``.method()``.
Both non-virtual and virtual methods are registered the same way.

Static member functions of a class are actually global functions,
so those should be registered as :doc:`global functions and not as object methods <global>`.

Member Function
~~~~~~~~~~~~~~~

.. code-block:: c++

    class my_class
    {
    public:
        int foo(bool arg);

        void bar() const;
    };

.. code-block:: c++

    // ...
        .method("int foo(bool arg)", &my_class::foo)
        .method("void bar() const", &my_class::bar);


Extend Class Interface Without Changing Its Implementation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Global Functions Taking an Object Parameter
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

It is possible to register a global function that takes a pointer or a reference to the object as a class method.
This can be used to extend the functionality of a class when accessed via AngelScript,
without actually changing the C++ implementation of the class.

.. code-block:: c++

    void foobar_0(my_class& this_, int arg);
    float foobar_1(float arg, const my_class& this_);

.. code-block:: c++

    // ...
        .method("void foobar_0(int arg)", &foobar_0)
        .method("float foobar_1(float arg) const", &foobar_1);

Member Functions from a Helper Object
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Similar to global functions taking an object parameter,
member functions taking an object parameter from a helper object can also be registered as class methods.

.. code-block:: c++

    struct helper
    {
        void foobar_3(my_class& this_, int arg);
        float foobar_4(float arg, const my_class& this_);
    };

.. code-block:: c++

    // It needs an instance
    helper instance{};

    // ...
        .method("void foobar_3(int arg)", &helper::foobar_3, asbind20::auxiliary(instance))
        .method("float foobar_4(float arg) const", &helper::foobar_4, asbind20::auxiliary(instance));

.. note::
  The parameter for receiving object will be located by the following logic:

  1. Check if the first/last parameter is a reference/pointer to the type being registered
  2. If both first and last parameters satisfy the condition, asbind20 will prefer the first one.

     This is designed to keep consistency with existing C++ paradigm,
     such as how ``std::invoke`` deals with a member function pointer.

  If this is not the desired behavior, you can manually specify the position of that special parameter.

  .. code-block:: c++

    // Instance of the helper object
    helper instance{};
    // Using namespace to simplify code
    using namespace asbind20;

    // ...
        .method("void foobar_0(int arg)", &foobar_0, call_conv<asCALL_CDECL_OBJFIRST>)
        .method("float foobar_1(float arg) const", &foobar_1, call_conv<asCALL_CDECL_OBJLAST>)
        .method("void foobar_3(int arg)", &helper::foobar_3, call_conv<asCALL_THISCALL_OBJFIRST>, auxiliary(instance))
        .method("float foobar_4(float arg) const", &helper::foobar_4, call_conv<asCALL_THISCALL_OBJLAST>, auxiliary(instance));

Function Receiving ``asIScriptGeneric*``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c++

    void gfn(asIScriptGeneric* gen);
    void gfn_using_aux(asIScriptGeneric* gen);

.. code-block:: c++

    // ...
        .method("float gfn()", &gfn)
        .method("int gfn_using_aux()", &gfn_using_aux, asbind20::auxiliary(/* some auxiliary data */));

.. note::
   Make sure the method declaration matches what the registered function does with the ``asIScriptGeneric``!

Methods Using Composite Members
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If the application class that is being registered uses composition,
then it is possible to register the methods of the composite members without wrapper functions.

.. code-block:: c++

    struct comp_helper
    {
        int exec() const;
        void set(int arg);
    };

    struct my_class
    {
        comp_helper* indirect;
    };

.. code-block:: c++

    using asbind20::composite;

    // ...
        // Via a member pointer
        .method("int exec() const", &comp_helper::exec, composite(&my_class::indirect))
        // Via offset
        .method("void set(int arg)", &comp_helper::set, composite(offsetof(my_class, indirect)));

.. note::
   Currently, the composite methods only allow ``THISCALL`` calling convention. Besides, the generic wrapper doesn't support it.

Tips for Registering Methods
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. Registering Argument-Dependent Interfaces

   The member functions you want to register may have default arguments.
   Besides, some C++ libraries use the `argument-dependent lookup (ADL) <https://en.cppreference.com/w/cpp/language/adl>`_ to extend their interfaces.

   You need a wrapper function for this kind of interface.
   For convenience, you can also register them by lambda expressions.

   .. code-block:: c++

    value_class<math_lib_3rd::int128_t>(engine, "int128", /* ... */)
        .method(
            "int128 abs() const",
            [](const math_lib_3rd::int128& val)
                -> math_lib_3rd::int128_t
            {
                // No need to explicitly specify the namespace for abs(int128_t)
                return abs(val);
            }
        );

2. Overloaded Member Functions in C++

   It will be ambiguous to take address of overloaded functions,
   you need to use ``overload_cast`` with arguments to choose the function you want.

   .. code-block:: c++

    class my_class
    {
    public:
        void foo(int a, int b);
        void foo(float f) const;
    };

   .. code-block:: c++

    using namespace asbind20;
    // ...
        .method("void foo(int a, int b)", overload_cast<int, int>(&my_class::foo))
        // Use the tag "const_" for a const member function
        .method("void foo(float f) const", overload_cast<float>(&my_class::foo, const_));

Object Properties
-----------------

Class member variables can be registered,
so that they can be directly accessed by the script without the need for any method calls.

Ordinary Member Variables
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c++

    struct my_class
    {
        int a;
        int b;
    };

.. code-block:: c++

    // ...
        // Via a member pointer
        .property("int a", &my_class::a)
        // Via offset
        .property("int b", offsetof(my_class, b));

Composite Members
~~~~~~~~~~~~~~~~~

If the application class that is being registered uses composition,
then it is possible to register the properties of the composite members without wrapper functions.

.. code-block:: c++

    struct comp_data
    {
        int comp_a;
        int comp_b;
    };

    struct my_class
    {
        int a;
        comp_data* indirect;
        int b;
    };

.. code-block:: c++

    using asbind20::composite;

    // ...
        // Via a member pointer
        .property("int comp_a", &comp_data::comp_a, composite(&my_class::indirect))
        // Via offset
        .property("int comp_b", offsetof(comp_data, comp_b), composite(offsetof(my_class, indirect)));

Registering Types with Similar Interfaces
-----------------------------------------

Sometimes you may want to reuse code for binding similar type.
The binding generators of classes provide ``.use`` for reusing a certain set of registering code.

For example,
if your reference classes all use ``addref`` and ``release`` for reference counting,
you can create a helper to register those behaviors.

.. code-block:: c++

    struct register_refcount_helper
    {
        template <typename AutoRegister>
        void operator()(AutoRegister& ar) const
        {
            using class_type = typename AutoRegister::class_type;

            // Assuming those types have the same interfaces
            ar
                .addref(&class_type::addref)
                .release(&class_type::release);
        }
    };

.. code-block:: c++

    asbind20::ref_class<my_ref_class_0>(/* ... */)
        .use(register_refcount_helper{});

    asbind20::ref_class<my_ref_class_1>(/* ... */)
        .use(register_refcount_helper{});

Operator Overloads
------------------

Operator overloads are registered by `special method names <https://www.angelcode.com/angelscript/sdk/docs/manual/doc_script_class_ops.html>`_ in AngelScript.
You can just register them like ordinary methods.

However, the tools introduced in this section may help you register operators more easily.

Predefined Operator Helpers
~~~~~~~~~~~~~~~~~~~~~~~~~~~

There are predefined helpers that have the same name as AngelScript declaration.

Given constant C++ references ``a`` and ``b``, as well as a variable ``val`` of the type being registered ``T``,

+----------------------------------------+-----------------------------------+
| AngelScript Declaration                | Equivalent C++ Code               |
+========================================+===================================+
| ``T& opAssign(const T&in)``            | ``val = a``                       |
+----------------------------------------+-----------------------------------+
| ``bool opEquals(const T&in) const``    | ``a == b``                        |
+----------------------------------------+-----------------------------------+
| ``int opCmp(const T&in) const``        | ``translate_three_way(a <=> b)``  |
|                                        | *(see note)*                      |
+----------------------------------------+-----------------------------------+
| ``T& opAddAssign(const T&in)``         | ``val += a``                      |
+----------------------------------------+-----------------------------------+
| ``T& opSub/Div/MulAssign(const T&in)`` | Similar to the above one          |
+----------------------------------------+-----------------------------------+
| ``T& opPreInc/Dec()``                  | ``++val`` / ``--val``             |
+----------------------------------------+-----------------------------------+

The operators with ``T&`` as return type will return reference to the object being used,
so multiple assignment can be chained.

.. note::
    .. doxygenfunction:: asbind20::translate_three_way

    This wrapper requires ``operator<=>`` returns ``std::weak_ordering`` at least,
    i.e., **no** ``std::partial_ordering`` support.
    The result of three way comparison will be translated to integral value recognized by AngelScript.

If the type is registered as value type, there will be some additional predefined helpers.
These helpers will return result by value, so they cannot be used by a reference class.

+----------------------------------------+-----------------------------------+
| AngelScript Declaration                | Equivalent C++ Code               |
+========================================+===================================+
| ``T opAdd(const T&in) const``          | ``a + b``                         |
+----------------------------------------+-----------------------------------+
| ``T opSub/Div/Mul(const T&in) const``  | Similar to the above one          |
+----------------------------------------+-----------------------------------+
| ``T opPostInc/Dec()``                  | ``val++`` / ``val--``             |
+----------------------------------------+-----------------------------------+
| ``T opNeg() const``                    | ``-a``                            |
+----------------------------------------+-----------------------------------+

Example code:

.. code-block:: c++

    struct my_class
    {
        my_class& operator=(const my_class&);

        bool operator==(const my_class&) const;

        std::weak_ordering operator<=>(const my_class&) const;

        my_class& operator+=(const my_class&);
        friend my_class operator+(const my_class& lhs, const my_class& rhs);

        my_class& operator++();

        my_class operator--(int); // the postfix one

        my_class operator-() const;
    };

.. code-block:: c++

    // ...
        .opAssign()
        .opEquals()
        .opCmp()
        .opAddAssign()
        .opPreInc()
        // For value types:
        .opAdd()
        .opPostInc()
        .opNeg();

Type Conversion Operators
~~~~~~~~~~~~~~~~~~~~~~~~~

The type conversion operators can be used to convert types without a conversion constructor.
`This official document <https://www.angelcode.com/angelscript/sdk/docs/manual/doc_script_class_ops.html#doc_script_class_conv>`_ has explained the logic of type conversion in AngelScript

.. code-block:: c++

    struct my_class
    {
        operator bool() const;

        explicit operator std::string() const
    };

The generated conversion operators will use the expression ``static_cast<T>(val)`` internally.

.. code-block:: c++

    // ...
        // Type declaration can be omitted for primitive types
        .opImplConv<bool>()
        // Remember to register support of string at first
        .opConv<std::string>("string");

More Complex Operators
~~~~~~~~~~~~~~~~~~~~~~

If you want to register an operator overload whose signature is not listed above,
you can try the tools in the external header ``asbind20/operators.hpp``.

The operator tools use ``_this`` or ``const_this`` as a placeholder,
and use ``param<T>`` for parameters.

If you want to specify the return type, you can use ``->return_<T>()``,
which is designed to simulate the trailing return type ``auto f() -> Return`` since C++11.

.. code-block:: c++

    struct my_class
    {
        int operator-();
        float operator~() const;

        int operator--();
        int operator++(int);

        my_class& operator+=(int val);

        int operator[](int idx) const;
    };

.. code-block:: c++

    value_class<my_class>(engine, "my_class", /* ... */)
        // int my_class::operator-()
        .use(-_this)
        // float my_class::operator~() const
        .use(~const_this)
        // Unlike the predefined helpers,
        // this doesn't require the return type to be the same with the type being registered.
        .use(--_this)
        .use(_this++)
        .use(_this += param<int>)
        // Also supports the index operator
        .use(const_this[param<int>]);

.. note::
   In some libraries like the STL, the ``operator[]`` doesn't have a boundary check.
   You still need a wrapper function with checks for registering it.
   Otherwise bad script can directly crash your application, instead of giving an clear error message.

If the ``operator*`` doesn't return the type being registered,
e.g., dot product of a math vector which returns a scalar type,
the predefined helpers will fail to deal with this kind of operator overloads,
but this tool can be your best friend.

.. code-block:: c++

    value_class<vec2f>(engine, "vec2f", /* ... */)
        // It can also be more explicitly ".use((const_this * const_this)->return_<float>())"
        .use(const_this * const_this);


If the type involves in the operator is neither the class type being registered nor primitive types,
you may need to manually pass the script parameter declaration.

.. code-block:: c++

    // ...
        .use((const_this + param<const string&>("const string&in"))->return_<string>("string"))
        .use((param<const string&>("const string&in") + const_this)->return_<string>("string"));

With the power of this tool, you can even register the ``<iostream>`` to AngelScript.
Here is an example from the unit test of asbind20.

.. code-block:: c++

    ref_class<std::ostream>(
        engine,
        "ostream",
        asOBJ_NOCOUNT
    )
        .use(_this << param<bool>)
        .use(_this << param<int>)
        .use(_this << param<float>)
        .use(_this << param<const std::string&>("const string&in"));

    global(engine)
        .function(
            "void newline(ostream& os)",
            [](std::ostream& os)
            { os << std::endl; }
        )
        .property("ostream cout", std::cout);

Member Aliases
--------------
You can register a member ``funcdef``.

Here use the ``script_array`` from asbind20 extension as an example.
The same logic also applies to other classes.

.. code-block:: c++

    // ...
        .funcdef("bool erase_if_callback(const T&in if_handle_then_const)")
        .method("void erase_if(const erase_if_callback&in fn, uint idx=0, uint n=-1)", /* ... */);

.. note::
   Unlike the raw AngelScript interface,
   you don't need to add the class name into the declaration of member ``funcdef`` for asbind20.

Template Types
--------------

A template type in AngelScript works similarly to how templates work in C++.
The scripts will be able to instantiate different forms of the template type by specifying which subtype that should be used.
The methods for the instance will then be adapted to this subtype,
so that the correct handling of parameters and return types will be applied.

It is quite complex to implement a template class for AngelScript,
so they are described in a separated page.

.. toctree::
   :maxdepth: 2

   template_type
