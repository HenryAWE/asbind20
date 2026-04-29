Comparing Script Objects
========================

When implementing a container for script objects, you need a way to compare elements —
for ``operator==``, ``operator<=>``, ``std::sort``, or deduplication.
AngelScript classes can define ``opEquals`` and ``opCmp`` methods,
but calling them from C++ requires discovering them at runtime.

``container/compare.hpp`` provides tools to inspect a script type for these methods
and invoke them through a uniform interface.

.. note::
    The content of this file is designed for library developers implementing containers.
    If you are not building a custom container for script objects,
    you likely do not need these APIs.

Discovering Comparators
-----------------------

Call ``get_comparator`` with an ``asITypeInfo*`` to scan the type's methods.
It looks for methods matching these signatures:

- ``bool opEquals(const T&in) const``
- ``int opCmp(const T&in) const``

.. code-block:: c++

    #include <asbind20/container/compare.hpp>

    asITypeInfo* ti = /* ... */;

    auto result = asbind20::container::get_comparator(ti);
    auto status = result.get_status();

    if(status.opCmp >= 0) {
        // type has opCmp
    }
    if(status.opEquals >= 0) {
        // type has opEquals
    }
    if(!status.good()) {
        // neither method found — type is not comparable
    }

    asbind20::container::script_element_comparator cmp = result.get();

Using the Comparator
--------------------

Once obtained, ``script_element_comparator`` provides ``eq()`` and ``compare()``
that invoke the discovered script methods. Both take a script context and two ``const void*``
pointers to the elements.

.. code-block:: c++

    asIScriptContext* ctx = /* ... */;
    const void* lhs = /* pointer to first element */;
    const void* rhs = /* pointer to second element */;

    // Check equality — uses opEquals when available, otherwise falls back to opCmp() == 0
    if(cmp.eq(ctx, lhs, rhs)) {
        // elements are equal
    }

    // Compare — returns std::partial_ordering (unordered if comparison fails or is unavailable)
    std::partial_ordering ord = cmp.compare(ctx, lhs, rhs);
    if(ord < 0) {
        // lhs < rhs
    } else if(ord == 0) {
        // lhs == rhs
    } else if(ord > 0) {
        // lhs > rhs
    }

    // For more control, invoke the underlying script_method objects directly:
    auto eq_result = cmp.invoke_eq(ctx, lhs, rhs);
    if(eq_result.has_value() && eq_result.value()) {
        // ...
    }

Checking Availability
^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c++

    if(cmp.has_opEquals()) { /* opEquals is available */ }
    if(cmp.has_opCmp())    { /* opCmp is available */ }
    if(cmp)                 { /* at least one comparison method exists */ }

Integration with Containers
---------------------------

A typical pattern for a container implementing ``operator==``:

.. code-block:: c++

    template <typeinfo_policy TypeInfoPolicy>
    bool my_container::operator==(const my_container& other) const
    {
        if(this->size() != other.size())
            return false;

        auto cmp = container::get_comparator(this->element_type_info()).get();
        if(!cmp)
            return false; // not comparable — treat as not equal

        request_context ctx(this->get_engine());
        for(std::size_t i = 0; i < this->size(); ++i) {
            if(!cmp.eq(ctx, (*this)[i], other[i]))
                return false;
        }
        return true;
    }

.. doxygenclass:: asbind20::container::script_element_comparator
  :members:
  :undoc-members:

.. doxygenclass:: asbind20::container::get_comparator_result
  :members:
  :undoc-members:

.. doxygenfunction:: asbind20::container::get_comparator
