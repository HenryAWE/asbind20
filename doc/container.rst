Storing Script Objects
======================

Sometimes you may want to store script objects and use them later,
or to implement your own container for script objects.
There are some tools provided by asbind20 for storing script objects.

Storing a Single Script Object
------------------------------

.. doxygenclass:: asbind20::container::single
  :members:
  :undoc-members:

Containers
----------

Policies of Type Information
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Policies for how containers deal with the type information.

.. doxygenconcept:: asbind20::container::typeinfo_policy

.. doxygengroup:: TypeInfoPolicies
  :content-only:

Sequential Containers
~~~~~~~~~~~~~~~~~~~~~

``small_vector`` is a sequential container for AngelScript objects with small-size optimization (SSO).
It stores a fixed number of elements inline (on the stack) before falling back to heap allocation,
similar to how ``std::string`` uses SSO for short strings.

Template parameters
^^^^^^^^^^^^^^^^^^^

- **TypeInfoPolicy** — determines how the element type is derived from the type information.
  Use ``typeinfo_identity`` when the type info *is* the element type (e.g. ``int``, a registered value class),
  or ``typeinfo_subtype<N>`` when the element is the *N*-th subtype of a template type
  (e.g. ``array<int>`` → ``typeinfo_subtype<0>``).
- **StaticCapacityBytes** — inline storage size in bytes. Must be a multiple of ``sizeof(void*)``.
  Defaults to ``4 * sizeof(void*)``.
- **Allocator** — allocator for heap-backed storage. Defaults to ``script_allocator<void>``, which uses
  AngelScript's memory API.

Constructing
^^^^^^^^^^^^

A ``small_vector`` must be constructed with type information. There are three ways:

.. code-block:: c++

   // From an asITypeInfo pointer
   small_vector<typeinfo_identity> vec(some_typeinfo);

   // From an asITypeInfo pointer with initializer list
   small_vector<typeinfo_identity> vec(some_typeinfo, ilist);

   // From an engine and type ID (for primitive types, engine can be null)
   small_vector<typeinfo_identity> vec(nullptr, asTYPEID_INT32);

Element access returns ``void*`` / ``const void*``. You cast to the concrete type:

.. code-block:: c++

   small_vector<typeinfo_identity> vec(nullptr, asTYPEID_INT32);
   int val = 42;
   vec.push_back(&val);

   int* p = static_cast<int*>(vec[0]);

The container provides standard ``std::vector``-like modifiers: ``push_back``, ``emplace_back``,
``push_back_n``, ``emplace_back_n``, ``pop_back``, ``insert``, ``erase``, ``resize``, ``clear``,
``reserve``, ``shrink_to_fit``, ``reverse``, and ``assign``.

Iterators
^^^^^^^^^

``const_iterator`` is a random-access iterator that stores an offset into the vector rather than
a raw pointer. This makes it safe against reallocation — an invalid iterator from script can be
detected by the host without crashing. Dereferencing yields ``const void*`` to the element.

.. code-block:: c++

   for(auto it = vec.begin(); it != vec.end(); ++it) {
       int val = *static_cast<const int*>(*it);
   }

Visiting elements
^^^^^^^^^^^^^^^^^

The ``visit`` method applies a visitor to a range of elements, handling type-aware dispatch:

.. code-block:: c++

    vec.visit(
        [](auto* begin, auto* end)
        {
            for(auto* p = begin; p != end; ++p)
                // Do something
        },
        0, vec.size()
    );

GC integration
^^^^^^^^^^^^^^

Call ``enum_refs()`` to enumerate references within the vector to the AngelScript GC.
This should be called from the object type's ``EnumReferences`` callback.

.. doxygenclass:: asbind20::container::small_vector
  :members:
  :undoc-members:


Associative Containers
~~~~~~~~~~~~~~~~~~~~~~

*Not implemented yet*
