Script Initialization List
==========================

Types can be registered to allow them to be created from initialization lists.

List Factories and Constructors
-------------------------------

Similar to normal factories and constructors,
the list factories for reference types and list constructors for value types can be registered by ``.list_factory`` and ``.list_constructor``, respectively.

If you already have wrapper functions, you can register them by ``.list_factory_function`` or ``.list_constructor_function``

This kind of helpers needs you to provide pattern of the initialization list, such as ``float, float`` or ``repeat int``.
For helpers using the constructors of C++ class directly, you can also specify the optional template parameter ``ElementType`` for the element type of initialization list.
If the list pattern contains ``repeat`` or ``?`` that need to parse manually, you can just use the default template parameter,
which is ``void``.

The helpers expect the constructors or wrapper functions use a pointer (``ElementType*``) to receive the buffer of initialization list.
The requirements about return value and other parameters are the same as normal factories and constructors,
which is described in :doc:`how to register an object type <../object_type>`.

.. code-block:: c++

    struct my_val_class
    {
        my_val_class(int* il);
    };

    struct my_ref_class
    {
        my_ref_class(void* il);
    };

    struct my_class
    { /* ... */};

    void init_from_list(my_class* mem, void* il);

.. code-block:: c++

    asbind20::value_class<my_val_class>(/* ... */)
        .list_constructor<int>("int, int");

    asbind20::ref_class<my_ref_class>(/* ... */)
        .list_factory("repeat int"); // Equivalent to .list_factory<void>("repeat int")

    asbind20::value_class<my_class>(/* ... */)
        .list_factory_function("repeat int", &init_from_list);

.. note::
   Unlike C++, each type in AngelScript can only have **one** list factory or constructor.

For repeated pattern, asbind20 provides a helper class to parse it from a raw ``void*``.

.. doxygenclass:: asbind20::script_init_list_repeat
   :members:
   :undoc-members:

Initialization List Policies
----------------------------

The asbind20 provides some policies for adapting existing C++ paradigm for constructing from a range of value.

List Pattern with Limited Elements
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Policies for the pattern with limited elements, i.e., no ``repeat`` in the pattern.

1. Applying

  .. doxygenstruct:: asbind20::policies::apply_to

  Example

  .. code-block:: c++

    struct vec3f
    {
        vec3f(float x, float y, float z);
    };

  .. code-block:: c++

    using namespace asbind20;
    value_class<vec3f>(/* ... */)
        .list_constructor<float>("float,float,float", use_policy<policies::apply_to<3>>);

List Pattern with Repeated Elements
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Policies for the pattern with repeated elements, i.e., containing ``repeat`` in the pattern.

1. Automatically convert to ``script_init_list_repeat``

   .. doxygenstruct:: asbind20::policies::repeat_list_proxy

   Example

   .. code-block:: c++

    class int_list
    {
    public:
        int_list(asbind20::script_init_list_repeat il);
    };

   .. code-block:: c++

    using namespace asbind20;
    ref_class<int_list>(/* ... */)
        .list_factory("repeat int", use_policy<policies::repeat_list_proxy>);


1. As an iterator pair of ``[begin, end)``

   .. doxygenstruct:: asbind20::policies::as_iterators

   Example

   .. code-block:: c++

    class int_list
    {
    public:
        // Expects iterator to int
        template <typename Iterator>
        int_list(Iterator start, Iterator stop);
    };

   .. code-block:: c++

    using namespace asbind20;
    ref_class<int_list>(/* ... */)
        .list_factory("repeat int", use_policy<policies::as_iterators>);


2. As a pair of pointer and size

   .. doxygenstruct:: asbind20::policies::pointer_and_size

   Example

   .. code-block:: c++

    class int_list
    {
    public:
        int_list<int>(int* ptr, std::size_t count);
    };

   .. code-block:: c++

    using namespace asbind20;
    ref_class<int_list>(/* ... */)
        .list_factory<int>("repeat int", use_policy<policies::pointer_and_size>);

3. As the ``std::initializer_list``

   .. doxygenstruct:: asbind20::policies::as_initializer_list

   Example

   .. code-block:: c++

    class int_list
    {
    public:
        int_list(std::initializer_list<int> il);
    };

   .. code-block:: c++

    using namespace asbind20;
    ref_class<int_list>(/* ... */)
        .list_factory<int>("repeat int", use_policy<policies::as_initializer_list>);

   .. note::
      This policy is implemented by non-standard code, please check the macro ``ASBIND20_HAS_AS_INITIALIZER_LIST`` at first.

      Currently, this policy are supported on MSVC STL, libstdc++, and libc++.

4. As the ``std::span``

   .. doxygenstruct:: asbind20::policies::as_span

   Example

   .. code-block:: c++

    class int_list
    {
    public:
        int_list(std::span<int> sp);
    };

   .. code-block:: c++

    using namespace asbind20;
    ref_class<int_list>(/* ... */)
        .list_factory<int>("repeat int", use_policy<policies::as_span>);

5. ``std::from_range`` and a range

   .. doxygenstruct:: asbind20::policies::as_from_range

   Example

   .. code-block:: c++

    class int_list
    {
    public:
        template <typename Range>
        int_list(std::from_range_t, Range&& rng);
    };

   .. code-block:: c++

    using namespace asbind20;
    ref_class<int_list>(/* ... */)
        .list_factory<int>("repeat int", use_policy<policies::as_from_range>);

   .. note::
    This policy is only available when compiler supports C++23.
    You can check whether the macro ``ASBIND20_HAS_CONTAINERS_RANGES`` is defined.
